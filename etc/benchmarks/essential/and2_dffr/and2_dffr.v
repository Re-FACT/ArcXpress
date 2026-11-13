/////////////////////////////////////////
//  Functionality: 2-input AND with clocked DFF
//                 and combinational outputs 
//  Author:        Xifan Tang
////////////////////////////////////////

`timescale 1ns / 1ps

module and2_dffr(
  a,
  b,
  clk,
  rst,
  c,
  d);

input wire clk;
input wire rst;

input wire a;
input wire b;
output wire c;
output reg d;

assign c = a & b;

initial begin
  d = 0;
end

always @(posedge clk or posedge rst) begin
  if (rst) begin
    d = 0;
  end else begin
    d <= c;
  end
end

endmodule
