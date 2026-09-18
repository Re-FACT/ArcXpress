import time
import logging
import yaml
import os
import re
import subprocess
from datetime import timedelta
from datetime import datetime
import threading
from xml.dom import minidom


# Class of an I/O coordinate manager
class IoCoordManager:
    def __init__(self):
        # Internal data
        self.__io_coord2names_ = {}
        self.__io_xy2zs_ = {}
        # Constants
        self.__XML_IO_TAG_ = "io"
        self.__XML_IO_PAD_TAG_ = "pad"
        self.__XML_IO_X_TAG_ = "x"
        self.__XML_IO_Y_TAG_ = "y"
        self.__XML_IO_Z_TAG_ = "z"
        self.__COORD_DELIM_ = ","

    # Build a key for coord (only x and y)
    def __build_coordxy_key(self, x, y):
        coord_str = ""
        coord_str += str(x)
        coord_str += self.__COORD_DELIM_
        coord_str += str(y)
        return coord_str

    # Build a key for coord
    def __build_coord_key(self, x, y, z):
        coord_str = ""
        coord_str += str(x)
        coord_str += self.__COORD_DELIM_
        coord_str += str(y)
        coord_str += self.__COORD_DELIM_
        coord_str += str(z)
        return coord_str

    # Read I/O coordinates from XML file
    def load_from_xml(self, xml_file):
        io_xml = minidom.parse(xml_file)
        io_coords = io_xml.getElementsByTagName(self.__XML_IO_TAG_)
        for io_coord in io_coords:
            # Build key to search pad name by coord
            coord_str = self.__build_coord_key(
                io_coord.attributes[self.__XML_IO_X_TAG_].value,
                io_coord.attributes[self.__XML_IO_Y_TAG_].value,
                io_coord.attributes[self.__XML_IO_Z_TAG_].value,
            )
            coordxy_str = self.__build_coordxy_key(
                io_coord.attributes[self.__XML_IO_X_TAG_].value,
                io_coord.attributes[self.__XML_IO_Y_TAG_].value,
            )
            # Fill the mapping
            self.__io_coord2names_[coord_str] = io_coord.attributes[self.__XML_IO_PAD_TAG_].value
            if coordxy_str not in self.__io_xy2zs_:
                self.__io_xy2zs_[coordxy_str] = []
            self.__io_xy2zs_[coordxy_str].append(
                int(io_coord.attributes[self.__XML_IO_Z_TAG_].value)
            )
        # Sort the xy-to-z list
        for key_xy in self.__io_xy2zs_:
            self.__io_xy2zs_[key_xy].sort(reverse=False)

    # Get the pad name by a given coordinate
    def pad_name(self, x, y, z):
        coord_str = self.__build_coord_key(x, y, z)
        return self.__io_coord2names_[coord_str]

    # Get the range of z under a given x, y
    def z_range(self, x, y):
        coordxy_str = self.__build_coordxy_key(x, y)
        if coordxy_str not in self.__io_xy2zs_:
            return []  # Bypass as the coordinate does not exist
        return self.__io_xy2zs_[coordxy_str]

    # Clear all the data
    def clear(self):
        self.__init__()
