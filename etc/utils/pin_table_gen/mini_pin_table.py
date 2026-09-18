import time
import logging
import yaml
import os
import re
import subprocess
from datetime import timedelta
from datetime import datetime
import threading
import csv


# Class of an mini pin table
class MiniPinTable:
    def __init__(self):
        # Internal data
        self.__orientations_ = []
        self.__rows_ = []
        self.__columns_ = []
        self.__offsets_ = []
        self.__pad_names_ = []
        self.__pin_names_ = []
        self.__gpio_types_ = []
        self.__associated_clks_ = []
        self.__clock_edges_ = []
        # Constants
        self.__DELIMITER_ = ","
        self.__QUOTECHAR_ = "|"
        self.__HEAD_ROW_ = [
            "orientation",
            "row",
            "col",
            "pin_num_in_cell",
            "port_name",
            "mapped_pin",
            "GPIO_type",
            "Associated Clock",
            "Clock Edge",
        ]

    # Create a new row
    def create_pin(self):
        pin_idx = len(self.__orientations_)
        self.__orientations_.append("")
        self.__rows_.append("")
        self.__columns_.append("")
        self.__offsets_.append("")
        self.__pad_names_.append("")
        self.__pin_names_.append("")
        self.__gpio_types_.append("")
        self.__associated_clks_.append("")
        self.__clock_edges_.append("")
        return pin_idx

    # Set orientation
    def set_orientation(self, pin_idx, value):
        self.__orientations_[pin_idx] = value

    def set_row(self, pin_idx, value):
        self.__rows_[pin_idx] = value

    def set_column(self, pin_idx, value):
        self.__columns_[pin_idx] = value

    def set_offset(self, pin_idx, value):
        self.__offsets_[pin_idx] = value

    def set_pad_name(self, pin_idx, value):
        self.__pad_names_[pin_idx] = value

    def set_pin_name(self, pin_idx, value):
        self.__pin_names_[pin_idx] = value

    def set_gpio_type(self, pin_idx, value):
        self.__gpio_types_[pin_idx] = value

    def set_associated_clk(self, pin_idx, value):
        self.__associated_clks_[pin_idx] = value

    def set_clock_edge(self, pin_idx, value):
        self.__clock_edges_[pin_idx] = value

    def pins(self):
        return range(len(self.__orientations_))

    def orientation(self, pin_idx):
        return self.__orientations_[pin_idx]

    def row(self, pin_idx):
        return self.__rows_[pin_idx]

    def column(self, pin_idx):
        return self.__columns_[pin_idx]

    def offset(self, pin_idx):
        return self.__offsets_[pin_idx]

    def pad_name(self, pin_idx):
        return self.__pad_names_[pin_idx]

    def pin_name(self, pin_idx):
        return self.__pin_names_[pin_idx]

    def gpio_type(self, pin_idx):
        return self.__gpio_types_[pin_idx]

    def associated_clock(self, pin_idx):
        return self.__associated_clks_[pin_idx]

    def clock_edge(self, pin_idx):
        return self.__clock_edges_[pin_idx]

    # Write pin table to csv file
    def write_csv(self, csv_filename):
        with open(csv_filename, "w") as csvfile:
            csv_writer = csv.writer(
                csvfile,
                delimiter=self.__DELIMITER_,
                quotechar=self.__QUOTECHAR_,
                quoting=csv.QUOTE_MINIMAL,
            )
            csv_writer.writerow(self.__HEAD_ROW_)
            for pin_id in self.pins():
                curr_csv_row = []
                curr_csv_row.append(self.orientation(pin_id))
                curr_csv_row.append(self.row(pin_id))
                curr_csv_row.append(self.column(pin_id))
                curr_csv_row.append(self.offset(pin_id))
                curr_csv_row.append(self.pad_name(pin_id))
                curr_csv_row.append(self.pin_name(pin_id))
                curr_csv_row.append(self.gpio_type(pin_id))
                curr_csv_row.append(self.associated_clock(pin_id))
                curr_csv_row.append(self.clock_edge(pin_id))
                csv_writer.writerow(curr_csv_row)

    # Clear all the data
    def clear(self):
        self.__init__()
