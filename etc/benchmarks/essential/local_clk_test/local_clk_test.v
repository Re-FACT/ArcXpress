/* UTF-8 */
module local_clk_test (
    input  clk_i,          // 输入时钟
    input  rst_ni,         // 输入复位信号
    input  A_i,            // A
    input  B_i,            // B
    output reg C_o,            // 正常输出
    output reg C_local_clk_o,  // 本地时钟驱动输出 
    output reg C_local_rst_o   // 本地时钟复位输出
);
    wire clk_local, rst_local;
    assign clk_local = ~clk_i;  // local clock
    assign rst_local = rst_ni & B_i;
    always @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            C_o = 1'b0;
        end else begin
            C_o = A_i;
        end
    end

    always @(posedge clk_local or negedge rst_ni) begin
        if (!rst_ni) begin
            C_local_clk_o = 1'b0;
        end else begin
            C_local_clk_o = A_i;
        end
    end

    always @(posedge clk_i or negedge rst_local) begin
        if (!rst_local) begin
            C_local_rst_o = 1'b0;
        end else begin
            C_local_rst_o = A_i;
        end
    end
endmodule
