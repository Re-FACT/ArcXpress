# Makefile

SIM ?= icarus
TOPLEVEL_LANG ?= verilog

# Source files
VERILOG_SOURCES = $(PWD)/pff.v

# Top-level module name
TOPLEVEL = pff

# Python test module name (without .py)
MODULE = test_pff

# Include cocotb's makefile rules
include $(shell cocotb-config --makefiles)/Makefile.sim
