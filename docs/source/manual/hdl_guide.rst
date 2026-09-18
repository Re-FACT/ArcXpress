.. _manual_hdl_code:

HDL Coding Guidelines
=====================

Standards
---------

.. list-table:: HDL Support
  :widths: 20 20 20 20
  :header-rows: 1

  * - Language
    - Verilog
    - VHDL
    - SystemVerilog
  * - Standard
    - IEEE Std 1364-2005
    - N/A
    - N/A


Basic Coding Rules
------------------

Explicit Register Declaration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Registers must be explicitly declared.

.. code-block::

  // reg must be explicitly defined 
  module test (
      input  wire a,  // wire can be explicitly defined
      input       b,  // wire can be implicitly defined
      output reg  c,  // reg must be explicitly defined
      output reg  d   // reg must be explicitly defined
  );

alternatively,

.. code-block::

  // the following also works 
  module test (
      a,
      b,
      c,
      d
  );
      input wire a;   // wire can be explicitly defined
      input      b;   // wire can be implicitly defined
      output reg c;   // reg must be explicitly defined
      output reg d;   // reg must be explicitly defined

Register Assignments
~~~~~~~~~~~~~~~~~~~~

Use Non-Blocking Assignment
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Always use non-blocking assignment (``<=``) for register assignments.

.. code-block::

  reg state_c, state_n;
  always @(posedge clk or negedge rst_n) begin
      if (!rst_n) begin
          state_c <= 1'b0;
      end else begin
          state_c <= state_n;     // use non-blocking assignment
          // state_c = state_n;   // blocking assignment , not recommended
      end
  end

Reset Requirement
^^^^^^^^^^^^^^^^^
Registers **must have a reset signal**.

.. warning::  
   Failing to specify initial states will cause 'X' signals when running post-synthesis simulations (see guidelines in :ref:`datasheet_post_synth_sim`).

.. code-block:: 

    // registers must have a reset signal 
    reg state_c, state_n;

    // correct case: include reset
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state_c <= 1'b0;
        end else begin
            state_c <= state_n;
        end
    end

    // wrong case: no reset
    // always @(posedge clk) begin
    //     state_c <= state_n;
    // end


Assignments in Combinational Logic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``assign`` for Combinational Logic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: 

    wire a, b, c;
    assign c = a & b;

Blocking Assignments
^^^^^^^^^^^^^^^^^^^^
Blocking assignments (``=``) are only allowed in combinational logic.

.. code-block::   

  wire a, b, c;
  reg d;
  assign c = a & b;
  always @(posedge clk_i or negedge rst_ni) begin
      if (!rst_ni) begin
          d <= 1'b0;
      end else begin
          d <= c;
          // d = c; // blocking assignments are only allowed in combinational logic
      end
  end


Conditional Statements
~~~~~~~~~~~~~~~~~~~~~~

Complete Case/If Statements
^^^^^^^^^^^^^^^^^^^^^^^^^^^
``case`` and ``if`` must cover **all conditions**. Otherwise, a ``default`` branch is required.

.. code-block::

  always @(posedge clk_i or negedge rst_ni) begin
      if (!rst_ni) begin
          d <= 2'd0;
      end else begin
          case (state)
              2'b01: d <= 2'd1;
              2'b10: d <= 2'd2;
              2'b11: d <= 2'd2;
              2'b00: d <= 2'd0;   // must cover all conditions
              // default: d <= 2'd0; // or use default
          endcase
      end
  end

Prohibit Implicit Latches
^^^^^^^^^^^^^^^^^^^^^^^^^
All outputs must be assigned values in **all branches** within any logic block.

.. code-block:: 

  // Wrong case
  reg a, b;
  always @(posedge clk or negedge rstn) begin
      if (!rstn) begin
          b <= 1'b0;
      end else begin
          if (a == 1'b0) begin
              b <= 1'b1;
          end
      end
  end

  // Correct case
  reg a, b;
  always @(posedge clk or negedge rstn) begin
      if (!rstn) begin
          b <= 1'b0;
      end else begin
          if (a == 1'b0) begin
              b <= 1'b1;
          end else begin
              b <= 1'b0;  // all outputs must be assigned values
          end
      end
  end


Assignment Consistency
~~~~~~~~~~~~~~~~~~~~~~

Do Not Mix ``=`` and ``<=``
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Do not mix blocking (``=``) and non-blocking (``<=``) assignments in the same ``always`` block.

.. code-block:: 

    reg a, b, c;
    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            b <= 1'b0;
        end else begin
            c <= a;
            b <= c;
            // b = c;  // Do not mix <= and =
        end
    end


Data Structures
---------------

Avoid Complex Arrays
~~~~~~~~~~~~~~~~~~~~
Avoid using multidimensional arrays and dynamic arrays. Use only fixed-length vectors.

.. code-block::

    // reg [7:0] memory [0:7];    // not recommended
    reg [63:0] memory;            // recommended


Always Blocks
-------------

Avoid ``always @(*)``
~~~~~~~~~~~~~~~~~~~~~
Avoid using ``always @(*)`` blocks. Instead, provide explicit sensitivity lists.

.. code-block::

  // Avoid to use always @(*) even for combinatorial blocks
  always @(posedge clk or negedge rst) begin
    if (rst == 1'b0) begin
      result <= 0;
    end else begin
      result <= result + 1;
    end
  end

Location Constraints
--------------------

User can define location constraints through the following parameters when instanciate primitives:

- ``FPGA_LOC_X`` define the x coordinate. Please ensure the number is in the range of the FPGA fabric width. Otherwise, placement may fail. Refer to your device datasheet to valid range.
- ``FPGA_LOC_Y`` define the y coordinate. Please ensure the number is in the range of the FPGA fabric height. Otherwise, placement may fail. Refer to your device datasheet to valid range.
- ``FPGA_LOC_Z`` define the z coordinate. Please ensure the number is in the range of the capacity of (x, y) defined through ``FPGA_LOC_X`` and ``FPGA_LOC_Y``. Otherwise, placement may fail. Refer to your device datasheet to valid range.

.. warning:: Use real number. Integer will not work!

.. code-block::

  // Constrain the primitive to be mapped to (x=3, y=2, z=1)
  pcounter32 pcnt #(
    .FPGA_LOC_X(3.0), 
    .FPGA_LOC_Y(2.0), 
    .FPGA_LOC_Z(1.0)
  ) ( 
    .clk_i(ck),
    .rst_i(rst),
    .q_o(out)
  );

Other Rules
-----------

- ``define`` macros should only be used for parameterization.  
  ``Parameters`` are recommended as the first choice.  
- Avoid using ``inout``.  
- Do not rely on ``#delay`` and avoid behavioral-level syntax such as ``fork...join``.  
- Separate synthesizable code from testbench code.  
- The use of system tasks such as ``$display`` and ``$finish`` is prohibited in synthesizable code.  

PCNT and CCB Deployment
-----------------------


PCNT's and CCB's event trigger
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PCNT's event trigger ``event_i`` and CCB's event trigger ``pcnt_event_i`` can be connected together, but their functionalities are not entirely identical.
    
    - PCNT's port ``event_i`` triggers PCNT's event, which will load the ``LOAD_VAL`` into the counter,   
    - CCB's event will update the ``MATCH0_VAL`` and ``MATCH1_VAL`` to the corresponding selected counter. And there are some differences between the 16-bit and 32-bit modes. 
    
For example, when using 32-bit counters, the ``event_i[0]``, ``event_i[2]``, ``event_i[4]`` are corresponding to the ``pcnt0``, ``pcnt1``, ``pcnt2``, respectively. And the ``event_i[1]``, ``event_i[3]``, ``event_i[5]`` are not valid, and they should be set to logic 0.


.. code-block::

    ccb ccb_inst #(
        .FPGA_LOC_X(3.0),
        .FPGA_LOC_Y(2.0),
        .FPGA_LOC_Z(0.0)
    )(
    ...
    .pcnt_event_i({pcnt0_event, 1'b0, pcnt1_event, 1'b0, pcnt2_event, 1'b0})
    ...
    );
    ...

    pcounter32 pcnt0 #(
        .FPGA_LOC_X(3.0),
        .FPGA_LOC_Y(2.0),
        .FPGA_LOC_Z(1.0)
    )(
    ...
    .event_i(pcnt0_event),
    ...
    );

    pcounter32 pcnt1 #(
        .FPGA_LOC_X(3.0),
        .FPGA_LOC_Y(2.0),
        .FPGA_LOC_Z(2.0)
    )(
    ...
    .event_i(pcnt1_event),
    ...
    );

    pcounter32 pcnt2 #(
        .FPGA_LOC_X(3.0),
        .FPGA_LOC_Y(2.0),
        .FPGA_LOC_Z(3.0)
    )(
    ...
    .event_i(pcnt2_event),
    ...
    );

And when using 16-bit counters, the ``event_i[0:5]`` are corresponding to the ``pcnt0`` to ``pcnt5``, respectively. 

.. code-block::

    ccb (
    ...
    .pcnt_event_i({pcnt0_event, pcnt1_event, pcnt2_event, pcnt3_event, pcnt4_event, pcnt5_event})
    ...
    );
    ...

    pcnt0 (
    ...
    .event_i(pcnt0_event),
    ...
    );

    pcnt1 (
    ...
    .event_i(pcnt1_event),
    ...
    );

    ...
    
    pcnt4 (
    ...
    .event_i(pcnt4_event),
    ...
    );
    pcnt5 (
    ...
    .event_i(pcnt5_event),
    ...
    );

PCNT's enable signal
~~~~~~~~~~~~~~~~~~~~
  PCNT's enable signal should be set to either 0 or 1, and must not be left floating. (Most inputs are **NOT** recommended to be left floating.)

.. code-block::

    pcounter32_clkp_arstn_load #(
        ...
    ) core (
        ...
        .enable_i(1'b1),    // Correct usage
        //.enable_i(),      // NOT allowed
        ...
    );

PCNT's initial value
~~~~~~~~~~~~~~~~~~~~
  PCNT's ``LOAD_VAL``, ``MATCH0_VAL``, ``MATCH1_VAL`` should include the number of bits, such as ``32'd100`` and ``16'd101`` 
  
  .. code-block::

    pcounter32_clkp_arstn_load #(
        //.LOAD_VAL  (20),        // NOT recommend
        .LOAD_VAL  (32'd20),    // recommend
    ) core (
        ...
    );
  
  And **MUST** be adapted to match the corresponding bit width of the PCNT.

  For example, a 16-bit pcnt must use a 16-bit variable and cannot be a 32-bit one.
  
  .. code-block::

    pcounter16_clkp_arstn_load #(
        //.MATCH0_REF(5),       // NOT allowed because unsized numeric constants in Verilog are treated as 32-bit signed values by default.
        //.MATCH1_REF(32'd10)   // NOT allowed due to bit width mismatch
        .MATCH0_REF(16'd5),     // Correct writing style
        .MATCH1_REF(16'd10)     // Correct writing style
    ) core (
        ...
    );


Testbench
---------
Monitor and Driver
~~~~~~~~~~~~~~~~~~
 Testbench should NOT access any internal signals(Although Cocotb supports such access, these signals cannot be accessed in the FPGA after preconfiguration). Only the top-level ports should be checked and modified.
 
 Each input of the top-level module should have a specific value and should not be left floating; the input signals of the intermediate-level modules, whether floating or not, should not be accessed by the testbench.

.. code-block::

    module PWM_test (
        input      clk_i,        // Recommended for testbench access
        input      rst_ni,       // Recommended for testbench access
        input      sel,          // Recommended for testbench access
        output reg pwm_opening,  // Recommended for testbench access
        output reg pwm_closing   // Recommended for testbench access
    );
        wire [1:0] state;           // NOT recommended for testbench access
        reg  [1:0] state_d1;        // NOT recommended for testbench access
        wire       reset_counter;   // NOT recommended for testbench access
        wire       match1_o;        // NOT recommended for testbench access
    
    ...

    endmodule 
