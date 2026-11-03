`default_nettype none

module obuf (f2a_o, fpga_out_i);

input  wire fpga_out_i;    // Output pin from FPGA internal fabric
output wire f2a_o;     // FPGA-to-ASIC

  assign f2a_o = fpga_out_i;

endmodule

`default_nettype wire
