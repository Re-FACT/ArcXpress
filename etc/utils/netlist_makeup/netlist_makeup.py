"""
File name: netlist_makeup.py
Descriptions:
    This script will add feedtrhough to specific global signals in tile level. And reconnect global signals according to fabric_key.xml
    To load netlist: instance = netlist_makeup(.../fabric_netlists.v) if there are many file in sub folders,
        or instance = netlist_makeup(.../*.v) if no sub folder in that path.
    To load fabric_key files: instance.parse_fabric_key(fabric_key_file=...,ref_module_file=...,module_name_file=...)
    To add feedthrough to specific module: instance.add_feedthrough(module_name,signal_name,std_cell_name_for_buffer)
    To same udpated netlist: instance.save_netlist(output_path)
    To reconnect feedthrough signals: instance.reorder_feedthrough_signal(signal_name)
    Script will add feedthrough output pins use name like: prog_reset_fto, prog_clk_fto or config_enale_fto
    This script only save updated modules, by default they are: fpga_core.v and tile/*.v
    Refer to example codes in ___main___ for more details
Author: Weijun
Date: 2024/02/08
Revision: 1.08
V1.08
 - Update remove_undriven_port() to tie floating input pins to const0
V1.07
 - Add buffer to mem_init_clk_i and adjust feedthrough sequence to have *_ftb input connect to tile input port instead of *_mkbuf_out
V1.06
 - Add buffers into prog_clk and prog_reset net in each tile by insert_net_buffer_in_tile()
V1.05
 - Create subroutine to add buffer into net and wrap related functions for clock buffering
V1.00
 - Develop new features to connect bscan signals by xml sequence and create SI/SO port from level 0 grid to fpga top
V0.95
 - Add function call create_clock_output() to create clock output ports; Updated add_feedthrough() with new param idx;
   Have new feature to remove undriven ports in tile; Update parse_makeup_key() to parse undriven ports
   Show error message in reorder_feedthrough_signal()
V0.90
 - Add feedthrough for bram initialization signals and set index of bram tiles
V0.85
 - Parse_fabric_key() line "tmp.sort(key=lambda x:(x[0]))" adjust to tmp.sort(key=lambda x:int(x[0])), sort by number instead of text
V0.82
 - Initial release
"""

import os
import spydrnet as sdn
from glob import glob
import tempfile
import xml.etree.ElementTree as ET
import argparse
import logging
import re
import time
from datetime import timedelta
from datetime import datetime
import netlist_makeup_config

#####################################################################
# Error codes
#####################################################################
error_codes = {"SUCCESS": 0, "ERROR": 1, "FILE_ERROR": 2}

#####################################################################
# Initialize logger
#####################################################################
logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.INFO)


class netlist_makeup:
    lib = None
    top = None
    core = None
    bufs = (
        {}
    )  # Buffers type from std cells to be used in feedthrough: {"bufname":buf_definition,...}
    modules = {}  # Modules to be added feedthrough {"module_name":sdn.defitinion,...}
    cables = (
        {}
    )  # Cables to connect feedthrough ports in fpga_core level, each global signal has one cable
    fabkey = []  # FPGA fabric key: [region0,region1...], region->[[id,alias],...]
    mkpkey = {}  # {"type name":[[[region0 id,instance,module],[id,instance,module]],[region1],...)}

    def __init__(self, input_files="", topname="fpga_top", corename="fpga_core"):
        self.lib = self.open_netlist(input_files)
        self.top = next(self.lib.get_definitions(topname))
        self.core = next(self.lib.get_definitions(corename))
        # assign_library = next(self.lib.get_libraries("SDN_VERILOG_ASSIGNMENT"), None)
        # const0 = next(self.lib.get_definitions("const0"),None)

    def open_netlist(self, input_files=""):
        netlist = None
        if input_files.find("*") >= 0:
            verilog_files = glob(input_files)
        else:
            verilog_files = []
            path = input_files[: input_files.rfind("/") + 1]
            with open(input_files, "r", encoding="UTF-8") as fpi:
                for line in fpi:
                    if line.lstrip().find("`include") == 0:
                        fname = (
                            line.replace("`include", "").replace('"', "").replace("\n", "").lstrip()
                        )
                        verilog_files.append(path + fname)
        with tempfile.NamedTemporaryFile(suffix=".v") as fp:
            for each_file in verilog_files:
                if each_file.rfind(".v") != len(each_file) - 2:
                    continue
                logging.info(each_file)
                with open(each_file, "r", encoding="UTF-8") as fpv:
                    for line in fpv:
                        if line.lstrip().find("`") == 0:  # not support macro in netlist yet
                            logging.warning(
                                f"!!!! Warning, not support macro yet, skip this file: {each_file}"
                            )
                            break
                        fp.write(str.encode(line))
                    # fp.write(str.encode(" ".join(fpv.readlines())))
            fp.seek(0)
            # os.system(f'cp {fp.name} test.v') # for debug only
            netlist = sdn.parse(fp.name)
        return netlist

    def get_module_and_instances(self):
        module_list = []  # [[module name,instance name.... ],...]
        module_dict = {}
        if self.core is None:
            return module_list
        instances = self.core.get_instances()
        for ins in instances:
            module_def = ins.reference
            if module_def.name in module_dict:
                idx = module_dict[module_def.name]
                module_list[idx].append(ins.name)
            else:
                module_dict[module_def.name] = len(module_dict)
                module_list.append([module_def.name, ins.name])
        return module_list

    def add_feedthrough(self, module_name, port_name, bufcell, idx=None):
        if module_name is None or len(module_name) == 0 or len(port_name) == 0:
            return None
        module: sdn.Definition = next(self.lib.get_definitions(module_name))
        port: sdn.Port = next(module.get_ports(port_name), None)
        cable: sdn.Cable = next(module.get_cables(port_name), None)
        if cable is None:
            logging.error(f"!!!! Error, not found wire {port_name} when add signal feedthrough")
            return None
        # Create feedthrough output port+buf and connect to related input signal
        idx_tag = "" if idx == None else f"_{idx}"
        p = module.create_port(f"{port_name}_fto{idx_tag}", pins=1, direction=sdn.OUT)
        cable_ft = module.create_cable(f"{port_name}_fto{idx_tag}", wires=1)
        cable_ft.wires[0].connect_pin(p.pins[0])
        buf = self.create_buffer(bufcell)
        buf_inst = module.create_child(name=f"{port_name}_ftb{idx_tag}", reference=buf)
        buf_in = next(self.get_port_pins(buf_inst, "I"))
        buf_out = next(self.get_port_pins(buf_inst, "Z"))
        cable.wires[0].connect_pin(buf_in)
        cable_ft.wires[0].connect_pin(buf_out)

    def create_buffer(self, stdcell_name):
        if stdcell_name in self.bufs:
            return self.bufs[stdcell_name]
        elif stdcell_name[:4] == "TIEH":
            buf = self.core.library.create_definition(stdcell_name)
            buf.create_port("Z", pins=1, direction=sdn.OUT)
            self.bufs[stdcell_name] = buf
        elif stdcell_name[:4] == "TIEL":
            buf = self.core.library.create_definition(stdcell_name)
            buf.create_port("ZN", pins=1, direction=sdn.OUT)
            self.bufs[stdcell_name] = buf
        else:
            buf = self.core.library.create_definition(stdcell_name)
            buf.create_port("I", pins=1, direction=sdn.IN)
            buf.create_port("Z", pins=1, direction=sdn.OUT)
            self.bufs[stdcell_name] = buf
        return buf

    def get_port_pins(self, inst, ports):
        """
        Returns all the outerpins of this port on this instance
        args:
                port (Port, str): Port of the instance
        """
        if isinstance(ports, str):
            ports = inst.reference.get_ports(ports)
        return (inst.pins[p] for port in ports for p in port.pins)

    def parse_fabric_key(self, fabric_key_file, ref_module_file="", module_name_file=""):
        ref_mod = {}
        mod_nam = {}
        if ref_module_file != "":
            tree = ET.parse(ref_module_file)
            root = tree.getroot()
            if root.tag != "module_names":
                logging.error("!!!! Error, invalid ref_module_file: ", ref_module_file)
                return
            for mod in root:
                d = mod.get("default")
                g = mod.get("given")
                ref_mod[d] = g
        if module_name_file != "":
            tree = ET.parse(module_name_file)
            root = tree.getroot()
            if root.tag != "module_names":
                logging.error("!!!! Error, invalid module_name_file: ", module_name_file)
                return
            for mod in root:
                d = mod.get("default")
                g = mod.get("given")
                mod_nam[d] = g
        tree = ET.parse(fabric_key_file)
        root = tree.getroot()
        if root.tag == "fabric_key" and root[0].tag == "module" and root[0][0].tag == "region":
            name = root[0][0].attrib
            for region in root[0]:
                tmp = []
                for key in region:
                    id = key.get("id")
                    alias = key.get("alias")
                    tmp.append([id, alias])
                    name = key.get("name")
                    name = ref_mod[name] if name in ref_mod else name
                    name = mod_nam[name] if name in mod_nam else name
                    if name in self.modules:
                        continue
                    self.modules[name] = next(self.lib.get_definitions(name))
                tmp.sort(key=lambda x: int(x[0]))
                self.fabkey.append(tmp)
        else:
            logging.error("!!!! Error, invalid fabric key: ", fabric_key_file)

    # Reconnect feedthrough signal based on fabric key sequence
    def reorder_feedthrough_signal(self, signal_name, regions=None):
        if regions == None:
            regions = self.fabkey
        for region in regions:
            for t in range(len(region) - 1):
                drv = region[t][1]
                drv = next(self.core.get_instances(drv))
                drv = self.get_port_pins(drv, f"{signal_name}_fto")
                try:
                    drv = next(drv)
                except:
                    drv = None
                if drv is None:
                    logging.error(
                        f"!!! Error: fail to connect drive of {signal_name} in instance {region[t][1]}"
                    )
                    continue
                load = region[t + 1][1]
                load = next(self.core.get_instances(load))
                load = self.get_port_pins(load, signal_name)
                try:
                    load = next(load)
                except:
                    load = None
                if load is None:
                    logging.error(
                        f"!!! Error: fail to connect sink of {signal_name} in instance {region[t+1][1]}"
                    )
                    continue
                cable = next(self.core.get_cables(signal_name))
                windex = 0
                for w in cable.wires:  # Find proper wire index if signal bit width > 1
                    if load in w.pins:
                        windex = w.index()
                cable.wires[windex].disconnect_pin(load)
                key = f"{signal_name}_fts"
                if key in self.cables:
                    ft_cable = self.cables[key]
                    ft_cable.create_wire()
                else:
                    ft_cable = self.core.create_cable(key, wires=1)
                    self.cables[key] = ft_cable
                ft_cable.wires[-1].connect_pin(drv)
                ft_cable.wires[-1].connect_pin(load)

    def save_netlist(self, path="netlist_makeup_out"):
        if not os.path.exists(path):
            os.makedirs(path)
        if not os.path.exists(path + "/tile"):
            os.makedirs(path + "/tile")
        # sdn.compose(self.lib,path+"/fpga_all_out.v",write_blackbox=False)
        sdn.compose(
            self.lib,
            path + "/" + self.core.name + ".v",
            definition_list=[self.core.name],
            write_blackbox=False,
        )
        sdn.compose(
            self.lib,
            path + "/" + self.top.name + ".v",
            definition_list=[self.top.name],
            write_blackbox=False,
        )
        for tile in self.modules:
            sdn.compose(
                self.lib,
                path + "/tile/" + tile + ".v",
                definition_list=[tile],
                write_blackbox=False,
            )
        if "scan_chains" in self.mkpkey:
            if not os.path.exists(path + "/lb"):
                os.makedirs(path + "/lb")
            for lv in self.mkpkey["scan_chains"]:
                modules = self.mkpkey["scan_chains"][lv]["module"]
                if int(lv) > 0:
                    continue
                for mod in modules:
                    sdn.compose(
                        self.lib,
                        path + "/lb/" + mod + ".v",
                        definition_list=[mod],
                        write_blackbox=False,
                    )

    def parse_makeup_key(self, makeup_key_file=""):
        typ = None
        if makeup_key_file != "":
            tree = ET.parse(makeup_key_file)
            root = tree.getroot()
            if root.tag != "makeup_key":
                logging.error("!!!! Error, invalid makeup_key_file: ", makeup_key_file)
                return
            region_lst = []
            kmodule = {}
            for el in root:
                if el.tag == "signal":
                    typ = el.get("name")
                    signal_lst = []
                    for sig in el:
                        name = sig.get("name")
                        width = int(sig.get("width"))
                        if sig.tag == "mem_index":
                            self.mkpkey[f"{typ}_mem_index"] = (name, width)
                        elif sig.tag == "port":
                            umod = sig.get("module")
                            signal_lst.append([name, width, umod])
                            if umod not in kmodule:
                                kmodule[umod] = next(self.lib.get_definitions(umod))
                        else:
                            signal_lst.append([name, width])
                    self.mkpkey[f"{typ}_signal"] = signal_lst
                elif el.tag == "region":
                    key_lst = []
                    for ft in el:
                        idx = ft.get("id")
                        mod = ft.get("module")
                        ins = ft.get("instance")
                        # [idx,ins,...] this sequence is compatible with self.fabkey
                        key_lst.append([idx, ins, mod])
                        if mod in kmodule:
                            continue
                        kmodule[mod] = next(self.lib.get_definitions(mod))
                    key_lst.sort(key=lambda x: int(x[0]))
                    region_lst.append(key_lst)
                elif el.tag == "scanchain":
                    self.parse_scan_chain(el)
                elif el.tag == "clock":
                    self.parse_clock_net(el)

            self.mkpkey[f"{typ}_region"] = region_lst
            self.mkpkey[f"{typ}_module"] = kmodule
        return typ

    def set_bram_mem_index(self, type_name, std_lib="7TR40"):
        key = f"{type_name}_mem_index"
        if key not in self.mkpkey:
            logging.error(f"!!!! Error, type of {type_name} is invalid to set memory index")
            return
        mods = self.mkpkey[f"{type_name}_module"]
        # Create tie H/L instance and ports in related modules
        for m in mods:
            for cell in ["tiel", "tieh"]:
                pt: sdn.Port = next(mods[m].get_ports(f"mem_{cell}_fto"), None)
                pout = "Z" if cell == "tieh" else "ZN"
                if pt == None:
                    pot = mods[m].create_port(f"mem_{cell}_fto", pins=1, direction=sdn.OUT)
                    tie = self.create_buffer(f"{cell.upper()}{std_lib}")
                    tie_inst = mods[m].create_child(name=f"mem_{cell}_ftb", reference=tie)
                    tie_out = next(self.get_port_pins(tie_inst, pout))
                    cable_ft = mods[m].create_cable(f"mem_{cell}_fto", wires=1)
                    cable_ft.wires[0].connect_pin(tie_out)
                    cable_ft.wires[0].connect_pin(pot.pins[0])
        # Connect mem_index pins to tie H/L port according to instance id
        for region in self.mkpkey[f"{type_name}_region"]:
            for key in region:
                idx = bin(int(key[0]))[2:]
                inst = key[1]
                num = int(self.mkpkey[f"{type_name}_mem_index"][1])
                name = self.mkpkey[f"{type_name}_mem_index"][0]
                ram_inst = next(self.core.get_instances(inst))
                if len(idx) < 8:
                    idx = "0000000" + idx
                    idx = idx[-8:]
                # get tie H/L pins and create tile specific cable/wire
                tieh_pin = next(self.get_port_pins(ram_inst, "mem_tieh_fto"), None)
                tiel_pin = next(self.get_port_pins(ram_inst, "mem_tiel_fto"), None)
                tieh_cable = self.core.create_cable(f"{inst}_mem_tieh_fto", wires=1)
                tiel_cable = self.core.create_cable(f"{inst}_mem_tiel_fto", wires=1)
                tieh_cable.wires[0].connect_pin(tieh_pin)
                tiel_cable.wires[0].connect_pin(tiel_pin)
                # Connect all mem_index pins with tie H/L pins
                for i in range(num):
                    cname = f"{inst}_undriven_{name}_{i}_"
                    # print(idx,idx[-1-i],cname)
                    undriven: sdn.Cable = next(self.core.get_cables(cname), None)
                    sik = next(self.get_port_pins(ram_inst, f"{name}_{i}_"))
                    undriven.wires[0].disconnect_pin(sik)
                    self.core.remove_cable(undriven)  # remove undriven wire
                    ft_cable = tieh_cable if idx[-1 - i] == "1" else tiel_cable
                    ft_cable.wires[0].connect_pin(sik)

    # Create clock output from fpga_core, clk_list = [[clk name, output numbers], ...]
    def create_clock_output(self, clk_list=None, bufcell=None):
        if clk_list == None or bufcell == None:
            return
        core_insts = self.top.get_instances()
        core_inst = None
        for core in core_insts:
            # core.name is instance name, core.reference.name is module name
            if core.reference.name == self.core.name:
                core_inst = core
        if core_inst == None:
            logging.error("!!! Error: core instance not found")
        for ck in clk_list:
            name = ck[0]
            num = ck[1]
            for i in range(num):
                self.add_feedthrough(self.core.name, name, bufcell, i)
                drv_pin = next(self.get_port_pins(core_inst, f"{name}_fto_{i}"))
                sik_port = self.top.create_port(f"{name}_fto_{i}", pins=1, direction=sdn.OUT)
                cable_ft = self.top.create_cable(f"{name}_fto_{i}", wires=1)
                cable_ft.wires[0].connect_pin(drv_pin)
                cable_ft.wires[0].connect_pin(sik_port.pins[0])

    # Remove undriven port of specific tile
    def remove_undriven_port(self, type_name=None, std_lib="7TR40", tie_low=True):
        if type_name == None:
            logging.error("!!! Error: remove_underiven_port needs a valid type_name")
            return
        mods = self.mkpkey[f"{type_name}_module"]
        pots = self.mkpkey[f"{type_name}_signal"]
        for p in pots:  # Remove undriven port and wire in tile
            pname = p[0]
            mname = p[2]
            # print(f"remove {pname} in {mname}")
            port = next(mods[mname].get_ports(pname), None)
            if port == None:
                logging.warning(f"Warning: can't find {pname} in {mname}")
                continue
            cable = (port.pins)[
                0
            ].wire.cable  # get connected cable of this port, cable contains several wires
            if cable.name != pname:
                logging.error(
                    "!!! Error: port name and undriven signal name can't match in module -", mname
                )
                return
            if port.direction is sdn.IN:
                # Create tie L cell if not available yet
                tiel_cable = next(mods[mname].get_cables(f"undriven_tiel_ft0"), None)
                if tiel_cable == None:
                    tie = self.create_buffer(f"TIEL{std_lib}")
                    tie_inst = mods[mname].create_child(name=f"undriven_tiel_ftb", reference=tie)
                    tie_out = next(self.get_port_pins(tie_inst, "ZN"))
                    tiel_cable = mods[mname].create_cable(f"undriven_tiel_ft0", wires=1)
                    tiel_cable.wires[0].connect_pin(tie_out)
                # Disconnect wires of port.pins and cable.wires
                for drv_pin in port.pins:
                    # if type(drv_pin)==sdn.InnerPin
                    wid = drv_pin.wire.index()
                    cable.wires[wid].disconnect_pin(drv_pin)
                    # Attention:
                    # cable.wires[].pins are OuterPin, used outside of module;
                    # cable.wires[].get_pins() are InnerPin, used inside of module;
                    sik_pins = (cable.wires[wid]).pins
                    # disconnect previous wire and reconnect with tie L tiel_cable.wires
                    for sik in sik_pins:
                        cable.wires[wid].disconnect_pin(sik)
                        tiel_cable.wires[0].connect_pin(sik)
                mods[mname].remove_cable(cable)
            mods[mname].remove_port(port)

        # Remove undriven cable/wires in fpga core module
        core_cables = self.core.get_cables()
        udr_cables = []
        tiel_cables = []
        for c in core_cables:
            if c.name.find("undriven") != -1:
                udr = 1
                # Make sure all wires are undriven before remove cable
                for w in c.wires:
                    if len(w.pins) > 0:
                        if tie_low:
                            for px in w.pins:
                                if type(px) is sdn.OuterPin:
                                    if px.inner_pin.port.direction is sdn.IN:
                                        tiel_cables.append(c)
                                    else:
                                        udr = 0
                                elif px.port.direction is sdn.IN:
                                    udr = 0
                        else:
                            udr = 0
                # Attention: don't remove cable when iteration, otherwise the index may change or miss some elements
                # if udr==1: self.core.remove_cable(c)
                if udr == 1:
                    udr_cables.append(c)
                else:
                    logging.warning(f"Warning: suspcious undriven cable - {c.name}")
        # Tile specific wire to const0
        cnt = 0
        # const0 = next(self.lib.get_definitions("const0"),None) # This line enable user defined const0 module
        const0 = self.create_buffer(f"TIEL{std_lib}")  # Enable this line when use standard cell
        for c in tiel_cables:
            if const0 == None:
                logging.warning(f"Warning: suspcious undriven cable - {c.name}")
                continue
            else:
                logging.info(f"Rename cable {c.name} and tie related inputs to const0_tiel_{cnt}")
            consti = self.core.create_child(name=f"const0_tiel_{cnt}", reference=const0)
            # tie_out = next(self.get_port_pins(consti, "const0")) # This line enable user defined const0 module
            tie_out = next(
                self.get_port_pins(consti, "ZN")
            )  # Enable this line when use standard cell
            if len(c.wires) > 1:
                logging.warning(f"Warning: cable {c.name} has width > 0, only index [0] tie low!")
            for w in c.wires:
                w.connect_pin(tie_out)
            c.name = c.name.replace("undriven_", "")
            cnt += 1
            # for w in c.wires:
            #    for px in w.pins: pass

        # It is safe to remove cable after all element are found
        for c in udr_cables:
            self.core.remove_cable(c)

    # Parse scain chain in xml file
    def parse_scan_chain(self, el, si="SI", so="SO"):
        lv = el.get("level")
        grid = el.get("grid")
        if "scan_chains" not in self.mkpkey:
            self.mkpkey["scan_chains"] = {}
        if lv not in self.mkpkey["scan_chains"]:
            self.mkpkey["scan_chains"][lv] = {}
        dic = self.mkpkey["scan_chains"][lv]
        if "module" not in dic:
            dic["module"] = {}
        if grid in dic["module"]:
            logging.error("!!! Error: duplicated xml elements -", grid)
        dic["module"][grid] = next(self.lib.get_definitions(grid), None)
        if dic["module"][grid] == None:
            logging.error(f"!!! Error: fail to find module {grid} in bscan xml file ")
            return
        chain_i = []
        chain_o = []
        for ln in el:
            if ln.tag == "signal":
                io = ln.get("io")
                idx = ln.get("id")
                pre = ln.get("prefix")
                suf = ln.get("suffix")
                wid = ln.get("width")
                if io == si:
                    chain_i.append([idx, pre, suf, wid])  # dic["signal"][f"{grid}_si"]=(name,sub)
                elif io == so:
                    chain_o.append([idx, pre, suf, wid])  # dic["signal"][f"{grid}_so"]=(name,sub)
                else:
                    logging.warning(f"Warning: wrong io name in xml file - {io} {grid}")
        chain_i.sort(key=lambda x: int(x[0]))
        chain_o.sort(key=lambda x: int(x[0]))
        if len(chain_i) != len(chain_o):
            logging.warning("!!! Error: si/so pin count can't match, please check xml file")
        dic[f"{grid}_si"] = chain_i
        dic[f"{grid}_so"] = chain_o

    # Create SI/SO port in fpga top and core module, connect in proper sequence
    def reorder_scan_chain_signal(self, type_name, si="SI", so="SO"):
        self.reorder_sub_scan_chain_signal(si, so)
        core_si_port = self.core.create_port(si, pins=1, direction=sdn.IN)
        core_so_port = self.core.create_port(so, pins=1, direction=sdn.OUT)
        core_si_cable = self.core.create_cable(si, wires=1)
        core_so_cable = self.core.create_cable(so, wires=1)
        scan_cable = self.core.create_cable(f"{si}_{so}_chain", wires=1)
        # Connect SI/SO signal in fpga core module
        regions = self.mkpkey[f"{type_name}_region"]
        for region in regions:
            for t in range(len(region)):
                inst = region[t][1]
                inst = next(self.core.get_instances(inst))
                drv = next(self.get_port_pins(inst, so), None)
                if drv is None:
                    logging.error(f"!!! Error: fail to connect SO pin in instance {region[t][1]}")
                    continue
                # load = region[t][1]if t==0 or t==len(region)-1 else region[t+1][1]
                # load = next(self.get_port_pins(load,si),None)
                load = next(self.get_port_pins(inst, si), None)
                if load is None:
                    logging.error(f"!!! Error: fail to connect SI pin in instance {region[t+1][1]}")
                    continue
                if t == 0:
                    core_si_cable.wires[0].connect_pin(core_si_port.pins[0])
                    core_si_cable.wires[0].connect_pin(load)
                    scan_cable.wires[0].connect_pin(drv)
                elif t == len(region) - 1:
                    scan_cable.wires[-1].connect_pin(load)
                    core_so_cable.wires[0].connect_pin(core_so_port.pins[0])
                    core_so_cable.wires[0].connect_pin(drv)
                else:
                    scan_cable.wires[-1].connect_pin(load)
                    scan_cable.create_wire()
                    scan_cable.wires[-1].connect_pin(drv)

        # Connect SI/SO signal to fpga top module
        core_insts = self.top.get_instances()
        core_inst = None
        for core in core_insts:
            if core.reference.name == self.core.name:
                core_inst = core
        if core_inst == None:
            logging.error("!!! Error: core instance not found")
        top_si_port = self.top.create_port(si, pins=1, direction=sdn.IN)
        top_so_port = self.top.create_port(so, pins=1, direction=sdn.OUT)
        top_si_cable = self.top.create_cable(si, wires=1)
        top_so_cable = self.top.create_cable(so, wires=1)
        drv = next(self.get_port_pins(core_inst, so), None)
        load = next(self.get_port_pins(core_inst, si), None)
        top_si_cable.wires[0].connect_pin(top_si_port.pins[0])
        top_si_cable.wires[0].connect_pin(load)
        top_so_cable.wires[0].connect_pin(top_so_port.pins[0])
        top_so_cable.wires[0].connect_pin(drv)

    # Reconnect scan chain signal in tile or sub modules
    def reorder_sub_scan_chain_signal(self, si="SI", so="SO"):
        if "scan_chains" not in self.mkpkey:
            return
        keys = sorted(self.mkpkey["scan_chains"].keys(), key=lambda x: int(x))
        remain_port = {}
        for lv in keys:
            modules = self.mkpkey["scan_chains"][lv]["module"]
            for mod in modules:
                # print("reorder module -",mod)
                si_lst = self.mkpkey["scan_chains"][lv][f"{mod}_si"]
                so_lst = self.mkpkey["scan_chains"][lv][f"{mod}_so"]
                # Find SI/SO port otherwise create it
                si_head = next(modules[mod].get_ports(si), None)
                so_tail = next(modules[mod].get_ports(si), None)
                si_calbe = next(modules[mod].get_cables(si), None)
                so_calbe = next(modules[mod].get_cables(si), None)
                if si_head is None:
                    si_head = modules[mod].create_port(si, pins=1, direction=sdn.IN)
                    si_calbe = modules[mod].create_cable(si, wires=1)
                    si_head = si_head.pins[0]
                    so_tail = modules[mod].create_port(so, pins=1, direction=sdn.OUT)
                    so_calbe = modules[mod].create_cable(so, wires=1)
                    so_tail = so_tail.pins[0]
                si_sik = None
                so_drv = None
                ch_drv = None
                # Loop to connect all instances that has SI/SO port
                for i in range(len(si_lst)):
                    gsi = si_lst[i]
                    gso = so_lst[i]
                    wid = int(gsi[3])
                    inst = None
                    has_si = 0
                    # Loop to connect instances that has no SI/SO port
                    for idx in range(wid):
                        cname = f"{gsi[1]}_{idx}_{gsi[2]}"
                        cgsi = next(modules[mod].get_cables(cname), None)
                        cgso = next(modules[mod].get_cables(f"{gso[1]}_{idx}_{gso[2]}"), None)
                        ch_drv = si_head if ch_drv == None else so_drv
                        if cgsi == None:
                            logging.error(f"!!! Error: can't find scan signal {cname} in {mod}")
                            continue
                        # Find related sc_in pins
                        for p in cgsi.wires[0].pins:
                            if type(p) == sdn.OuterPin:
                                # level 0 module keep sc_in index 0 for flag to find proper instance name to facility upper module
                                inst = p.instance
                                si_sik = next(self.get_port_pins(inst, si), None)
                                if si_sik == None:
                                    si_sik = p
                                else:
                                    # above sc_in index 0 in level 0 will be remove outside this loop, so same it in a dict
                                    remain_port[inst.reference.name] = p.inner_pin.port
                                    has_si = 1
                                if idx == 0:
                                    cgsi.wires[0].disconnect_pin(p)
                        for p in cgsi.wires[
                            0
                        ].pins:  # It is safe to disconnect after all pins are iterated in above loop
                            if type(p) == sdn.InnerPin:
                                cgsi.wires[0].disconnect_pin(p)
                        # Find related sc_out pins
                        so_drv = next(self.get_port_pins(inst, so), None)
                        for p in cgso.wires[0].pins:
                            if type(p) == sdn.OuterPin:
                                if so_drv == None:
                                    so_drv = p
                                cgso.wires[0].disconnect_pin(p)
                        # Reconnect proper sc_in pins and remove useless sc_in port/cables
                        cpsi = next(modules[mod].get_ports(cname))
                        if idx == 0:
                            si_calbe.wires[0].connect_pin(si_sik)
                            si_calbe.wires[0].connect_pin(ch_drv)
                            modules[mod].remove_cable(cgsi)
                            if has_si:
                                modules[mod].remove_port(cpsi)
                        else:
                            modules[mod].remove_port(cpsi)
                            cgsi.wires[0].connect_pin(ch_drv)
                        # Reconnect SO pins and remove useless sc_out port/cables
                        if idx == wid - 1 or has_si:
                            so_calbe.wires[0].connect_pin(so_drv)
                            so_calbe.wires[0].connect_pin(so_tail)
                        modules[mod].remove_cable(cgso)
                        cpso = next(modules[mod].get_ports(f"{gso[1]}_{idx}_{gso[2]}"))
                        modules[mod].remove_port(cpso)
                        if has_si:
                            break

                    # To remove useless sc_in/out in level 1 or higher modules if existing
                    if has_si == 0:
                        continue
                    for idx in range(1, wid):
                        cname = f"{gsi[1]}_{idx}_{gsi[2]}"
                        cgsi = next(modules[mod].get_cables(cname), None)
                        if cgsi != None:
                            modules[mod].remove_cable(cgsi)
                        cgso = next(modules[mod].get_cables(f"{gso[1]}_{idx}_{gso[2]}"), None)
                        if cgso != None:
                            modules[mod].remove_cable(cgso)
                        cpsi = next(modules[mod].get_ports(cname), None)
                        if cpsi != None:
                            modules[mod].remove_port(cpsi)
                        cpso = next(modules[mod].get_ports(f"{gso[1]}_{idx}_{gso[2]}"), None)
                        if cpso != None:
                            modules[mod].remove_port(cpso)
        # Remove level 0 sc_in index 0 rlated port
        for mod in remain_port:
            module = next(self.lib.get_definitions(mod), None)
            assert module != None
            module.remove_port(remain_port[mod])

    # parse clock net form makeup key xml
    def parse_clock_net(self, el, clk="clk"):
        lv = el.get("level")
        grid = el.get("grid")
        if "clocks" not in self.mkpkey:
            self.mkpkey["clocks"] = {}
        if lv not in self.mkpkey["clocks"]:
            self.mkpkey["clocks"][lv] = {}
        dic = self.mkpkey["clocks"][lv]
        if "module" not in dic:
            dic["module"] = {}
        if grid in dic["module"]:
            logging.error("!!! Error: duplicated xml elements -", grid)
        dic["module"][grid] = next(self.lib.get_definitions(grid), None)
        if dic["module"][grid] == None:
            logging.error(f"!!! Error: fail to find module {grid} in clock makeup xml file ")
            return
        clk_i = []
        for ln in el:
            if ln.tag == "signal":
                io = ln.get("io")
                idx = ln.get("id")
                pre = ln.get("prefix")
                suf = ln.get("suffix")
                wid = ln.get("width")
                if io == clk:
                    clk_i.append([idx, pre, suf, wid])  # dic["signal"][f"{grid}_si"]=(name,sub)
                else:
                    logging.warning(f"Warning: wrong io name in xml file - {io} {grid}")
        clk_i.sort(key=lambda x: int(x[0]))
        dic[f"{grid}_clki"] = clk_i

    # Add buffer to specific signals net
    def add_buffer_to_net(self, module_name, net_name, bufcell, buf_inst_name=None):
        if module_name is None or len(module_name) == 0 or len(net_name) == 0:
            return None
        module: sdn.Definition = next(self.lib.get_definitions(module_name))
        cable: sdn.Cable = next(module.get_cables(net_name), None)
        if cable is None:
            logging.error(f"!!!! Error, not found net {net_name} when add buffer")
            return None
        con_lst = []
        inp_lst = []
        buf_cable = module.create_cable(f"{net_name}_mkbuf_out", wires=0)
        for w in cable.wires:
            idx = w.index()
            buf = self.create_buffer(bufcell)
            buf_inst = module.create_child(
                name=f"{net_name if buf_inst_name==None else buf_inst_name}_mkbuf_{idx}",
                reference=buf,
            )
            buf_in = next(self.get_port_pins(buf_inst, "I"))
            buf_out = next(self.get_port_pins(buf_inst, "Z"))
            buf_cable.create_wire()
            buf_cable.wires[-1].connect_pin(buf_out)
            inp_lst.append([w, buf_in])
            # w.connect_pin(buf_in) # Don's disconnect or connect pin when iterate pins
            con_lst.append([])
            for p in w.pins:
                if type(p) == sdn.OuterPin:
                    con_lst[-1].append([w, buf_cable.wires[-1], p])
        for g in con_lst:
            for con in g:
                con[0].disconnect_pin(con[2])
                con[1].connect_pin(con[2])
        for g in inp_lst:
            g[0].connect_pin(g[1])

    # Insert clock buffer in tiles
    def insert_clock_buffer_in_tile(self, type, bufcell="CLKBUFV4_7TR40", buf_inst_name=None):
        for lv in self.mkpkey["clocks"]:
            modules = self.mkpkey["clocks"][lv]["module"]
            for mod in modules:
                dic = self.mkpkey["clocks"][lv][f"{mod}_clki"]
                for ck in dic:
                    wid = int(ck[3])
                    for i in range(wid):
                        net_name = f"{ck[1]}_{i}_{ck[2]}"
                        inst_name = f"clk{i}" if buf_inst_name == None else buf_inst_name
                        self.add_buffer_to_net(mod, net_name, bufcell, inst_name)

    # Insert net buffer in tiles
    def insert_net_buffer_in_tile(self, net_name, bufcell="CLKBUFV4_7TR40", buf_inst_name=None):
        for tile in self.modules:
            inst_name = f"{net_name}" if buf_inst_name == None else buf_inst_name
            self.add_buffer_to_net(tile, net_name, bufcell, inst_name)

    def create_top_level_output_ports(self, tile_instance_name, tile_port_pattern, top_port_name):
        """
        Create new output ports at top level (fpga_top) and connect them to tile instance's matching output ports
        Uses bus notation for multiple ports and exact port names for single ports
        Disconnects existing connections before creating new ones
        """
        try:
            # Step 1: Find the tile instance in fpga_core
            tile_instance = next(self.core.get_instances(tile_instance_name), None)
            if tile_instance is None:
                logging.error(f"Tile instance '{tile_instance_name}' not found in fpga_core")
                return 0

            # Step 2: Find all matching output ports in the tile instance
            matching_ports = []
            for port in tile_instance.reference.get_ports():
                if tile_port_pattern in port.name and port.direction is sdn.OUT:
                    matching_ports.append(port)

            if not matching_ports:
                #            logging.error(f"No output ports matching pattern '{tile_port_pattern}' found in instance '{tile_instance_name}'")
                return 0

            #        logging.info(f"Found {len(matching_ports)} matching output ports for pattern '{tile_port_pattern}'")

            # For single port, use exact name; for multiple ports, use bus notation
            if len(matching_ports) == 1:
                # Single port - use exact name
                return self._create_single_output_port(
                    tile_instance, matching_ports[0], top_port_name
                )
            else:
                # Multiple ports - use bus notation
                return self._create_bus_output_ports(tile_instance, matching_ports, top_port_name)

        except Exception as e:
            #         logging.error(f"Error creating top-level output ports: {str(e)}")
            return 0

    def _create_single_output_port(self, tile_instance, tile_port, top_port_name):
        """Create a single output port with exact name"""
        try:
            # Get the number of pins (bit width) from the tile port
            port_pins_count = len(tile_port.pins)
            logging.info(f"Port '{tile_port.name}' has {port_pins_count} pins")

            # Get the pins from the tile instance
            tile_pins = list(self.get_port_pins(tile_instance, tile_port.name))
            if not tile_pins:
                #           logging.error(f"Failed to get pins for port '{tile_port.name}' in instance '{tile_instance.name}'")
                return 0

            # Handle existing connections - disconnect wires from tile pins
            for tile_pin in tile_pins:
                if hasattr(tile_pin, "wire") and tile_pin.wire is not None:
                    wire = tile_pin.wire
                    cable = wire.cable
                    if cable is not None:
                        try:
                            cable.wires[wire.index()].disconnect_pin(tile_pin)
                        #                        logging.info(f"Disconnected existing connection for pin '{tile_port.name}'")
                        except Exception as e:
                            logging.warning(
                                f"Could not disconnect pin '{tile_port.name}': {str(e)}"
                            )

            # Create new output port in fpga_core with matching bit width
            core_output_port = self.core.create_port(
                top_port_name, pins=port_pins_count, direction=sdn.OUT  # Exact name as specified
            )

            # Create cable in fpga_core with same name as port
            core_cable = self.core.create_cable(
                top_port_name, wires=port_pins_count  # Same name as port
            )

            # Connect tile pins to core cable, then core cable to core port
            for j in range(port_pins_count):
                core_cable.wires[j].connect_pin(core_output_port.pins[j])
                core_cable.wires[j].connect_pin(tile_pins[j])

            # Find fpga_core instance in fpga_top
            core_instance = None
            for inst in self.top.get_instances():
                if inst.reference.name == self.core.name:
                    core_instance = inst
                    break

            if core_instance is None:
                #            logging.error("fpga_core instance not found in fpga_top")
                return 0

            # Get the core pins in fpga_top
            core_pins = list(self.get_port_pins(core_instance, top_port_name))
            if not core_pins or len(core_pins) != port_pins_count:
                #            logging.error(f"Failed to get pins for port '{top_port_name}' in fpga_core instance")
                return 0

            # Handle existing connections in fpga_top - disconnect wires from core pins
            for core_pin in core_pins:
                if hasattr(core_pin, "wire") and core_pin.wire is not None:
                    wire = core_pin.wire
                    cable = wire.cable
                    if cable is not None:
                        try:
                            cable.wires[wire.index()].disconnect_pin(core_pin)
                        #                        logging.info(f"Disconnected existing connection for core pin '{top_port_name}'")
                        except Exception as e:
                            logging.warning(
                                f"Could not disconnect core pin '{top_port_name}': {str(e)}"
                            )

            # Create new output port in fpga_top with matching bit width
            top_output_port = self.top.create_port(
                top_port_name, pins=port_pins_count, direction=sdn.OUT  # Exact name as specified
            )

            # Create cable in fpga_top with same name as port
            top_cable = self.top.create_cable(
                top_port_name, wires=port_pins_count  # Same name as port
            )

            # Connect core pins to top cable, then top cable to top port
            for j in range(port_pins_count):
                top_cable.wires[j].connect_pin(top_output_port.pins[j])
                top_cable.wires[j].connect_pin(core_pins[j])

            return 1

        except Exception as e:
            #        logging.error(f"Error creating single output port '{top_port_name}': {str(e)}")
            return 0

    def _create_bus_output_ports(self, tile_instance, matching_ports, top_port_name):
        """Create multiple output ports using bus notation"""
        try:
            created_ports = 0

            # First, collect all port information
            port_info_list = []
            total_width = 0

            for i, tile_port in enumerate(matching_ports):
                port_pins_count = len(tile_port.pins)
                port_info_list.append(
                    {
                        "index": i,
                        "port": tile_port,
                        "width": port_pins_count,
                        "start_bit": total_width,
                    }
                )
                total_width += port_pins_count

            # Create bus port in fpga_core
            core_bus_port = self.core.create_port(
                top_port_name, pins=total_width, direction=sdn.OUT  # Bus name
            )

            # Create bus cable in fpga_core
            core_bus_cable = self.core.create_cable(
                top_port_name, wires=total_width  # Same name as port
            )

            # Connect bus port to bus cable
            for j in range(total_width):
                core_bus_cable.wires[j].connect_pin(core_bus_port.pins[j])

            # Process each tile port and connect to appropriate bits in the bus
            for port_info in port_info_list:
                tile_port = port_info["port"]
                start_bit = port_info["start_bit"]
                width = port_info["width"]

                # Get the pins from the tile instance
                tile_pins = list(self.get_port_pins(tile_instance, tile_port.name))
                if not tile_pins:
                    #               logging.error(f"Failed to get pins for port '{tile_port.name}'")
                    continue

                # Handle existing connections - disconnect wires from tile pins
                for tile_pin in tile_pins:
                    if hasattr(tile_pin, "wire") and tile_pin.wire is not None:
                        wire = tile_pin.wire
                        cable = wire.cable
                        if cable is not None:
                            try:
                                cable.wires[wire.index()].disconnect_pin(tile_pin)
                            #                            logging.info(f"Disconnected existing connection for pin '{tile_port.name}'")
                            except Exception as e:
                                logging.warning(
                                    f"Could not disconnect pin '{tile_port.name}': {str(e)}"
                                )

                # Connect tile pins to appropriate bits in the bus
                for j in range(width):
                    core_bus_cable.wires[start_bit + j].connect_pin(tile_pins[j])

            #           logging.info(f"Connected tile port '{tile_port.name}' to bus bits [{start_bit + width - 1}:{start_bit}]")

            # Find fpga_core instance in fpga_top
            core_instance = None
            for inst in self.top.get_instances():
                if inst.reference.name == self.core.name:
                    core_instance = inst
                    break

            if core_instance is None:
                #          logging.error("fpga_core instance not found in fpga_top")
                return 0

            # Get the core pins in fpga_top
            core_pins = list(self.get_port_pins(core_instance, top_port_name))
            if not core_pins or len(core_pins) != total_width:
                #           logging.error(f"Failed to get pins for port '{top_port_name}' in fpga_core instance")
                return 0

            # Handle existing connections in fpga_top - disconnect wires from core pins
            for core_pin in core_pins:
                if hasattr(core_pin, "wire") and core_pin.wire is not None:
                    wire = core_pin.wire
                    cable = wire.cable
                    if cable is not None:
                        try:
                            cable.wires[wire.index()].disconnect_pin(core_pin)
                        #                        logging.info(f"Disconnected existing connection for core pin '{top_port_name}'")
                        except Exception as e:
                            logging.warning(
                                f"Could not disconnect core pin '{top_port_name}': {str(e)}"
                            )

            # Create bus port in fpga_top
            top_bus_port = self.top.create_port(
                top_port_name, pins=total_width, direction=sdn.OUT  # Bus name
            )

            # Create bus cable in fpga_top
            top_bus_cable = self.top.create_cable(
                top_port_name, wires=total_width  # Same name as port
            )

            # Connect core pins to top bus cable, then top bus cable to top bus port
            for j in range(total_width):
                top_bus_cable.wires[j].connect_pin(top_bus_port.pins[j])
                top_bus_cable.wires[j].connect_pin(core_pins[j])

            return 1

        except Exception as e:
            logging.error(f"Error creating bus output ports '{top_port_name}': {str(e)}")
            return 0

    def add_custom_ports_to_tiles(self, config):

        if not isinstance(config, list):
            logging.error("Invalid config format: expected list of dictionaries")
            return

        for tile_config in config:
            tile_name = tile_config.get("tile")
            ports_info = tile_config.get("ports")

            if not tile_name or not ports_info:
                logging.warning(f"Missing 'tile' or 'ports' in config entry: {tile_config}")
                continue

            module_def = next(self.lib.get_definitions(tile_name), None)
            if module_def is None:
                logging.warning(f"Module '{tile_name}' not found in netlist")
                continue

            for port_info in ports_info:
                name = port_info.get("name")
                width = port_info.get("width")
                direction = port_info.get("direction")

                if not name or not width or not direction:
                    logging.warning(f"Invalid port info: {port_info}")
                    continue

                if direction.lower() == "in":
                    dir_enum = sdn.IN
                elif direction.lower() == "out":
                    dir_enum = sdn.OUT
                elif direction.lower() == "inout":
                    dir_enum = sdn.INOUT
                else:
                    logging.warning(f"Unsupported direction: {direction}")
                    continue

                try:
                    port = module_def.create_port(name, pins=width, direction=dir_enum)
                    logging.info(
                        f"Added port '{name}' [{width} bits] to module '{tile_name}' (direction: {direction})"
                    )
                except Exception as e:
                    logging.warning(f"Failed to create port '{name}' in '{tile_name}': {e}")

    def connect_instance_port_to_top(self, instance_name, port_name, top_port_name):
        """
        Connect a specific instance port to a top-level port
        Creates a wire with the same name as the top-level port in the core module
        and connects the instance port to the top-level port through this wire.

        Args:
            instance_name (str): Name of the instance in fpga_core
            port_name (str): Name of the port on the instance
            top_port_name (str): Name of the top-level port in fpga_top
        """
        try:
            # Find the instance in fpga_core
            instance = next(self.core.get_instances(instance_name), None)
            if instance is None:
                logging.error(f"Instance '{instance_name}' not found in fpga_core")
                return False

            # Find the port on the instance
            port_pins = list(self.get_port_pins(instance, port_name))
            if not port_pins:
                logging.error(f"Port '{port_name}' not found on instance '{instance_name}'")
                return False

            # Get bit width of the port
            port_width = len(port_pins)

            # Find fpga_core instance in fpga_top
            core_instance = None
            for inst in self.top.get_instances():
                if inst.reference.name == self.core.name:
                    core_instance = inst
                    break

            if core_instance is None:
                logging.error("fpga_core instance not found in fpga_top")
                return False

            # Get the core pins in fpga_top that correspond to the top-level port
            core_pins = list(self.get_port_pins(core_instance, top_port_name))
            if not core_pins or len(core_pins) != port_width:
                logging.error(
                    f"Top-level port '{top_port_name}' not found or has different width in fpga_top"
                )
                return False

            # Create a wire in fpga_core with the same name as the top-level port
            # Check if cable already exists
            core_cable = next(self.core.get_cables(top_port_name), None)
            if core_cable is None:
                core_cable = self.core.create_cable(top_port_name, wires=port_width)
            else:
                # If cable exists, make sure it has the right width
                if len(core_cable.wires) < port_width:
                    for _ in range(port_width - len(core_cable.wires)):
                        core_cable.create_wire()

            # Connect the instance port pins to the core cable wires
            for i in range(port_width):
                # Disconnect any existing connections on the instance pins
                if hasattr(port_pins[i], "wire") and port_pins[i].wire is not None:
                    wire = port_pins[i].wire
                    cable = wire.cable
                    if cable is not None:
                        try:
                            cable.wires[wire.index()].disconnect_pin(port_pins[i])
                        except:
                            pass  # Ignore if already disconnected

                # Disconnect any existing connections on the core pins
                if hasattr(core_pins[i], "wire") and core_pins[i].wire is not None:
                    wire = core_pins[i].wire
                    cable = wire.cable
                    if cable is not None:
                        try:
                            cable.wires[wire.index()].disconnect_pin(core_pins[i])
                        except:
                            pass  # Ignore if already disconnected

                # Connect instance pins to core cable
                core_cable.wires[i].connect_pin(port_pins[i])
                core_cable.wires[i].connect_pin(core_pins[i])

            logging.info(
                f"Successfully connected instance '{instance_name}' port '{port_name}' to top-level port '{top_port_name}'"
            )
            return True

        except Exception as e:
            logging.error(f"Error connecting instance port to top: {str(e)}")
            return False


#####################################################################
# Core function to a complete netlist makeup flow
#####################################################################
def run_netlist_makeup(
    fabric_netlist_file,
    fabric_key_file,
    ref_module_file,
    module_name_file,
    bram_makeup_key_file,
    undriven_makeup_key_file,
    bscan_makeup_key_file,
    clkn_makeup_key_file,
    output_dir,
    feedthru_cell,
    buf_cell,
    clkbuf_cell,
    run_specific_sig_ft,
    run_bram_makeup,
    run_boundaryscan_makeup,
    run_remove_undriven_ports,
    run_clk_buffers_add,
    run_specific_sig_output,
    specific_sig_out_info,
    run_add_ports,
    inst_port_info,
    run_connect_instance_port_to_top,
    specific_port_to_top_info,
):
    logging.info("\nRuning pythons script netlist_makeup.py ...")
    logging.info("Parse fabric netlists ...")
    netmp = netlist_makeup(fabric_netlist_file)
    logging.info("Done")

    logging.info("Parse fabric key and related files ...")
    netmp.parse_fabric_key(
        fabric_key_file=fabric_key_file,
        ref_module_file=ref_module_file,
        module_name_file=module_name_file,
    )

    logging.info("Done")
    if run_specific_sig_ft:
        logging.info("Add feedthroughs and buffers ...")
        scan = netmp.parse_makeup_key(bscan_makeup_key_file)

        logging.info("Add clock/reset/mem_init_clk_i buffers in  tiles ...")
        netmp.insert_net_buffer_in_tile("prog_clk", feedthru_cell)
        netmp.insert_net_buffer_in_tile("prog_reset", feedthru_cell)
        logging.info("Done")

        logging.info("Create feedthrough for configuration signals ...")
        tile_list = [key for key in netmp.modules]
        signal_list = ["prog_reset", "prog_clk", "config_enable"]
        for tile in tile_list:
            for signal in signal_list:
                netmp.add_feedthrough(tile, signal, feedthru_cell)
        for signal in signal_list:
            netmp.reorder_feedthrough_signal(signal)
    if run_bram_makeup:
        if not (bram_makeup_key_file == None or bram_makeup_key_file == ""):
            logging.info("Create feedthrough for memory initialization signals ...")
            bram = netmp.parse_makeup_key(bram_makeup_key_file)
            bram_tile = ["grid_ram_tile_main", "grid_ram_tile_sub"]
            for m in bram_tile:
                netmp.add_buffer_to_net(m, "mem_init_clk_i", "CLKBUFV4_7TR40", "mem_init_clk_i")
            ram_list = [k for k in netmp.mkpkey[f"{bram}_module"]]
            ram_signal = [k[0] for k in netmp.mkpkey[f"{bram}_signal"]]
            for r in ram_list:
                for signal in ram_signal:
                    netmp.add_feedthrough(r, signal, buf_cell)
            for signal in ram_signal:
                netmp.reorder_feedthrough_signal(signal, netmp.mkpkey[f"{bram}_region"])
            logging.info("Set bram index/address for initialization ...")
            netmp.set_bram_mem_index(bram)  # set bram index/address for initialization

    # clk_out_list = [["clk0", 2], ["clk2", 2]]
    #  netmp.create_clock_output(clk_out_list, feedthru_cell)

    if run_boundaryscan_makeup:
        logging.info("Create feedthrough for bscan global signals ...")
        scan_list = [k for k in netmp.mkpkey[f"{scan}_module"]]
        scan_signal = [k[0] for k in netmp.mkpkey[f"{scan}_signal"]]
        for s in scan_list:
            for signal in scan_signal:
                netmp.add_feedthrough(s, signal, buf_cell)
        for signal in scan_signal:
            netmp.reorder_feedthrough_signal(signal, netmp.mkpkey[f"{scan}_region"])
        logging.info("Reorder scan chain SI and SO ...")
        netmp.reorder_scan_chain_signal(scan)
        logging.info("Done")

    if run_remove_undriven_ports:
        if not (undriven_makeup_key_file == None or undriven_makeup_key_file == ""):
            logging.info("Remove undriven ports from specific tiles ...")
            #     undriven = netmp.parse_makeup_key(undriven_makeup_key_file)
            #    netmp.remove_undriven_port(undriven)
            logging.info("Done")

    if run_clk_buffers_add:
        logging.info("Add clock buffers to specific tiles ...")
        #  clkn = netmp.parse_makeup_key(clkn_makeup_key_file)
        #  netmp.insert_clock_buffer_in_tile(clkn, clkbuf_cell)
        logging.info("Done")

    target_ports_list = []

    if run_add_ports:
        #      adjustd_ports = []
        status = netmp.add_custom_ports_to_tiles(inst_port_info)
        #          adjusted_ports.append(one_design[0])
        port_name = set()
        for tile in inst_port_info:
            if "ports" in tile and isinstance(tile["ports"], list):
                for port in tile["ports"]:
                    if isinstance(port, dict) and "name" in port:
                        port_name.add(port["name"])
        target_ports_list = target_ports_list + sorted(list(port_name))

    if run_specific_sig_output:
        for each_info in specific_sig_out_info:
            netmp.create_top_level_output_ports(
                each_info["tile_name"], each_info["ports_name"], each_info["target_top_ports"]
            )
            target_ports_list.append(each_info["target_top_ports"])
    # netmp.create_top_level_output_ports("tile_3__1_", "rdata_o", "ccb_rdata_o")

    if run_connect_instance_port_to_top:
        for each_info in specific_port_to_top_info:
            netmp.connect_instance_port_to_top(
                each_info["tile_name"], each_info["ports_name"], each_info["target_top_ports"]
            )

    # for k in netmp.mkpkey: print(k,netmp.mkpkey[k])
    logging.info(f"Saving netlist to {output_dir} ...\n")
    netmp.save_netlist(output_dir)

    # replace the \<const0> generated by spydrnet
    output_files = glob(os.path.join(output_dir, "tile/*.v"))
    output_files.append(output_dir + "fpga_core.v")
    output_files.append(output_dir + "fpga_top.v")

    for each_netlist in output_files:
        with open(each_netlist, "r") as file:
            lines = file.readlines()
        modified_lines1 = [re.sub(r"\\<const0\>", "1'b0", line) for line in lines]
        modified_lines2 = [
            modified_line2
            for modified_line2 in modified_lines1
            if modified_line2.strip() != "    wire 1'b0 ;\n".strip()
        ]

        # Change the byte order from big-endian to little-endian in output RTL
        modified_lines3 = []
        for line in modified_lines2:
            #         if "output" in line and target_top_ports in line and re.search(r'\[(\d+):(\d+)\]', line):
            #             line = re.sub(r'\[(\d+):(\d+)\]', lambda m: f'[{m.group(2)}:{m.group(1)}]', line)
            #         if "wire" in line and target_top_ports in line and re.search(r'\[(\d+):(\d+)\]', line):
            #             line = re.sub(r'\[(\d+):(\d+)\]', lambda m: f'[{m.group(2)}:{m.group(1)}]', line)
            #         if target_top_ports + "(" + target_top_ports in line and re.search(r'\[(\d+):(\d+)\]', line):
            #             line = re.sub(r'\[(\d+):(\d+)\]', lambda m: f'[{m.group(2)}:{m.group(1)}]', line)
            for one_port in target_ports_list:
                if "wire" in line and one_port in line and re.search(r"\[(\d+):(\d+)\]", line):
                    line = re.sub(
                        r"\[(\d+):(\d+)\]", lambda m: f"[{m.group(2)}:{m.group(1)}]", line
                    )
                if one_port + "(" + one_port in line and re.search(r"\[(\d+):(\d+)\]", line):
                    line = re.sub(
                        r"\[(\d+):(\d+)\]", lambda m: f"[{m.group(2)}:{m.group(1)}]", line
                    )
                if (
                    ("output" in line or "input" in line)
                    and one_port in line
                    and re.search(r"\[(\d+):(\d+)\]", line)
                ):
                    line = re.sub(
                        r"\[(\d+):(\d+)\]", lambda m: f"[{m.group(2)}:{m.group(1)}]", line
                    )

            modified_lines3.append(line)

        with open(each_netlist, "w") as file:
            file.writelines(modified_lines3)


#####################################################################
# Main function
#####################################################################
if __name__ == "__main__":
    # Execute when the module is not initialized from an import statement
    # Parse the options and apply sanity checks
    parser = argparse.ArgumentParser(
        description="Run netlist make-up to add dedicated feedthrough signals"
    )
    parser.add_argument(
        "--fabric_netlist",
        required=True,
        help="The Verilog file fabric_netlist.v which contains a list of all the FPGA netlists",
    )
    parser.add_argument(
        "--fabric_key",
        required=True,
        help="The fabric key file corresponding to the fabric netlists",
    )
    parser.add_argument(
        "--reference_module_names",
        required=True,
        help="The reference module name file corresponding to the fabric netlists",
    )
    parser.add_argument(
        "--module_names",
        required=True,
        help="The final module name file corresponding to the fabric netlists",
    )
    parser.add_argument(
        "--bram_makeup_key_file",
        required=False,
        help="The key file for connecting BRAM signals",
    )
    parser.add_argument(
        "--undriven_makeup_key_file",
        required=False,
        help="The key file for handling undriven nets",
    )
    parser.add_argument(
        "--bscan_makeup_key_file",
        required=True,
        help="The key file for handling boundary scan nets",
    )
    parser.add_argument(
        "--clkn_makeup_key_file",
        required=True,
        help="The key file for handling clock buffers",
    )
    parser.add_argument(
        "--output_dir",
        required=True,
        help="The output directory for the netlists after performing make-up",
    )
    parser.add_argument(
        "--feedthru_cell",
        default="CLKBUFV4_7TR40",
        help="The name of standard cell to be used when inserting feedthrough",
    )
    parser.add_argument(
        "--buf_cell",
        default="BUFV4_7TR40",
        help="The name of standard cell to be used when inserting buffers",
    )
    parser.add_argument(
        "--clkbuf_cell",
        default="CLKBUFV4_7TR40",
        help="The name of standard cell to be used when inserting clock buffers",
    )
    parser.add_argument(
        "--config_file",
        default=None,
        help="The YAML config file determinate which function will run",
    )

    # Log runtime
    start_time = time.time()
    args = parser.parse_args()

    makeup_setting = netlist_makeup_config.NetlistMakeupConfig()

    if args.config_file:
        makeup_setting.load(args.config_file)

        run_specific_sig_ft_setting = makeup_setting.run_specific_sig_ft()
        run_bram_makeup_setting = makeup_setting.run_bram_makeup()
        run_boundaryscan_makeup_setting = makeup_setting.run_boundaryscan_makeup()
        run_remove_undriven_ports_setting = makeup_setting.run_remove_undriven_ports()
        run_clk_buffers_add_setting = makeup_setting.run_clk_buffers_add()
        run_specific_sig_output_setting = makeup_setting.run_specific_sig_output()
        specific_sig_out_info = makeup_setting.specific_sig_out_info()
        run_add_ports = makeup_setting.run_add_ports()
        inst_port_info = makeup_setting.inst_port_info()
        run_connect_instance_port_to_top = makeup_setting.run_connect_instance_port_to_top()
        specific_port_to_top_info = makeup_setting.specific_port_to_top_info()

    else:
        logging.info("No config file provided. Using default settings.")
        run_specific_sig_ft_setting = False
        run_bram_makeup_setting = False
        run_boundaryscan_makeup_setting = False
        run_remove_undriven_ports_setting = False
        run_clk_buffers_add_setting = False
        run_specific_sig_output_setting = False
        specific_sig_out_info = ""
        run_add_ports = False
        inst_port_info = ""
        run_connect_instance_port_to_top = False
        specific_port_to_top_info = ""

    num_errors = 0
    run_netlist_makeup(
        args.fabric_netlist,
        args.fabric_key,
        args.reference_module_names,
        args.module_names,
        args.bram_makeup_key_file,
        args.undriven_makeup_key_file,
        args.bscan_makeup_key_file,
        args.clkn_makeup_key_file,
        args.output_dir,
        args.feedthru_cell,
        args.buf_cell,
        args.clkbuf_cell,
        run_specific_sig_ft_setting,
        run_bram_makeup_setting,
        run_boundaryscan_makeup_setting,
        run_remove_undriven_ports_setting,
        run_clk_buffers_add_setting,
        run_specific_sig_output_setting,
        specific_sig_out_info,
        run_add_ports,
        inst_port_info,
        run_connect_instance_port_to_top,
        specific_port_to_top_info,
    )

    end_time = time.time()
    time_diff = timedelta(seconds=(end_time - start_time))

    time_str = "Running netlist makeup flow took " + str(time_diff)
    logging.info(time_str)

    if num_errors == 0:
        exit(error_codes["SUCCESS"])
    else:
        exit(error_codes["ERROR"])
