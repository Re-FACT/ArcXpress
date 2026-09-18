import logging
import yaml
import os
import re

# Constants


RUN_SPECIFIC_SIG_FT_TAG = "run_specific_sig_ft"
RUN_BRAM_MAKEUP_TAG = "run_bram_makeup"
RUN_BOUNDARYSCAN_MAKEUP_TAG = "run_boundaryscan_makeup"
RUN_REMOVE_UNDRIVEN_PORTS_TAG = "run_remove_undriven_ports"
RUN_CLK_BUFFERS_ADD_TAG = "run_clk_buffers_add"
RUN_SPECIFIC_SIG_OUTPUT_TAG = "run_specific_sig_output"
RUN_ADD_PORTS_TAG = "run_add_ports"
RUN_CONNECT_INSTANCE_PORT_TO_TOP_TAG = "run_connect_instance_port_to_top"
SPECIFIC_SIG_OUT_INFO_TAG = "specific_sig_out_info"
INST_PORT_INFO_TAG = "inst_port_info"
SPECIFIC_PORT_TO_TOP_INFO_TAG = "specific_port_to_top_info"


# Class of a PTPX task manager
class NetlistMakeupConfig:
    def __init__(self):
        # Internal data
        self.__db_ = {}
        self.__is_dirty_ = True  # By default it should be dirty. After loading data and pass sanity checks, it becomes clean

    def __is_key_exist(self, key, path):
        if key in path:
            return True
        return False

    # get the selected device name
    def run_clk_buffers_add(self):
        self.__check_valid()
        return self.__db_[RUN_CLK_BUFFERS_ADD_TAG]

    def run_specific_sig_output(self):
        self.__check_valid()
        return self.__db_[RUN_SPECIFIC_SIG_OUTPUT_TAG]

    def specific_sig_out_info(self):
        self.__check_valid()
        return self.__db_[SPECIFIC_SIG_OUT_INFO_TAG]

    def run_connect_instance_port_to_top(self):
        self.__check_valid()
        return self.__db_[RUN_CONNECT_INSTANCE_PORT_TO_TOP_TAG]

    def specific_port_to_top_info(self):
        self.__check_valid()
        return self.__db_[SPECIFIC_PORT_TO_TOP_INFO_TAG]

    def run_specific_sig_ft(self):
        self.__check_valid()
        return self.__db_[RUN_SPECIFIC_SIG_FT_TAG]

    def run_bram_makeup(self):
        self.__check_valid()
        return self.__db_[RUN_BRAM_MAKEUP_TAG]

    def run_boundaryscan_makeup(self):
        self.__check_valid()
        return self.__db_[RUN_BOUNDARYSCAN_MAKEUP_TAG]

    def run_remove_undriven_ports(self):
        self.__check_valid()
        return self.__db_[RUN_REMOVE_UNDRIVEN_PORTS_TAG]

    def run_add_ports(self):
        self.__check_valid()
        return self.__db_[RUN_ADD_PORTS_TAG]

    def inst_port_info(self):
        self.__check_valid()
        return self.__db_[INST_PORT_INFO_TAG]

    # Internal method to check if data is valid, throw exeception when invalid. Useful for accessors
    def __check_valid(self):
        if self.__is_dirty_ == True:
            raise Exception("Try to access data when internal data is still dirty. Load data first")

    def is_valid(self):
        return not (self.__is_dirty_)

    def __check_required_syntax(self, req_syntax):
        if req_syntax not in self.__db_:
            logging.error(f"Required syntax {req_syntax} not found!")
            self.__is_dirty_ = True
            return False
        return True

    # Load data from yaml file
    def load(self, yaml_filename):
        self.__db_ = {}  # Ensure a clean start
        with open(yaml_filename, "r") as stream:
            try:
                self.__db_ = yaml.load(stream, Loader=yaml.FullLoader)
            except yaml.YAMLError as exc:
                logging.error(exc)
        # Validation
        self.__is_dirty_ = False
        if not self.__check_required_syntax(RUN_CLK_BUFFERS_ADD_TAG):
            return
        if not self.__check_required_syntax(RUN_SPECIFIC_SIG_OUTPUT_TAG):
            return
        if not self.__check_required_syntax(SPECIFIC_SIG_OUT_INFO_TAG):
            return
        if not self.__check_required_syntax(RUN_SPECIFIC_SIG_FT_TAG):
            return
        if not self.__check_required_syntax(RUN_BRAM_MAKEUP_TAG):
            return
        if not self.__check_required_syntax(RUN_BOUNDARYSCAN_MAKEUP_TAG):
            return
        if not self.__check_required_syntax(RUN_REMOVE_UNDRIVEN_PORTS_TAG):
            return
        if not self.__check_required_syntax(RUN_ADD_PORTS_TAG):
            return
        if not self.__check_required_syntax(INST_PORT_INFO_TAG):
            return
        if not self.__check_required_syntax(RUN_CONNECT_INSTANCE_PORT_TO_TOP_TAG):
            return
        if not self.__check_required_syntax(SPECIFIC_PORT_TO_TOP_INFO_TAG):
            return

    # Clear all the data
    def clear(self):
        self.__init__()
