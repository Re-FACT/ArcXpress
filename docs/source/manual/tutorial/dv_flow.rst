.. _tutorial_dv_flow:

Design Verification
-------------------

In the AAEE flow, Design Verification is performed after generating the netlist and bitstream.
This document describes **when to use each Design Verification command**
and **how to run them in practice**, focusing on real-world workflows rather
than option-level descriptions. It is intended to complement the existing
command reference documentation.

.. rubric:: Overall Workflow Overview

A typical Design Verification (DV) flow follows this sequence:

1. **Initialize a design verification context**
   - ``setup_design_verification``

2. **Run preconfigured simulation to validate the FPGA fabric**
   - ``run_preconfigured_simulation``

3. **Perform power analysis after verification passes**
   - ``run_benchmark_power_analysis``

.. rubric:: Preconfigured Fabric Simulation

The ``run_benchmark_simulation`` command is used for a specific benchmark application. It is optional.

**Important:** This command is **not part of the design verification flow**. 
It can be executed independently, typically after architecture evaluation
(``arch_eval``), to check the functional behavior of preconfigured fabric
configurations.

Not all projects require every step; the exact flow depends on verification
goals and project maturity.

setup_design_verification
~~~~~~~~~~~~~~~~~~~~~~~~~

**What this command does**

``setup_design_verification`` initializes a **Design Verification (DV) workspace**
by binding a specific *bitstream*, *netlist*, *physical design*, and *process
corner* together.  
All subsequent simulations and power analyses will be based on this setup.

This command **does not run any simulation**.

------------------------
When to Use This Command
------------------------

You should run this command when:

- Architecture evaluation has completed successfully
- Netlist development has finished (RTL, gate-level, or post-layout)
- (Optional) Physical design results are available
- You want a **stable, reproducible baseline** for verification, regression,
  or power analysis

In a typical flow, this command is executed **once per verification setup**
before running any simulations.

------------------
Typical Invocation
------------------

- **Quick start**: use the latest available results and the default corner

  .. code-block:: bash

     arcxpress> setup_design_verification

By default, this command initializes the Design Verification (DV) workspace with all corners and benchmarks that are not disabled in the configuration. It ensures that all relevant simulations and power analyses can proceed based on this full setup, unless specific corners or benchmarks have been explicitly disabled in the configuration file or through command-line parameters.

- **Recommended for regression**: explicitly specify corner and input runs

  .. code-block:: bash

     arcxpress> setup_design_verification \
       --corner tt_25C_v0p9 \
       --netlist_run iter0 \
       --pd_run iter0 \
       --bitstream_run iter0 \
       --jobs 64

This command creates an independent Design Verification run with a fully
specified setup:

- ``--corner tt_25C_v0p9``  
  Selects the process, temperature, and voltage corner for verification.
  You can use the command ``report_corners`` to check which corners are
  supported by the device.

- ``--netlist_run iter0``  
  Uses the netlist generated in iteration ``iter0`` for verification.

- ``--pd_run iter0``  
  Uses the corresponding physical design results from iteration ``iter0``.
  This is typically required for layout-aware or post-layout verification.

- ``--bitstream_run iter0``  
  Uses the golden bitstream generated in the same iteration to ensure
  consistency across the verification flow.

- ``--jobs 64``  
  Enables parallel execution with up to 64 concurrent jobs to speed up
  the setup process.

---------------
Expected Output
---------------

After invoking the command, the tool will first print a summary of how your
input options have been parsed:

.. code-block:: text

   Confirm selected options when call command 'setup_design_verification'
   --run: off
   --corner: tt_25C_v0p9
   --bitstream run: iter0
   --netlist run: iter0
   --pd run: iter0
   --dryrun: off
   --jobs: 64
   --new_thread_wait_time: off
   --sim_start_wait_time: off
   --verbosity: off

If the setup completes successfully, you will see log messages similar to the
following, indicating that the Design Verification environment has been
initialized and is ready for subsequent commands:

.. code-block:: text

   Initializing design verification workspace...
   Done
   Generating testbenches...
   Setup simulation environment...
   Done

At this point, you can proceed to run simulation or power analysis commands.

-------------------------------
When this command is not needed
-------------------------------

- For quick, standalone benchmark RTL simulations
- When only generating scripts for debugging purposes (use ``--dryrun``)

-----
Notes
-----

This section provides a high-level usage guide for
``setup_design_verification``.  
For detailed descriptions of all available options and their default values,
refer to :ref:`arcxpress_dv_commands_setup_design_verification`.

run_preconfigured_simulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**What this command does**

``run_preconfigured_simulation`` runs pre-configured simulations to verify the
**functional correctness of the entire FPGA fabric** using pre-generated
testbenches.

This command focuses on specific applications mapped on the eFPGA fabric.

------------------------
When to Use This Command
------------------------

You should run this command when:

- ``setup_design_verification`` has completed successfully
- You want to validate overall design functionality for one or more corners
- You want to verify fabric behavior before running benchmark-level simulations
- You are performing functional regression on the FPGA architecture

------------------
Typical Invocation
------------------

- **Quick start**: use the latest available results and the default corner

  .. code-block:: bash

     arcxpress> run_preconfigured_simulation

By default, this command initializes the DV workspace with all available corners and benchmarks that are not disabled in the configuration.

- **Recommended for regression**: explicitly specify key options

  .. code-block:: bash

     arcxpress> run_preconfigured_simulation \
       --corner tt_25C_v0p9 \
       --benchmark and2 \
       --simulator mentor_modelsim \
       --netlist_type rtl \
       --dump_waveform fsdb

The above command runs a fabric-level simulation with the following settings:

- ``--corner tt_25C_v0p9``  
  Selects the process, temperature, and voltage corner for verification.
  You can use the command ``report_corners`` to check which corners are
  supported by the device.

- ``--benchmark and2``  
  Selects the benchmark named **and2** (which must already be loaded)
  as the simulation target.

- ``--simulator mentor_modelsim``  
  Specifies **Mentor ModelSim** as the simulator to run the simulation.

- ``--netlist_type rtl``  
  Uses the RTL-level netlist for simulation.

- ``--dump_waveform fsdb``  
  Enables waveform dumping and specifies **FSDB** as the output format.

---------------
Expected Output
---------------

After invoking the command, the tool will first print a summary showing how
the input options have been parsed:

.. code-block:: text

   --run: off
   --corner: tt_25C_v0p9
   --benchmark: and2
   --jobs: off
   --new_thread_wait_time: off
   --sim_start_wait_time: off
   --log_check_wait_time: off
   --simulator: mentor_modelsim
   --netlist_type: rtl
   --dump_waveform: fsdb
   --verbosity: off

If the simulation completes successfully, you will see log messages similar
to the following:

.. code-block:: text

   Final check will be applied when running architecture generation
   Run preconfigured simulation...
   Done

-------------------
Generated Artifacts
-------------------

- Simulation runs are executed using pre-configured testbenches
- Simulation log files are generated for later review
- Waveform file (``.fsdb``) is generated 

-----
Notes
-----

This command is intended for **fabric-level functional verification** and is
typically executed before running benchmark-level simulations.

For detailed option descriptions and advanced usage, refer to
:ref:`arcxpress_arch_eval_commands_run_preconfigured_simulation`.

run_benchmark_simulation
~~~~~~~~~~~~~~~~~~~~~~~~

**What this command does**

``run_benchmark_simulation`` runs a **benchmark-specific simulation** to verify the
functional behavior of a single application mapped to the FPGA fabric.  
It allows comparison across **RTL, post-synthesis, and post-implementation** netlists, and can optionally generate waveform files.

Unlike ``run_preconfigured_simulation``, this command focuses on **individual benchmark-level verification** rather than the full FPGA fabric.

------------------------
When to Use This Command
------------------------

You should run this command when:

- Bitstream generation for the benchmark has completed successfully
- You want to validate functional correctness for a specific benchmark
- You want to compare RTL, post-synthesis, and post-implementation results
- You are analyzing specific applications, workloads, or corner cases
- You are performing **benchmark-level functional regression**

------------------
Typical Invocation
------------------

- **Quick start**: use the latest available results and default options

  .. code-block:: bash

     arcxpress> run_benchmark_simulation --benchmark and2 --type rtl  

By default, this command initializes the DV workspace with all available options for the selected benchmark, using the latest results unless explicitly specified otherwise.

- **Recommended for regression**: explicitly specify corner, simulation type, simulator, and waveform options

  .. code-block:: bash

     arcxpress> run_benchmark_simulation \
       --corner tt_25C_v0p9 \
       --benchmark and2 \
       --simulator mentor_modelsim \
       --type rtl \
       --dump_waveform fsdb

The above command runs a benchmark-level simulation with the following settings:

- ``--corner tt_25C_v0p9``  
  Specifies the process, temperature, and voltage corner for the simulation.  
  You can use the command ``report_corners`` to check which corners are supported by the device.

- ``--benchmark and2``  
  Selects the benchmark named **and2** as the simulation target. The benchmark must already be mapped to the FPGA fabric.

- ``--simulator mentor_modelsim``  
  Specifies **Mentor ModelSim** as the simulator to run the simulation.

- ``--type rtl``  
  Uses the RTL-level netlist for simulation. Other options include ``post_synth`` and ``post_implementation``.

- ``--dump_waveform fsdb``  
  Enables waveform dumping and specifies **fsdb** format.

---------------
Expected Output
---------------

After invoking the command, the tool will print a summary showing how the input options were parsed:

.. code-block:: text

   Confirm selected options when call command 'run_benchmark_simulation'
   --run: off
   --corner: tt_25C_v0p9
   --benchmark: and2
   --simulator: mentor_modelsim
   --type: rtl
   --dump_waveform: fsdb
   --jobs: off
   --new_thread_wait_time: off
   --sim_start_wait_time: off
   --log_check_wait_time: off
   --verbosity: off

If the simulation completes successfully, you will see log messages similar to:

.. code-block:: text

   Running command:
   Done
   Running rtl simulation succeed.Total time spent running rtl simulation:3.19955 seconds

-------------------
Generated Artifacts
-------------------

- Simulation logs are generated in the ``reports/`` directory
- Waveform files (``.fsdb``) are generated
- Reports can be reviewed to verify functional correctness of the benchmark

-----
Notes
-----

This command is intended for **benchmark-level functional verification** and is typically executed **after `setup_design_verification`** and optional preconfigured simulations.  

For detailed option descriptions and advanced usage, refer to
:ref:`arcxpress_arch_eval_commands_run_benchmark_simulation`.

run_benchmark_power_analysis
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**What this command does**

``run_benchmark_power_analysis`` runs a **Qorso-based power analysis** on an
eFPGA fabric mapped to a specific benchmark.  
It estimates the **dynamic and/or static power consumption** of the FPGA
architecture under a selected process corner and design verification setup.

This command focuses on **benchmark-level power estimation** rather than
functional verification.

------------------------
When to Use This Command
------------------------

You should run this command when:

- You have completed **design verification simulations** (functional correctness)
- You want to estimate power consumption for a specific benchmark
- You want to compare power metrics under different corners or netlist types
- You are performing **power regression** or corner-specific power analysis

------------------
Typical Invocation
------------------

- **Quick start**: use the latest design verification run, default corner, and default settings

  .. code-block:: bash

     arcxpress> run_benchmark_power_analysis \
       --benchmark and2

By default, this command initializes the DV workspace with all available settings for the selected benchmark, using the latest design verification run unless specified otherwise.

- **Recommended for regression**: explicitly specify corner, run id, and parallel jobs

  .. code-block:: bash

     arcxpress> run_benchmark_power_analysis \
       --corner tt_25C_v0p9 \
       --benchmark and2 \
       --jobs 32

The above command runs a benchmark-level power analysis with the following settings:

- ``--corner tt_25C_v0p9``  
  Specifies the process, voltage, and temperature corner for power estimation.  
  You can use the command ``report_corners`` to see which corners are supported by the device.

- ``--benchmark and2``  
  Specifies the benchmark to be analyzed. The benchmark must have been mapped
  to the FPGA fabric during DV.

- ``--jobs 32``  
  Enables parallel execution with up to 32 concurrent jobs to speed up the
  power analysis.

---------------
Expected Output
---------------

After invoking the command, the tool will print a summary showing how the
input options were parsed:

.. code-block:: text

   Confirm selected options when call command 'run_benchmark_power_analysis'
   --run: off
   --corner: tt_25C_v0p9
   --benchmark: and2
   --jobs: 32
   --dryrun: off
   --verbosity: off

If the analysis completes successfully, you will see log messages similar to:

.. code-block:: text

   Run a Qorso-based power analysis...
   Running dynamic power analysis for benchmark 'and2' under corner 'tt_25C_v0p9'
   Done

-------------------
Generated Artifacts
-------------------

- Qorso-based power analysis reports are generated
- Power summary files are outputted in the ``reports/power/`` directory
- Analysis logs can be reviewed to inspect power estimation details

-----
Notes
-----

This command is intended for **benchmark-level power analysis** and is typically
run **after functional verification**.  

For detailed option descriptions and advanced usage, refer to
:ref:`arcxpress_dv_commands_run_benchmark_power_analysis`.
