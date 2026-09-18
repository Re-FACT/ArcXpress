import logging
import yaml
import os
import re

# Constants
RESET_BUF_TYPE_TAG = "reset_buf_type"
CLOCK_BUF_TYPE_TAG = "clock_buf_type"
GROUP_HEAD_BUF_TYPE_TAG = "group_head_buf_type"
RESET_PIN_NAME_TAG = "reset_pin_name"
CLOCK_PIN_NAME_TAG = "clock_pin_name"
D_PIN_NAME_TAG = "d_pin_name"


# Class of a PTPX task manager
class CcffBufConfig:
    def __init__(self):
        # Internal data
        self.__db_ = {}
        self.__is_dirty_ = True  # By default it should be dirty. After loading data and pass sanity checks, it becomes clean

    def __is_key_exist(self, key, path):
        if key in path:
            return True
        return False

    # get the selected device name
    def reset_buf_type(self):
        self.__check_valid()
        return self.__db_[RESET_BUF_TYPE_TAG]

    def clock_buf_type(self):
        self.__check_valid()
        return self.__db_[CLOCK_BUF_TYPE_TAG]

    def group_head_buf_type(self):
        self.__check_valid()
        return self.__db_[GROUP_HEAD_BUF_TYPE_TAG]

    def reset_pin_name(self):
        self.__check_valid()
        return self.__db_[RESET_PIN_NAME_TAG]

    def clock_pin_name(self):
        self.__check_valid()
        return self.__db_[CLOCK_PIN_NAME_TAG]

    def d_pin_name(self):
        self.__check_valid()
        return self.__db_[D_PIN_NAME_TAG]

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
        if not self.__check_required_syntax(RESET_BUF_TYPE_TAG):
            return
        if not self.__check_required_syntax(CLOCK_BUF_TYPE_TAG):
            return
        if not self.__check_required_syntax(GROUP_HEAD_BUF_TYPE_TAG):
            return
        if not self.__check_required_syntax(RESET_PIN_NAME_TAG):
            return
        if not self.__check_required_syntax(CLOCK_PIN_NAME_TAG):
            return
        if not self.__check_required_syntax(D_PIN_NAME_TAG):
            return

    # Clear all the data
    def clear(self):
        self.__init__()
