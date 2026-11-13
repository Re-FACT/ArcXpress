import cocotb
from cocotb.clock import Clock

# from cocotb.triggers import Timer, RisingEdge, FallingEdge
from cocotb.triggers import ValueChange


async def RisingEdge(signal):
    await ValueChange(signal)
    # if not rising edge, keep waiting
    while signal.value != 1:
        await ValueChange(signal)


async def FallingEdge(signal):
    await ValueChange(signal)
    # if not falling edge, keep waiting
    while signal.value != 0:
        await ValueChange(signal)


# 调试模式标志 - 可以通过环境变量设置
DEBUG_MODE = 0


def custom_assert(condition, message="Assertion failed"):
    """
    自定义断言函数，根据 DEBUG_MODE 行为不同：
    - 调试模式：记录错误但不中断进程
    - 普通模式：抛出异常中断程序
    """
    if not condition:
        if DEBUG_MODE == 1:
            # 调试模式下，只记录错误并继续
            cocotb.log.error(f"断言失败 (DEBUG MODE): {message}")
        else:
            # 普通模式下，中断程序
            raise AssertionError(message)


@cocotb.test()
async def test_and2_dffr(dut):
    ################################################################
    # Create clocks
    CLK_PERIOD = 10  # [ns]
    cocotb.start_soon(Clock(dut.clk, CLK_PERIOD, "ns").start())

    ################################################################
    # Test all the cases
    test_id = 0
    dut.rst.value = 0
    for a in range(2):
        for b in range(2):
            await FallingEdge(dut.clk)
            dut._log.info("==== Begin Test case %d ====", test_id)
            dut.a.value = a
            dut.b.value = b

            await RisingEdge(dut.clk)
            dut._log.info("a is %s", dut.a.value)
            dut._log.info("b is %s", dut.b.value)
            dut._log.info("c is %s", dut.c.value)
            expected_c = a & b
            dut._log.info("expected_c is %s", expected_c)
            custom_assert(dut.c.value == expected_c, "c does not match expected value!")

            await FallingEdge(dut.clk)
            expected_d = a & b
            dut._log.info("d is %s", dut.d.value)
            dut._log.info("expected_d is %s", expected_d)
            custom_assert(dut.d.value == expected_d, "d does not match expected value!")
            dut._log.info("==== End Test case %d ====", test_id)
            test_id += 1

    dut.rst.value = 1
    for a in range(2):
        for b in range(2):
            await FallingEdge(dut.clk)
            dut.a.value = a
            dut.b.value = b

            await RisingEdge(dut.clk)
            dut._log.info("a is %s", dut.a.value)
            dut._log.info("b is %s", dut.b.value)
            dut._log.info("c is %s", dut.c.value)
            expected_c = a & b
            dut._log.info("expected_c is %s", expected_c)
            custom_assert(dut.c.value == expected_c, "c does not match expected value!")

            await FallingEdge(dut.clk)
            expected_d = 0
            dut._log.info("d is %s", dut.d.value)
            dut._log.info("expected_d is %s", expected_d)
            custom_assert(dut.d.value == expected_d, "d does not match expected value!")
            dut._log.info("==== End Test case %d ====", test_id)
            test_id += 1
