import os
import spydrnet as sdn
from glob import glob
import tempfile
import argparse
import logging
import re
import time
from datetime import timedelta
from datetime import datetime
from xml.dom import minidom

#####################################################################
# Initialize logger
#####################################################################
logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.INFO)

# Constants
XML_ROOT_NODE = "signal_distribution_network"
XML_NTWK_NODE = "network"
XML_NTWK_ATTR_PORT = "port"
XML_NTWK_BUF_NODE = "buf"
XML_NTWK_BUF_ATTR_NAME = "name"
XML_NTWK_BUF_ATTR_CELL = "cell"
XML_NTWK_BUF_ATTR_X = "x"
XML_NTWK_BUF_ATTR_Y = "y"
XML_NTWK_BUF_ATTR_PARENT = "parent"
XML_NTWK_BUF_CHILD_NODE = "child"
XML_NTWK_BUF_CHILD_ATTR_NAME = "name"
XML_NTWK_BUF_LEAF_TILE_NODE = "leaf_tile"
XML_NTWK_BUF_LEAF_TILE_ATTR_X = "x"
XML_NTWK_BUF_LEAF_TILE_ATTR_Y = "y"


# Basic model a buffer in the signal distribution network
class SdNtwkBuf:
    def __init__(self, name, cell, x, y, parent):
        self.__name__ = name
        self.__cell__ = cell
        self.__x__ = x
        self.__y__ = y
        self.__parent__ = parent
        self.__child__ = []
        self.__leaf_x__ = []
        self.__leaf_y__ = []
        self.__CELL_I_PORT__ = "I"
        self.__CELL_O_PORT__ = "Z"

    def is_top(self):
        return self.__parent__ == ""

    def cell_input_port_name(self):
        return self.__CELL_I_PORT__

    def cell_output_port_name(self):
        return self.__CELL_O_PORT__

    def name(self):
        return self.__name__

    def set_name(self, val):
        self.__name__ = val

    def cell(self):
        return self.__cell__

    def set_cell(self, val):
        self.__cell__ = val

    def parent(self):
        return self.__cell__

    def set_parent(self, val):
        self.__parent__ = val

    def location_x(self):
        return self.__x__

    def location_y(self):
        return self.__y__

    def set_location(self, val):
        self.__x__ = val[0]
        self.__y__ = val[1]

    def add_child(self, val):
        self.__child__.append(val)

    def is_leaf_node(self):
        return len(self.__leaf_x__) > 0

    def children(self):
        return self.__child__

    def add_leaf(self, val):
        self.__leaf_x__.append(val[0])
        self.__leaf_y__.append(val[1])

    def leaf_coords(self):
        ret = []
        for i in range(len(self.__leaf_x__)):
            ret.append((self.__leaf_x__[i], self.__leaf_y__[i]))
        return ret


class SigDistrNtwk:
    def __init__(self):
        self.__lib__ = None
        self.__top__ = None
        self.__core__ = None
        self.__bufcells__ = {}  # map from buf cell name to its module reference in the sdn netlist
        self.__modules__ = {}  # Modules to be added feedthrough {"module_name":sdn.defitinion,...}
        # Signal distribution network details [port] -> [list_of_buffers]
        self.__sd_ntwk__ = {}
        # Constants
        self.__FPGA_CORE_NAME__ = "fpga_core"

    def __expect_single_xml_node(self, xml_node):
        if len(xml_node) == 0:
            raise Exception(f"Expect the '{xml_node.nodeName}' node of XML to be defined!")
        if len(xml_node) != 1:
            raise Exception(f"Expect only 1 '{xml_node.nodeName}' node of XML to be defined!")

    # read signal distribution network
    def read_signal_distribution_network_from_xml(self, fpath):
        xml_db = minidom.parse(fpath)
        if xml_db.documentElement.tagName != XML_ROOT_NODE:
            raise Exception(
                f"Expect the root node of XML of be {XML_ROOT_NODE}! Currently it is {xml_db.documentElement.tagName}"
            )
        sdntwk_root_node = xml_db.getElementsByTagName(XML_ROOT_NODE)
        self.__expect_single_xml_node(sdntwk_root_node)
        # Parse network one-by-one
        ntwk_nodes = sdntwk_root_node[0].getElementsByTagName(XML_NTWK_NODE)
        for ntwk_node in ntwk_nodes:
            port = ntwk_node.attributes[XML_NTWK_ATTR_PORT].value
            # Initialize
            self.__sd_ntwk__[port] = []
            # Walk through buffers
            buf_nodes = ntwk_node.getElementsByTagName(XML_NTWK_BUF_NODE)
            for buf_node in buf_nodes:
                name = buf_node.attributes[XML_NTWK_BUF_ATTR_NAME].value
                cell = buf_node.attributes[XML_NTWK_BUF_ATTR_CELL].value
                x = buf_node.attributes[XML_NTWK_BUF_ATTR_X].value
                y = buf_node.attributes[XML_NTWK_BUF_ATTR_Y].value
                parent = buf_node.attributes[XML_NTWK_BUF_ATTR_PARENT].value
                new_ntwk_node = SdNtwkBuf(name, cell, float(x), float(y), parent)
                # Walt through children
                child_nodes = buf_node.getElementsByTagName(XML_NTWK_BUF_CHILD_NODE)
                for child_node in child_nodes:
                    child_name = child_node.attributes[XML_NTWK_BUF_CHILD_ATTR_NAME].value
                    new_ntwk_node.add_child(child_name)
                # Walt through leaf tiles
                leaf_nodes = buf_node.getElementsByTagName(XML_NTWK_BUF_LEAF_TILE_NODE)
                for leaf_node in leaf_nodes:
                    leaf_x = leaf_node.attributes[XML_NTWK_BUF_LEAF_TILE_ATTR_X].value
                    leaf_y = leaf_node.attributes[XML_NTWK_BUF_LEAF_TILE_ATTR_Y].value
                    new_ntwk_node.add_leaf((leaf_x, leaf_y))
                # all the required information completed and add it to the list
                self.__sd_ntwk__[port].append(new_ntwk_node)

        return 0

    def read_fabric_netlist(self, input_file):
        self.__lib__ = self.__open_netlist(input_file)
        self.__core__ = next(self.__lib__.get_definitions(self.__FPGA_CORE_NAME__))
        return 0

    def __open_netlist(self, fpga_core_file):
        netlist = None
        with tempfile.NamedTemporaryFile(suffix=".v") as fp:
            with open(fpga_core_file, "r", encoding="UTF-8") as fpv:
                for line in fpv:
                    if line.lstrip().find("`") == 0:  # not support macro in netlist yet
                        logging.warning(
                            f"!!!! Warning, not support macro yet, skip the following line in {fpga_core_file}\n\t{line}"
                        )
                        continue
                    fp.write(str.encode(line))
            fp.seek(0)
            logging.info(f"Parsing {fpga_core_file}...")
            netlist = sdn.parse(fp.name)
        return netlist

    def write_fabric_netlist(self, fpath):
        logging.info(
            f"Writing modified fpga_core netlist with signal distribution network to {fpath}"
        )
        parent_dir = os.path.dirname(fpath)
        if not os.path.exists(parent_dir):
            os.makedirs(parent_dir)
        sdn.compose(
            self.__lib__,
            fpath,
            definition_list=[self.__FPGA_CORE_NAME__],
            write_blackbox=False,
        )
        return 0

    def add_sig_distr_buf_to_netlist(self):
        num_err = 0
        for port_str in self.__sd_ntwk__.keys():
            logging.info(f"Adding signal distribution network for port '{port_str}'")
            num_err += self.__add_sig_distr_buf_from_one_ntwk_to_netlist(port_str)
            logging.info(f"Done")
        return num_err

    def __parse_verilog_port_from_string(self, port_str):
        # Regex breakdown:
        # ([a-zA-Z_][a-zA-Z0-9_$]*) -> Captures valid Verilog signal names
        # (?:\[(\d+):(\d+)\])?      -> Optionally captures [MSB:LSB]
        pattern = r"([a-zA-Z_][a-zA-Z0-9_$]*)(?:\[(\d+):(\d+)\])?"

        match = re.search(pattern, port_str)

        if match:
            return {
                "name": match.group(1),
                "msb": int(match.group(2)) if match.group(2) else None,
                "lsb": int(match.group(3)) if match.group(3) else None,
            }
        return None

    def __find_netlist_port(self, vport):
        return next(self.__core__.get_ports(vport["name"]), None)

    def __find_and_create_buffer(self, buf_node):
        # Avoid duplicated creation
        if buf_node.cell() in self.__bufcells__.keys():
            return self.__bufcells__[buf_node.cell()]
        buf_module = self.__core__.library.create_definition(buf_node.cell())
        buf_module.create_port(buf_node.cell_input_port_name(), pins=1, direction=sdn.IN)
        buf_module.create_port(buf_node.cell_output_port_name(), pins=1, direction=sdn.OUT)
        self.__bufcells__[buf_node.cell()] = buf_module
        return buf_module

    def __get_instance(self, inst_name):
        for inst in self.__core__.children:
            if inst.name == inst_name:
                return inst
        return None

    def __get_instance_port_pins(self, inst, ports):
        if isinstance(ports, str):
            ports = inst.reference.get_ports(ports)
        return (inst.pins[p] for port in ports for p in port.pins)

    def __find_buf_node_by_name(self, port_str, child_node):
        for buf_node in self.__sd_ntwk__[port_str]:
            if buf_node.name() == child_node:
                return buf_node
        return None

    def __get_receiver_pins(self, src_pin):
        ret = []
        wire = src_pin.wire
        if wire:
            for pin in wire.pins:
                if pin != src_pin:
                    ret.append(pin)
        return ret

    def __find_top_buf_node(self, port_str):
        for buf_node in self.__sd_ntwk__[port_str]:
            if buf_node.is_top():
                return buf_node
        return None

    def __add_sig_distr_buf_from_one_ntwk_to_netlist(self, port_str):
        num_err = 0
        # Find the port from the netlist module
        vport = self.__parse_verilog_port_from_string(port_str)
        if vport == None:
            logging.error(
                f"Invalid port definition {port_str} in Verilog format. Expect explicit LSB and MSB definition like clk[1:1]"
            )
            return 1
        if vport["msb"] != vport["lsb"]:
            logging.error(
                f"Invalid port definition {port_str} in Verilog format. Require to be single-bit like clk[1:1]"
            )
            return 1
        nlist_port = self.__find_netlist_port(vport)
        if nlist_port == None:
            logging.error(
                f"Invalid port {port_str} which cannot be found in the netlist! Available ports are: "
            )
            for curr_port in self.__core__.ports:
                logging.info(f"\t{curr_port.name}")
            return 1
        if vport["lsb"] >= len(nlist_port.pins):
            curr_idx = vport["lsb"]
            logging.error(
                f"Invalid port definition {port_str}. The pin index {curr_idx} is out of the range of port in netlist"
            )
            return 1
        nlist_pin = nlist_port.pins[vport["lsb"]]

        # Create all the buffer cell modules
        for buf_node in self.__sd_ntwk__[port_str]:
            self.__find_and_create_buffer(buf_node)
        # Insert all the buffers with all the instance names
        for buf_node in self.__sd_ntwk__[port_str]:
            buf_mod = self.__bufcells__[buf_node.cell()]
            self.__core__.create_child(name=buf_node.name(), reference=buf_mod)
        # Create cable to connect its child buffers
        for buf_node in self.__sd_ntwk__[port_str]:
            if buf_node.is_leaf_node():
                continue
            # Get the src pins
            src_buf_inst = self.__get_instance(buf_node.name())
            if src_buf_inst == None:
                logging.error(
                    f"Cannot find the buffer instance {buf_node.name()} in the modified netlist"
                )
                return 1
            src_pin = next(
                self.__get_instance_port_pins(src_buf_inst, buf_node.cell_output_port_name()), None
            )
            # Collect sink pins
            des_buf_pins = []
            for child_node in buf_node.children():
                child_buf = self.__find_buf_node_by_name(port_str, child_node)
                if child_buf == None:
                    logging.error(
                        f"Cannot find the buffer description {buf_node.name()} in signal distribution network on port {port_str}"
                    )
                    return 1
                des_buf_inst = self.__get_instance(child_buf.name())
                if des_buf_inst == None:
                    logging.error(
                        f"Cannot find the buffer instance {buf_node.name()} in the modified netlist"
                    )
                    return 1
                des_buf_pins.append(
                    next(
                        self.__get_instance_port_pins(
                            des_buf_inst, child_buf.cell_input_port_name()
                        ),
                        None,
                    )
                )
            new_cable_name = buf_node.name() + "_" + buf_node.cell_output_port_name()
            new_cable = sdn.Cable(name=new_cable_name)
            new_cable.create_wire()
            new_cable.wires[-1].connect_pin(src_pin)
            for des_pin in des_buf_pins:
                new_cable.wires[-1].connect_pin(des_pin)
            self.__core__.add_cable(new_cable)

        # Create cable to connect buffers to leaf tiles
        for buf_node in self.__sd_ntwk__[port_str]:
            if not buf_node.is_leaf_node():
                continue
            # Get the src pins
            src_buf_inst = self.__get_instance(buf_node.name())
            if src_buf_inst == None:
                logging.error(
                    f"Cannot find the buffer instance {buf_node.name()} in the modified netlist"
                )
                return 1
            src_pin = next(
                self.__get_instance_port_pins(src_buf_inst, buf_node.cell_output_port_name()), None
            )
            # Collect buf pins: for each qualified child pin, disconnect from the original wires and connect to the new wire
            candidate_pins = self.__get_receiver_pins(nlist_pin)
            sink_pins = []
            for des_coord in buf_node.leaf_coords():
                # FIXME: Make it flexible
                expected_inst_name = f"tile_{des_coord[0]}__{des_coord[1]}_"
                for cand_pin in candidate_pins:
                    if cand_pin.instance.name == expected_inst_name:
                        sink_pins.append(cand_pin)
            # Disconnect
            for des_pin in sink_pins:
                nlist_pin.wire.disconnect_pin(des_pin)
            # Re-connect
            new_cable_name = buf_node.name() + "_" + buf_node.cell_output_port_name()
            new_cable = sdn.Cable(name=new_cable_name)
            new_cable.create_wire()
            new_cable.wires[-1].connect_pin(src_pin)
            for des_pin in sink_pins:
                new_cable.wires[-1].connect_pin(des_pin)
            self.__core__.add_cable(new_cable)

        # Create cable to connect top-module pin (nlist_pin) to the top-level buffer
        top_buf_node = self.__find_top_buf_node(port_str)
        if top_buf_node == None:
            logging.error(
                f"Cannot find the top buffer (without any parent) from the signal distribution network for {port_str}"
            )
            return 1
        # Get the src pins
        top_buf_inst = self.__get_instance(top_buf_node.name())
        if top_buf_inst == None:
            logging.error(
                f"Cannot find the buffer instance {top_buf_node.name()} in the modified netlist"
            )
            return 1
        top_buf_des_pin = next(
            self.__get_instance_port_pins(top_buf_inst, buf_node.cell_input_port_name()), None
        )
        # Reconnect
        nlist_pin.wire.connect_pin(top_buf_des_pin)

        return num_err

    def write_pd_constraint_tcl(self, fname, fpga_core_inst_name):
        logging.info(f"Writing physical design constraints to {fname}")
        with open(fname, "w") as f:
            for port_str in self.__sd_ntwk__.keys():
                for buf_node in self.__sd_ntwk__[port_str]:
                    # write location constraints
                    coord_str = (
                        "{" + str(buf_node.location_x()) + " " + str(buf_node.location_y()) + "}"
                    )
                    cell_str = fpga_core_inst_name + "/" + buf_node.name()
                    f.write(f"set_cell_location -coordinates {coord_str} {cell_str}\n")

        logging.info(f"Done")
        return 0
