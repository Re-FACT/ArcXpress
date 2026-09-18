import os
import spydrnet as sdn
from glob import glob
import tempfile
import xml.etree.ElementTree as ET
import copy
import argparse
import logging
import ccff_buf_config

"""
self.modules data structure
- "cfgmem":{}
 - <cfg group mem moduel name>:{}
   - "chain":[mem pieces in sequence]
   - "group":[mem pieces separated in group with proper fanout]
   - "gbuf":{<pin name>:[list of buffers]}
   - "buf_<drv>":{"level 1 buffer name":[list of level 2 child nodes]} 
   - "segment":[separated mem bits cell chain, include both mem bit and data chain buffers]
   - "tcl_buf_<drv>":[tcl cmds to insert buffers]
   - "tcl_del":[tcl cmds to insert delay cell for clean hold time]

- "tile"
 - <tile name>:{}
  - <grid name>:<config group mem name in this grid>
  - "chain":[grid name in sequence] 


"""

#####################################################################
# Error codes
#####################################################################
error_codes = {"SUCCESS": 0, "ERROR": 1, "FILE_ERROR": 2}

#####################################################################
# Initialize logger
#####################################################################
logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.INFO)


class ccff_makeup:
    def __init__(self, input_files="", topname="fpga_top", corename="fpga_core"):
        self.lib = self.open_netlist(input_files)
        self.top = next(self.lib.get_definitions(topname))
        self.core = next(self.lib.get_definitions(corename))
        self.modules = {}

        self.score = -1
        self.best = -1
        self.balance = 0
        self.success = 0

        self.parse_fpga_tile()

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

    def parse_fpga_tile(self, head="ccff_head", tail="ccff_tail"):
        insts = self.core.children
        tile_dict = {}
        mem_dict = {}
        for inst in insts:
            mod = inst.reference.name
            if mod.find("tile") >= 0 and mod not in tile_dict:
                grid_dict = {}
                grid_chain = []
                hcable = next(inst.reference.get_cables(head), None)
                if hcable == None:
                    continue
                pins = hcable.wires[0].pins
                loop = 0
                drv = None
                while loop < 9999:
                    for p in pins:
                        if type(p) == sdn.OuterPin and p != drv:
                            drv = next(self.get_port_pins(p.instance, tail), None)
                            if drv == None:
                                logging.error(
                                    f"!!! Error: fail to find port {tail} in {p.instance.name}"
                                )
                                return
                            grid_chain.append(p.instance)
                            break
                        elif type(p) == sdn.InnerPin and p.port.name == tail:
                            loop = 99999
                    pins = drv.wire.pins
                    loop += 1

                tmp_chain = []
                for grid in grid_chain:
                    gn = grid.reference.name
                    tmp_chain.append(gn)
                    if gn not in grid_dict:
                        mems = grid.reference.children
                        mname = ""
                        for m in mems:

                            if m.reference.name.find("config_group_mem") >= 0:
                                mname = m.reference.name

                                for child_module in m.reference.children:
                                    child_module_ref = child_module.reference.name
                                child_num = len(m.reference.children)
                                break
                        if mname == "":
                            logging.error(
                                "!!!Error: can't find proper config_group_mem in grid -", gn
                            )
                            exit()
                        grid_dict[gn] = mname
                        if mname not in mem_dict:
                            mem_dict[mname] = {
                                "segment": [],
                                "clkbuf": {},
                                "rstbuf": {},
                                "chain": [],
                                "gbuf": {},
                            }
                tile_dict[mod] = {"chain": tmp_chain, "grid": grid_dict}
        self.modules["tile"] = tile_dict
        self.modules["cfgmem"] = mem_dict

    def get_port_pins(self, inst, ports):
        """
        Returns all the outerpins of this port on this instance
        args:
                port (Port, str): Port of the instance
        """
        if isinstance(ports, str):
            ports = inst.reference.get_ports(ports)
        return (inst.pins[p] for port in ports for p in port.pins)

    # Check config_group_mem module and return DFF counts in this module
    # module_def: sdn.definition of config_group_men in netlist
    # mem_port_name: port name that connect to mem bit output
    def get_config_mem_bitcnt(self, module_def, mem_port_name="mem_out"):
        cable = next(module_def.get_cables(mem_port_name), None)
        if cable == None:
            return -1
        return len(cable.wires)

    # Parse config_group_mem ccff data chain sequence
    # module_name: module name of config_group_men in netlist
    # head: port name of ccff_head
    # tail: port name of ccff_tail
    # return: ccff data chain [["mem instance",mem instrance, mem bit counts],...]
    def parse_ccff_data_chain(self, module_name, head="ccff_head", tail="ccff_tail"):
        data_chain = []
        mod = next(self.lib.get_definitions(module_name), None)
        if mod == None:
            return []
        hcable = next(mod.get_cables(head), None)
        if hcable == None:
            return []
        pins = hcable.wires[0].pins
        loop = 0
        drv = None
        while loop < 9999:
            for p in pins:
                if type(p) == sdn.OuterPin and p != drv:
                    drv = next(self.get_port_pins(p.instance, tail), None)
                    if drv == None:
                        logging.error(f"!!! Error: fail to find port {tail} in {p.instance.name}")
                        return []
                    bitcnt = self.get_config_mem_bitcnt(p.instance.reference)
                    data_chain.append([p.instance.name, p.instance.reference.name, bitcnt])
                    break
                elif type(p) == sdn.InnerPin and p.port.name == tail:
                    loop = 99999
            pins = drv.wire.pins
            loop += 1
        if module_name in self.modules["cfgmem"]:
            self.modules["cfgmem"][module_name]["chain"] = data_chain
        else:
            logging.error(f"!!! Error, config group mem {module_name} is not found in self.modules")
        return data_chain

    # Group ccff chain DFF cells by data shift sequence
    # data_chain: the list return from parse_ccff_data_chain
    # reverse: True - mem bit sequence start from ccff tail to ccff head, False - normal sequence
    # return chain_group: groupped data chain by sequence
    def group_ccff_chain_segment(self, module_name, fanout, reverse=False):
        data_chain = self.modules["cfgmem"][module_name]["chain"]
        chain_group = []
        N = len(data_chain)
        bit = 0
        grp = [
            0,
        ]
        for i in range(N):  # break chain into groups/segments based on fanout setting
            c = data_chain[N - 1 - i] if reverse else data_chain[i]
            # print(i,bit,c[2],bit+c[2])
            if bit + c[2] >= fanout or i == N - 1:
                if bit > 0 and c[2] > fanout:
                    if grp != [0]:
                        chain_group.append(grp)
                    grp = [
                        0,
                    ]

                if (bit > 0 or i == N - 1) and (c[2] < fanout):
                    grp[0] = (
                        (bit + c[2])
                        if ((i == N - 1 and c[2] < fanout) or c[2] + bit == fanout)
                        else bit
                    )
                    if (i == N - 1 and c[2] < fanout) or c[2] + bit == fanout:
                        grp.append(c)
                    if grp != [0]:
                        chain_group.append(grp)
                    grp = (
                        [c[2], c]
                        if bit + c[2] > fanout
                        else [
                            0,
                        ]
                    )
                    bit = c[2] if bit + c[2] > fanout else 0
                if c[2] == fanout:
                    if grp != [0]:
                        grp[0] = bit
                        chain_group.append(grp)
                    chain_group.append([c[2], c])
                    grp = [
                        0,
                    ]
                    bit = 0
                if c[2] > fanout:
                    #                    chain_group = []
                    #                    print(f"The value of c is {c}")
                    if c[2] > fanout:
                        logging.warning(
                            f"Warning! - memory bit in instance {c[0]} is bigger than max fanout {fanout},so this instance will be devided manually\n"
                        )
                    bit_num = c[2]
                    grp_num = []
                    while bit_num > 0:
                        if bit_num - fanout >= 0:
                            grp_num.append(fanout)
                            bit_num -= fanout
                        else:
                            grp_num.append(bit_num)
                            bit_num = 0
                    for i in range(len(grp_num)):
                        new_grp = []
                        new_grp.append(grp_num[i])
                        semi_grp = []
                        str_inst = c[0]
                        str_ref = (
                            c[1]
                            + "|divided-manually_|"
                            + str(sum(grp_num[:i]))
                            + "|"
                            + str(sum(grp_num[: i + 1]))
                        )
                        semi_grp.append(str_inst)
                        semi_grp.append(str_ref)
                        semi_grp.append(grp_num[i])
                        new_grp.append(semi_grp)
                        chain_group.append(new_grp)

            else:
                bit += c[2]
                grp.append(c)
        #        print(f"The value of chain_group is {chain_group}")
        return chain_group

    def group_ccff_chain_cell(self, module_name, fanout=33, reverse=False):
        chain_group = self.group_ccff_chain_segment(module_name, fanout, reverse)
        # Re-arrange some small segments to balance buffer loading as possible
        cpx = [-4, -3, -2]  # number in sequence of increasing
        avx = [-1, -2]  # number in sequence of decreasing
        N = len(chain_group)
        for i in range(N):
            g = chain_group
            # dis = max(abs(g[i-1][0]-g[i][0]) if i-1>=0 else 0, abs(g[i+1][0]-g[i][0]) if i+1<N else 0)
            dis = g[i - 1][0] - g[i][0] if i - 1 >= 0 else 0
            avg = 0
            p = 0
            for cp in cpx:  # calcuate moving average
                if i + cp >= 0 and i + cp < N:
                    avg += (
                        abs(g[i + cp][0] - g[i + cp + 1][0])
                        if cp < 0
                        else abs(g[i + cp][0] - g[i + cp - 1][0])
                    )
                    p += 1
            avg = avg / p if p > 0 else 0
            if avg < dis:
                # print(i,avg,dis,chain_group[i])
                # tmp_chain = []+chain_group[i][1:]
                tmp_chain = [chain_group[i]]
                tmp_bit = chain_group[i][0]
                tmp_avx = []
                for v in avx:  # mix segment i with avx and re-arrange group
                    if i + v >= 0 and i + v < N:
                        if len(chain_group[i + v]) == 2:
                            break  # this segment is not separatable
                        # tmp_chain =  chain_group[i+v][1:] + tmp_chain
                        tmp_chain.insert(0, chain_group[i + v])
                        tmp_bit += chain_group[i + v][0]
                        tmp_avx.append(v)
                if tmp_bit > fanout:
                    self.balance_chain_segment(tmp_chain)
        self.modules["cfgmem"][module_name]["group"] = chain_group
        #        print(f"The value of new chain_group is {chain_group}")
        return chain_group

    # GBUFFD1BWP30P140HVT - W0.42, 32 fanout delay 70~100ps
    # BUFFD1BWP30P140HVT - W0.42, 32 fanout delay 70~100ps
    def tcl_insert_buffer_by_chain_group(
        self, module_name, drv, sink, buf_type="CLKBUFV4_7TR40", fanout=12, balance_buffer=True
    ):

        seg_lst = []  # level 2 mem bits list with buffer in tail
        buf_lv2 = []
        buf_dict = {}  # {"level 1 buffer name":[list of level 2 child nodes]}
        # buf_dict = {"lvs2":2,"lv1":{"list":[]}}
        head_lv1 = []
        head_lv2 = []
        tcl_buf = []
        chain_group = self.modules["cfgmem"][module_name]["group"]
        N = len(chain_group)
        fn = int(N / (int(N / fanout) + 1)) + 1
        clean = f'clean_buffer_tree -from "{drv}*"'
        tcl_buf.append(clean)
        fc = 0
        branch = ""
        eco = ""
        # tmp = []
        for x in range(N):
            manual = 0
            g = chain_group[x]
            pins = ""
            if len(g) == 1:
                break
            if fc == 0:
                head_lv1.append(g[1])
            head_lv2.append(g[1])
            tmp_lst = []
            for i in range(1, len(g)):
                s = g[i]
                pins += f' "{s[0]}/*/{sink}"'
                # tmp_lst.append(s)
            bn = f"{s[0]}_{sink.lower()}_buf2"

            if "manual" in s[1]:
                manual = 1
                pins = ""
                inst_name = s[0]
                bn = inst_name + str(x) + "_" + sink.lower() + "_buf2"
                num1 = (s[1].split("|"))[2]
                num2 = (s[1].split("|"))[3]
                for i in range(int(num1), int(num2)):
                    pin = f' "{inst_name}/*_{str(i)}_*/{str(sink)}"'
                    pins = pins + pin
            tmp_lst.append([bn, buf_type, 1])
            seg_lst.append(tmp_lst)
            buf_lv2.append(bn)  # TBD - may adjust buffer name
            cmd = (
                f"insert_buffer -new_cell_names {bn} "
                + "[get_pins {"
                + pins
                + "}]"
                + f" [get_lib_cells */{buf_type}]"
            )
            cmd = (
                cmd
                if balance_buffer
                else f"insert_buffer -new_cell_names {bn} "
                + "[get_pins {"
                + eco
                + pins
                + "}]"
                + f" [get_lib_cells */{buf_type}]"
            )
            eco = f'"{bn}/I"'
            # tmp.append(f"{s[0]}_{sink.lower()}_buf2")

            tcl_buf.append(cmd)
            branch += f' "{bn}/I"'
            fc += 1
            if (fc == fn or x == N - 1) and balance_buffer:
                fn_lv1 = f"{s[0]}_{sink.lower()}_buf1"
                if manual == 1:
                    fn_lv1 = inst_name + "_" + sink.lower() + "_buf1"
                brs = (
                    f"insert_buffer -new_cell_names {fn_lv1} "
                    + "[get_pins {"
                    + branch
                    + "}]"
                    + f" [get_lib_cells */{buf_type}]"
                )
                tcl_buf.append(brs)
                buf_dict[fn_lv1] = buf_lv2
                buf_lv2 = []
                branch = ""
                # tmp = []
                fc = 0
        self.modules["cfgmem"][module_name][f"buf_{drv}"] = buf_dict
        self.modules["cfgmem"][module_name][f"tcl_buf_{drv}"] = tcl_buf
        self.modules["cfgmem"][module_name]["gbuf"][sink] = seg_lst
        # for s in seg_lst: print(s)
        # for s in buf_dict: print(s,buf_dict[s],"---------------------\n")
        return [module_name, [head_lv1, head_lv2]]

    def tcl_insert_buffer_to_chain_group_head(
        self, groups, pin="EFPGA_CCFF_0_*/D", buf_type="BUFV1_7TH40", del_lv=1
    ):
        tcl_del = []
        module_name = groups[0]

        seg_lst = []
        gbuf_dict = self.modules["cfgmem"][module_name]["gbuf"]
        chain_group = self.modules["cfgmem"][module_name]["group"]
        #        print(f"The value of chain_group is {chain_group}")
        #        print(f"The value of groups is {groups}")
        for i in range(len(groups[1])):
            #            print(f"The value of i is {i}")
            grp = groups[1][i]
            #            print(f"The value of grp is {grp}")
            pins = ""
            for g in grp:
                #                print(f"The value of g is {g}")
                pins = f' "{g[0]}/{pin}"'

                bn = f"{g[0]}_del{i+1}"

                if "manual" in g[1]:
                    first_num = (g[1].split("|"))[2]
                    pins = f' "{g[0]}/EFPGA_CCFF_{first_num}_*/D"'
                    bn = f"{g[0]}_del{i+1}"
                    if i == 1:
                        position = grp.index(g)
                        bn = f"{g[0]}_del{i+1}_{position}"

                #    print(f"The value of pins is {pins}")
                #    print(f"The value of bn is {bn}")
                cmd = (
                    f"insert_buffer -new_cell_names {bn} -no_of_cells {del_lv} "
                    + "[get_pins {"
                    + pins
                    + "}]"
                    + f" [get_lib_cells */{buf_type}]"
                )
                # if self.fp_buf !=None: self.fp_buf.write(cmd+"\n") #print(cmd)
                tcl_del.append(cmd)
                if (
                    i == len(groups[1]) - 1
                ):  # Attention: Need adjust when buffer levels are deeper than 2
                    #                    print(grp)
                    #                    print("___________")
                    #                    print(chain_group)
                    assert len(grp) == len(
                        chain_group
                    ), "Should insert buffer to all segment head by default!"
                    bfn = f"{bn}"
                    #  print(f"The value of bn is {bn}")
                    tmp = []
                    for h in groups[1][0]:
                        #                        print(f"The value of h is {h}")
                        #                        print(f"The value of g is {g}")
                        if h[0] == g[0]:
                            tmp.append(f"{g[0]}_del{0+1}")  # TBD - may adjust buffer name
                        #                            print("YES!,h[0]=g[0]")
                        #                        print(f"The value of h is {h}")
                        #                        print(f"The value of g is {g}")
                        if ("manual" in h[1] and "|" in g[1]) and ((g[1].split("|"))[2] != "0"):
                            #       print((g[1].split("|"))[2])
                            #       print("YES!")
                            tmp = tmp[1:]
                    for lv in range(del_lv):
                        tmp.append(
                            bfn if lv == 0 else f"{bfn}_{lv-1}"
                        )  # TBD - may adjust buffer name
                    #   print(f"The value of tmp is {tmp}")
                    seg_lst.append(tmp)

        for i in range(len(chain_group)):
            grp = chain_group[i]
            #   print(f"The value of grp is {grp}")
            N = len(grp)
            tmp = seg_lst[i]
            for j in range(1, N):
                #    print(f"The value of j is {j}")
                s = grp[j]
                p = ""
                #    print(f"The value of s is {s}")
                for k in range(s[2]):
                    #     print(f"The value of k is {k}")
                    pn = pin.replace("_0_", f"_{k}_")
                    # p = f' "{s[0]}/{pn}"'
                    p = f"{s[0]}/{pn}"  # if s[0][-2].isdigit() else f'{s[0]}_{pn}'
                    bn = f"{s[0]}_del3_{k}"
                    if "manual" in s[1]:
                        first_num = (s[1].split("|"))[2]
                        k = int(first_num) + k
                        pn = pin.replace("_0_", f"_{k}_")
                        p = f"{s[0]}/{pn}"
                        bn = f"{s[0]}_del3_{k}"
                    cmd = (
                        f"insert_buffer -new_cell_names {bn} "
                        + "[get_pins {"
                        + p
                        + "}]"
                        + f" [get_lib_cells */{buf_type}]"
                    )
                    # if self.fp_buf !=None: self.fp_buf.write(cmd+"\n") #print(cmd)
                    tcl_del.append(cmd)
                    tmp.append(bn)  # TBD - may adjust buffer name
                    p = p.replace("/D", "")  # TBD - may adjust
                    tmp.append(p.replace("/", "_"))  # TBD - may adjust
            for k1 in gbuf_dict:
                tmp.append(gbuf_dict[k1][i][0][0])
            # tmp.append(mem_lst[i][-1][0])
        self.modules["cfgmem"][module_name]["segment"] = seg_lst
        self.modules["cfgmem"][module_name]["tcl_del"] = tcl_del
        # for s in self.modules["cfgmem"][module_name]["segment"]: print(s)

        # If a segment chain have big variation of segment bit number, try to break down segments and reorganize them with balanced bit number
        # params:
        # chains - reference of list of segment chain, will change segment chains
        # fanout - allow max bits in a small group of chain segment

    def balance_chain_segment(self, chains, fanout=33):
        self.score = -1
        self.best = -1
        self.balance = 0
        self.success = 0
        tmp_chain = copy.deepcopy(chains)
        self.move_chain_segment(tmp_chain, fanout)
        if self.best >= 0 and self.success > 0:
            self.score = -1
            self.balance = 1
            self.move_chain_segment(chains, fanout)

            # Move a small segment step by step and calculate score, find a solution to have best score
            # Score is assessed by average of bit number variations
            # params:
            # chains - reference of list of segment chain, will change segment chains
            # fanout - allow max bits in a small group of chain segment

    def move_chain_segment(self, chains, fanout=33):
        ret = False
        N = len(chains)
        if N < 2:
            return ret
        avg = 0
        pt = 0
        for i in range(1, N):
            avg += abs(chains[i][0] - chains[i - 1][0])
            if abs(chains[i][0] - chains[i - 1][0]) > 0:
                pt += 1
        avg = avg / pt if pt > 0 else avg
        self.score = avg if self.score < 0 or avg < self.score else self.score
        if self.balance == 0:
            self.best = self.score if self.best < 0 or self.score < self.best else self.best
        if self.score < avg:
            return ret
        # try to move one segment
        for i in range(N - 1):
            dis = 0
            c_head = chains[N - 1 - i]
            c_tail = chains[N - 2 - i]
            s = c_tail.pop()
            c_head.insert(1, s)
            c_head[0] += s[2]
            c_tail[0] -= s[2]
            pt = 0
            for j in range(1, N):
                dis += abs(chains[j][0] - chains[j - 1][0])
                if abs(chains[j][0] - chains[j - 1][0]) > 0:
                    pt += 1
            dis = dis / pt if pt > 0 else dis
            if self.balance > 0 and dis == self.best:
                return True
            if (
                dis < avg and c_head[0] <= fanout and c_tail[0] <= fanout
            ):  # get a better segment group
                # print(print("moving seg, curren avg,dis,score,best",avg,dis,self.score,self.best,chains))
                avg = dis
                if self.best > avg:
                    self.best = avg
                self.success = 1
                ret = True
            else:
                break
        if ret:
            # print("try again",chains)
            ret = self.move_chain_segment(chains)
        return ret

    def save_tcl_files(self, outdir="./ccff_buf"):
        if not os.path.exists(outdir):
            os.makedirs(outdir)
        for k1 in self.modules["cfgmem"]:
            for k2 in self.modules["cfgmem"][k1]:
                gen_tcl = 0
                if k2.find("tcl_") == 0:
                    gen_tcl = 1
                    fn = f"{outdir}/{k1}_{k2}.tcl"
                    logging.info(f"Generate: {fn}")
                    fp = open(fn, "w")
                    for cmd in self.modules["cfgmem"][k1][k2]:
                        fp.write(cmd + "\n")
                    fp.close()
            if gen_tcl > 0:
                fn = f"{outdir}/{k1}_segments.txt"
                logging.info(f"Generate: {fn}")
                fp = open(fn, "w")
                ix = 0
                for s in self.modules["cfgmem"][k1]["segment"]:
                    fp.write(f"\n# CCFF chain segment {ix} cell list:\n")
                    for c in s:
                        fp.write(c + "\n")
                    ix += 1
                fp.close()

    def create_ccff_group_and_buffer(
        self,
        rst_buf_type,
        clk_buf_type,
        group_head_buf_type,
        rst_pin_name,
        clk_pin_name,
        d_pin_name,
        fanout=33,
        head="ccff_head",
        tail="ccff_tail",
    ):
        for mod in self.modules["cfgmem"]:
            #      print("The value of mod is")
            #      print(mod)
            logging.info(f"Processing ccff chain - {mod}")
            self.parse_ccff_data_chain(mod, head, tail)
            self.group_ccff_chain_cell(mod, fanout)
            self.tcl_insert_buffer_by_chain_group(mod, "prog_reset", rst_pin_name, rst_buf_type)
            groups = self.tcl_insert_buffer_by_chain_group(
                mod, "prog_clk", clk_pin_name, clk_buf_type
            )
            self.tcl_insert_buffer_to_chain_group_head(
                groups, "EFPGA_CCFF_0_*/" + d_pin_name, group_head_buf_type
            )


#####################################################################
# Core function to a complete netlist makeup flow
#####################################################################
def run_ccff_makeup(
    fabric_netlist,
    output_dir,
    fanout,
    rst_buf_type,
    clk_buf_type,
    group_head_buf_type,
    rst_pin_name,
    clk_pin_name,
    d_pin_name,
):
    """
    netlist_path = "/home/wqin/work/tools/alkaid_infra/micropd"
    netlist_type =  "ultimate" # "dp" #
    cfmp = ccff_makeup(f"{netlist_path}/netlist/rtl/{netlist_type}/fabric_netlists.v")
    cfmp.create_ccff_group_and_buffer()
    cfmp.save_tcl_files()
    """
    logging.info("\nRuning pythons script ccff_makeup.py ...")
    cfmp = ccff_makeup(input_files=fabric_netlist)
    cfmp.create_ccff_group_and_buffer(
        rst_buf_type,
        clk_buf_type,
        group_head_buf_type,
        rst_pin_name,
        clk_pin_name,
        d_pin_name,
        fanout=int(fanout),
        head="ccff_head",
        tail="ccff_tail",
    )
    cfmp.save_tcl_files(outdir=output_dir)
    logging.info("\nGenerated ccff chain buffer tcls and segments")


#####################################################################
# Main function
#####################################################################
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a Design Compiler task for netlist synthesis")
    parser.add_argument(
        "--fabric_netlist",
        required=True,
        help="The Verilog file fabric_netlist.v which contains a list of all the FPGA netlists",
    )
    parser.add_argument(
        "--output_dir",
        required=True,
        help="The output directory for the netlists after performing make-up",
    )
    parser.add_argument(
        "--fanout",
        default="33",
        help="The fanout numbers to trigger buffer insertion",
    )
    parser.add_argument(
        "--buf_config",
        help="The YAML config file containing the buffer cell name to be inserted",
    )

    num_errors = 0
    args = parser.parse_args()

    buf_cfg = ccff_buf_config.CcffBufConfig()
    buf_cfg.load(args.buf_config)

    run_ccff_makeup(
        args.fabric_netlist,
        args.output_dir,
        args.fanout,
        buf_cfg.reset_buf_type(),
        buf_cfg.clock_buf_type(),
        buf_cfg.group_head_buf_type(),
        buf_cfg.reset_pin_name(),
        buf_cfg.clock_pin_name(),
        buf_cfg.d_pin_name(),
    )

    if num_errors == 0:
        exit(error_codes["SUCCESS"])
    else:
        exit(error_codes["ERROR"])
