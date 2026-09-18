# Common settings for cocotb makefile when running vcs
DUMP_FSDB = 0
COVERAGE = 0
DUMP_VCD ?= 0
FPGA_LEVEL = 1
ifeq ($(SIM),vcs)
# VCS Options
COMPILE_ARGS := $(COMPILE_ARGS) +define+functional
SIM_BUILD = ./
SIM_ARGS = 
EXTRA_ARGS = 
EXTRA_ARGS := $(EXTRA_ARGS) -debug_access+all
EXTRA_ARGS := $(EXTRA_ARGS) +incdir+
ifeq ($(FPGA_LEVEL),1)
EXTRA_ARGS := $(EXTRA_ARGS) +incdir+${NETLIST_TYPE}
EXTRA_ARGS := $(EXTRA_ARGS) +incdir+${NETLIST_TYPE}/custom_modules
endif
ifeq ($(DUMP_FSDB),1)
EXTRA_ARGS := $(EXTRA_ARGS) -kdb -lca +define+DUMP_FSDB=1
endif
ifeq ($(COVERAGE),1)
EXTRA_ARGS := $(EXTRA_ARGS) -cm line+tgl+cond+fsm
endif
endif
