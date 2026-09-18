.. _tutorial_benchmark_import:

Benchmark Import
================

What is a Benchmark?
--------------------

In FPGA development, a benchmark is a standardized set of test designs
and verification environments used to evaluate FPGA hardware capability,
toolchain efficiency, and design methodologies.

At AAEE, benchmarks are used to validate the functionality and performance
of the eFPGA platform across the full design and verification flow.

Benchmark Configuration File
----------------------------

Each benchmark is defined in a **benchmark configuration file**, which
specifies the required design sources, verification environment, and
constraints. A single file may define multiple benchmarks.

When adding benchmarks to your project, the configuration files must be
included in your **project.xml** file.

**Example of a Benchmark Configuration File (`checkin.xml`)**:

.. code-block:: xml

    <?xml version="1.0"?>
    <benchmark_suite>
        <stamp>
            <author>RapidFlex</author>
            <dates>
            <created>Sat Dec 31 03:14:41 PM PST 2022</created>
            <last_modified>Sat Dec 31 03:14:41 PM PST 2022</last_modified>
            </dates>
            <tool_version>0.0.15-dev+023fa82</tool_version>
        </stamp>
          <benchmark name="and2" suite="checkin">
        <design>
            <file type="verilog">${ROOT_PATH}/etc/benchmarks/alkaid_benchmark/checkin/and2/and2.v</file>
            <file type="blif">${ROOT_PATH}/etc/benchmarks/alkaid_benchmark/micro_benchmark/simple_gates/and2/and2.blif</file>
        </design>
        <top_module>and2</top_module>
        <cocotb>
            <file type="top">${ROOT_PATH}/etc/benchmarks/alkaid_benchmark/micro_benchmark/simple_gates/and2/test_and2.py</file>
            <file type="makefile">${ROOT_PATH}/etc/benchmarks/alkaid_benchmark/micro_benchmark/simple_gates/and2/Makefile</file>
        </cocotb>

        <constraints>
            <file type="pcf">${ROOT_PATH}/etc/benchmarks/alkaid_benchmark/checkin/and2/and2.pcf</file>
        </constraints>
    </benchmark_suite>

Benchmark Suite Structure
-------------------------

- **benchmark_suite**: Groups related benchmarks for a common test purpose.
- **benchmark name**: Unique identifier for each benchmark, also used for
  commands like `disable_benchmark`.
- **top_module**: Specifies the top-level design module for the benchmark.
- **design**: Lists all source files required by the benchmark.
- **cocotb**: Specifies the verification environment (test scripts and Makefile).
- **constraints**: Lists clock, pin, or other implementation constraints.

Defining Benchmarks in Project.xml
----------------------------------

Once the benchmark configuration files are ready, they must be referenced
in **project.xml** to be available in your project:

.. code-block:: xml

    <benchmarks default="and2">
        <file>${ROOT_PATH}/etc/benchmark_config/checkin.xml</file>
    </benchmarks>

- Each `<file>` tag points to a benchmark configuration file.
- The `default` attribute sets the default benchmark to use.
- The project file only references the configuration files; it does not
  redefine individual benchmarks.

Creating Custom Benchmarks
--------------------------

To create a custom benchmark:

1. **Choose or create a benchmark suite**  
   Select an existing suite (e.g., `pcounter`) or define a new suite name.

2. **Define a unique benchmark name**  
   Use a descriptive name reflecting functionality or test scenario.  

3. **Specify the top-level module**  
   Ensure `top_module` matches the design source top module.

4. **Add design source files**  
   Include all required files, specifying valid `type` attributes (e.g., `verilog`).

5. **Configure cocotb testbench**  
   Include top-level test script and Makefile for simulation.

6. **Specify constraint files**  
   Add all implementation constraints required for the design.

7. **Register the benchmark in project.xml**  
   Add the configuration file path to the `<benchmarks>` section.

Example Top Module
------------------

.. code-block:: verilog

    `timescale 1ns / 1ps

    module and2(
        a,
        b,
        c);

    input wire a;
    input wire b;
    output wire c;

    assign c = a & b;

    endmodule

Notes
-----

- Benchmark configuration files define the benchmark fully; the project.xml
  only references them.
- Retain consistent names across `top_module`, design files, and benchmark
  configuration.
- Multiple benchmarks can be included in a single configuration file.
