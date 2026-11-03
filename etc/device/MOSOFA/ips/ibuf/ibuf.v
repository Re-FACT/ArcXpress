`default_nettype none

module ibuf (a2f_i, 
            fpga_in_o
           );

input  wire a2f_i;      // ASIC-to-FPGA
output wire fpga_in_o;     // FPGA side input

 assign fpga_in_o = a2f_i;

endmodule

`default_nettype wire
