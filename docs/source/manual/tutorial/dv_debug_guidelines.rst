.. _developer_contributor_guidelines_debug_guidelines:

Debug Guidelines
================

When a test case in preconfigured verification fails, please consider the following steps in debugging

- Enable waveform outputs in simulation. Refer to the :ref:`arkangel_dv_commands_run_preconfig_sim`


.. code-block::

  # After dv run, you can find waveform file under <your_dv_home_directory>/preconfig_testbench/ultimate/checkin/and2/and2.fsdb

.. note:: For small FPGA fabrics, it is feasiable to log all the signals. However, for large FPGA fabrics, e.g., 24x24, we suggest to only log part of the signals. Otherwise, the waveform file is too big and costs a long waiting time to load in waveform viewers.

- Check if the I/O constraints or FPGA I/O assignment are correct. See details in the preconfigured wrapper Verilog

  .. note:: If the design contains any ``clock`` and ``reset`` signal, pin constraint files are required!

- Check if the input signal arrives at the designated I/O tile and drives global routing at the designated pin.

  - The I/O placement results can be found at ``reports/<FABRIC_NAME>/vpr_results/<BENCHMARK_SUITE_NAME>/*/*.place`` under the root of the repository
  - In this FPGA, I/Os are on the perimeter of the fabric.
  - For example, when an input signal ``a`` is place on ``0   2   0``, it is placed on the 1st pin of ``grid_io_left_0__2_``. You can find the corresponding block/module in the waveform viewer  

.. code-block::

  #block name    x   y   subblk  block number
  #---------- --  --  ------  ------------
  c       1   2   0   #0
  out:c       3   0   0   #1
  a       0   2   0   #2
  b       1   0   0   #3  

.. note:: VPR adds an prefix ``out:`` to output signal by default. Please be aware of this name change when probing signals!

- Check if the input signal pass through global routing and arrives at the designated input pin of a programmable block, which can be a CLB or DSP or BRAM.

  - The routing traces can be found at ``reports/<FABRIC_NAME>/vpr_results/<BENCHMARK_SUITE_NAME>/*/*.route`` under the root of the repository
  - For example, the following routing trace means that a signal ``a`` 

    - starts from the pin 3 of I/O tile ``grid_io_left_0__2_``
    - through a Y-direction routing track ``chany[0]`` in the switch block ``sb_0__2_`` 
    - through a X-direction routing track ``chanx[96]`` in the switch block ``sb_1__2_`` 
    - through a X-direction routing track ``chanx[98]`` in the switch block ``sb_2__2_`` 
    - ends at the pin 5 of a CLB tile ``grid_clb_1__2_``

.. code-block::

  Net 0 (a)

  Node:   5040    SOURCE (0,2)  Pad: 3  Switch: 0
  Node:   5094      OPIN (0,2)  Pad: 3  Switch: 2
  Node:   98332    CHANY (0,2)  Track: 0  Switch: 3
  Node:   73848    CHANX (1,2) to (2,2)  Track: (96, 98)  Switch: 1
  Node:   5213      IPIN (1,2)  Pin: 5   clb.I[5] Switch: 0
  Node:   5174      SINK (1,2)  Class: 0  Switch: -1

- Check if the signal arrives the designated input of a Look-Up Table (LUT) or DSP or BRAM. 

  - The mapping results can be found at ``reports/<FABRIC_NAME>/vpr_results/<BENCHMARK_SUITE_NAME>/*/*.net`` under the root of the repository
  - For example, the following mapping results mean that 

    - The 11th FLE inside a CLB is used.
    - The 2nd and 3rd inputs are routed from the CLB inputs ``clb.I[0]`` and ``clb.I[1]``.

.. code-block::

        <block name="c" instance="fle[11]" mode="n2_lut3">
            <inputs>
                <port name="in">open clb.I[0]-&gt;crossbar clb.I[1]-&gt;crossbar open</port>
                <port name="sc_in">open</port>
                <port name="cin">open</port>
                <port name="reset">open</port>
            </inputs>
            <outputs>
                <port name="out">open lut3inter[0].out[1]-&gt;direct_fle_out</port>
                <port name="sc_out">open</port>
                <port name="cout">open</port>
            </outputs>
            <clocks>
                <port name="clk">open</port>
            </clocks>
            <block name="c" instance="lut3inter[0]" mode="default">
                <inputs>
                    <port name="in">open fle.in[1]-&gt;direct_fle_in fle.in[2]-&gt;direct_fle_in</port>
                    <port name="reset">open</port>
                </inputs>
                <outputs>
                    <port name="out">open ble3[1].out[0]-&gt;direct_ble_out</port>
                </outputs>
                <clocks>
                    <port name="clk">open</port>
                </clocks>
                <block name="open" instance="ble3[0]" />
                <block name="c" instance="ble3[1]" mode="default">
                    <inputs>
                        <port name="in">open lut3inter.in[1]-&gt;direct_ble1_in lut3inter.in[2]-&gt;direct_ble1_in</port>
                        <port name="reset">open</port>
                    </inputs>
                    <outputs>
                        <port name="out">lut3[0].out[0]-&gt;mux_seq_comb_selector</port>
                    </outputs>
                    <clocks>
                        <port name="clk">open</port>
                    </clocks>
                    <block name="c" instance="lut3[0]" mode="lut3">
                        <inputs>
                            <port name="in">open ble3.in[1]-&gt;direct_lut_in ble3.in[2]-&gt;direct_lut_in</port>
                        </inputs>
                        <outputs>
                            <port name="out">lut[0].out[0]-&gt;direct:lut3</port>
                        </outputs>
                        <clocks />
                        <block name="c" instance="lut[0]">
                            <attributes />
                            <parameters />
                            <inputs>
                                <port name="in">open lut3.in[1]-&gt;direct:lut3 lut3.in[2]-&gt;direct:lut3</port>
                                <port_rotation_map name="in">open 1 0</port_rotation_map>
                            </inputs>
                            <outputs>
                                <port name="out">c</port>
                            </outputs>
                            <clocks />
                        </block>
                    </block>
                    <block name="open" instance="ff[0]" />
                </block>
            </block>

- Check if the primitive block, e.g., LUT,  is programmed correctly. The waveform of its configuration ports and compare with the ``fabric_bitstream.xml``.

- If there are doubts on the correctness of bitstream for LUTs, the original truth table can be found in the ``.blif`` file (see the following example). For a mapped LUT in a ``.net`` file, the block is named after the output net of a LUT in a ``.blif`` file. This is applied to each primitive block in ``.net`` files. For example, the LUT in the following example is mapped to a block whose name is ``c`` in the ``.net`` file.

.. code-block::

    # Benchmark "and2" written by ABC on Thu Sep 22 11:08:10 2022
    .model and2
    .inputs a b 
    .outputs c 
    .names b a c
    11 1
    .end

- Check if the outputs of the primitive block, e.g., LUT, are expected. 

- Check if the outputs are propagated to the designated programmable block outputs. Refer to the way in decoding ``.net`` file which is introduced in previous steps.

- Check if the outputs are propagated through the global routing and arrive at the designated I/O tile. Refer to the way in decoding ``.route`` file which is introduced in previous steps.

- Check if the outputs are propagated to the designated FPGA pin. Refer to the way in tracing input signals which is introduced in previous steps.

- Check all the signals one by one

