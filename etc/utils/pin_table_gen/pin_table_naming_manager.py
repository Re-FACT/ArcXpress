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


# Class of a naming manager for efpga pin table
class PinTableNamingManager:
    def __init__(self):
        # Internal data
        self.__sides_ = []
        self.__x_ranges_ = []
        self.__y_ranges_ = []
        # Constants
        self.__XML_RULE_TAG_ = "side_rule"
        self.__XML_SIDE_TAG_ = "side"
        self.__XML_STARTX_TAG_ = "startx"
        self.__XML_ENDX_TAG_ = "endx"
        self.__XML_STARTY_TAG_ = "starty"
        self.__XML_ENDY_TAG_ = "endy"

    # Read naming rules from XML file
    def load_from_xml(self, xml_file):
        rules_xml = minidom.parse(xml_file)
        rules = rules_xml.getElementsByTagName(self.__XML_RULE_TAG_)
        for rule in rules:
            # Fill the mapping
            if rule.attributes[self.__XML_SIDE_TAG_].value in self.__sides_:
                side_idx = self.__sides_.index(rule.attributes[self.__XML_SIDE_TAG_].value)
                self.__x_ranges_[side_idx].append(
                    [
                        rule.attributes[self.__XML_STARTX_TAG_].value,
                        rule.attributes[self.__XML_ENDX_TAG_].value,
                    ]
                )
                self.__y_ranges_[side_idx].append(
                    [
                        rule.attributes[self.__XML_STARTY_TAG_].value,
                        rule.attributes[self.__XML_ENDY_TAG_].value,
                    ]
                )
            else:
                self.__sides_.append(rule.attributes[self.__XML_SIDE_TAG_].value)
                self.__x_ranges_.append(
                    [
                        [
                            rule.attributes[self.__XML_STARTX_TAG_].value,
                            rule.attributes[self.__XML_ENDX_TAG_].value,
                        ]
                    ]
                )
                self.__y_ranges_.append(
                    [
                        [
                            rule.attributes[self.__XML_STARTY_TAG_].value,
                            rule.attributes[self.__XML_ENDY_TAG_].value,
                        ]
                    ]
                )

    # List rule ids
    def rules(self):
        return range(len(self.__sides_))

    # return side defined in a naming rule
    def side(self, rule_id):
        return self.__sides_[rule_id]

    # Return all the coordinates defined in a naming rule
    def coords(self, rule_id):
        coords = []
        for ipair in range(len(self.__x_ranges_[rule_id])):
            for ix in range(
                int(self.__x_ranges_[rule_id][ipair][0]),
                int(self.__x_ranges_[rule_id][ipair][1]) + 1,
            ):
                for iy in range(
                    int(self.__y_ranges_[rule_id][ipair][0]),
                    int(self.__y_ranges_[rule_id][ipair][1]) + 1,
                ):
                    coords.append([ix, iy])
        return coords

    # Clear all the data
    def clear(self):
        self.__init__()
