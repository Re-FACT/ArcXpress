module pff (
    input  wire       clk_i,
    input  wire       d_i,
    input  wire       clr_i,     // Reset / Set control signal
    input  wire       en_i,      // Clock enable
    input  wire [3:0] mode_i,    // 4-bit configuration mode input
    output reg        q_o
);

    // Unpack mode bits for readability
    wire clk_edge  = mode_i[3]; // 0: Rising edge, 1: Falling edge
    wire sync_mode = mode_i[2]; // 0: Asynchronous, 1: Synchronous
    wire polarity  = mode_i[1]; // 0: Active-Low,  1: Active-High
    wire func_type = mode_i[0]; // 0: Reset (0),   1: Set (1)

    // Evaluate active state of the control signal (clr_i)
    wire active_ctrl = polarity ? clr_i : ~clr_i;
    wire target_val  = func_type; // 0 for reset, 1 for set

    // Effective clock selection (inverts clock for falling-edge modes)
    // Note: In physical FPGA/ASIC flows, clock inversion is managed via clock gating/mux primitives.
    wire eff_clock = clk_edge ? ~clk_i : clk_i;

    // 1. Asynchronous Reset/Set Behavior
    always @(posedge eff_clock or posedge active_ctrl) begin
        if (sync_mode == 1'b0) begin
            if (active_ctrl) begin
                q_o <= target_val;
            end else if (en_i) begin
                q_o <= d_i;
            end
        end
    end

    // 2. Synchronous Reset/Set Behavior
    always @(posedge eff_clock) begin
        if (sync_mode == 1'b1) begin
            if (active_ctrl) begin
                q_o <= target_val;
            end else if (en_i) begin
                q_o <= d_i;
            end
        end
    end

endmodule
