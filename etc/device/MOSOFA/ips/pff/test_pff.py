import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, FallingEdge, Timer

@cocotb.test()
async def test_pff_modes(dut):
    """Test suite for the programmable flip-flop (pff) across multiple modes."""
    
    # Start a 10ns clock (100 MHz)
    clock = Clock(dut.clk_i, 10, units="ns")
    cocotb.start_soon(clock.start())

    # Initialize inputs
    dut.d_i.value = 0
    dut.clr_i.value = 0
    dut.en_i.value = 0
    dut.mode_i.value = 0

    await Timer(20, units="ns")

    # -------------------------------------------------------------
    # Test 1: Rising Edge, Asynchronous, Active-High Reset, Active-High Enable
    # mode_i breakdown:
    #   [4] clk_edge  = 0 (Rising Edge)
    #   [3] sync_mode = 0 (Asynchronous)
    #   [2] polarity  = 1 (Active-High clr_i)
    #   [1] func_type = 0 (Reset to 0)
    #   [0] en_pol    = 1 (Active-High en_i)
    # Binary: 00101 (0x05)
    # -------------------------------------------------------------
    dut.mode_i.value = 0b00101
    
    # Assert asynchronous reset immediately
    dut.clr_i.value = 1
    await Timer(2, units="ns")
    assert dut.q_o.value == 0, f"Async Reset Failed: Expected 0, got {dut.q_o.value}"

    # Release reset, enable data capture
    dut.clr_i.value = 0
    dut.en_i.value = 1
    dut.d_i.value = 1
    
    await RisingEdge(dut.clk_i)
    await Timer(1, units="ns")  # Propagation delay
    assert dut.q_o.value == 1, f"Data Capture Failed: Expected 1, got {dut.q_o.value}"

    # Disable enable; changing d_i should not affect q_o
    dut.en_i.value = 0
    dut.d_i.value = 0
    await RisingEdge(dut.clk_i)
    await Timer(1, units="ns")
    assert dut.q_o.value == 1, f"Clock Enable Failed: Expected to hold 1, got {dut.q_o.value}"

    # -------------------------------------------------------------
    # Test 2: Falling Edge, Synchronous, Active-Low Set, Active-High Enable
    # mode_i breakdown:
    #   [4] clk_edge  = 1 (Falling Edge)
    #   [3] sync_mode = 1 (Synchronous)
    #   [2] polarity  = 0 (Active-Low clr_i)
    #   [1] func_type = 1 (Set to 1)
    #   [0] en_pol    = 1 (Active-High en_i)
    # Binary: 11011 (0x1B)
    # -------------------------------------------------------------
    dut.mode_i.value = 0b11011
    dut.clr_i.value = 1  # Deasserted for active-low
    dut.en_i.value = 1
    dut.d_i.value = 0

    # Assert synchronous set (clr_i = 0) and wait for the falling edge
    dut.clr_i.value = 0
    await FallingEdge(dut.clk_i)
    await Timer(1, units="ns")
    assert dut.q_o.value == 1, f"Sync Set Failed: Expected 1, got {dut.q_o.value}"

    # Release set signal
    dut.clr_i.value = 1
    await FallingEdge(dut.clk_i)
    await Timer(1, units="ns")
    assert dut.q_o.value == 1, f"Hold State Failed: Expected 1, got {dut.q_o.value}"

    cocotb.log.info("ALL PROGRAMMABLE FF COCOTB TESTS PASSED SUCCESSFULLY!")
