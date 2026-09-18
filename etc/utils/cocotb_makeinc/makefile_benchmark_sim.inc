# Common settings for cocotb makefile when running vcs
DUMP_FSDB = 0
DUMP_VCD ?= 0

ifeq ($(DUMP_VCD), 1)

cocotb_iverilog_dump.v:
	echo 'module cocotb_iverilog_dump();' > $@
	echo 'initial begin' >> $@
	echo '    $$dumpfile("$(TOPLEVEL).vcd");' >> $@
	echo '    $$dumpvars(0, $(TOPLEVEL));' >> $@
	echo 'end' >> $@
	echo 'endmodule' >> $@

ifeq ($(SIM), icarus)
PLUSARGS += -lv
override FST :=
VERILOG_SOURCES += cocotb_iverilog_dump.v
COMPILE_ARGS += -s cocotb_iverilog_dump
endif

ifeq ($(SIM), vcs)
VERILOG_SOURCES += cocotb_iverilog_dump.v
COMPILE_ARGS += -top cocotb_iverilog_dump
endif

endif

ifeq ($(DUMP_FSDB), 1)

cocotb_iverilog_dump.v:
	echo 'module cocotb_iverilog_dump();' > $@
	echo 'initial begin' >> $@
	echo '    $$fsdbDumpfile("$(TOPLEVEL).fsdb");' >> $@
	echo '    $$fsdbDumpvars(0, "+struct", "+mda", "+all");' >> $@
	echo 'end' >> $@
	echo 'endmodule' >> $@

ifeq ($(SIM), vcs)
VERILOG_SOURCES += cocotb_iverilog_dump.v
COMPILE_ARGS += -top cocotb_iverilog_dump
endif

endif
