.. _arcxpress_dv_commands:

Design Verification Commands
----------------------------

.. _arcxpress_dv_commands_setup_design_verification:

setup_design_verification
~~~~~~~~~~~~~~~~~~~~~~~~~

Setup design verification for selected golden bitstream and netlists

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner. Use the keyword ``all`` to sweep all the available corners.  

.. note:: Expect long runtime and high-usage on computing resource when you choose to sweep all the corners

.. option:: --bitstream_run <string>

  Specify the run id of architecture evaluation. See details in :ref:`arcxpress_arch_eval_commands`. Note that the architecture evaluation should be finished successfully. If not specifed, the latest run will be automatically consideried.

.. option:: --netlist_run <string>

  Specify the run id of netlist development. See details in :ref:`arcxpress_netlist_dev_commands`. Note that the netlist development should be finished successfully. If not specifed, the latest run will be automatically consideried.

.. option:: --pd_run <string>

  .. note:: For gate-level design verification, this is not required.

  Specify the run id of physical design. See details in :ref:`arcxpress_netlist_dev_commands`. Note that the physical design should be finished successfully. If not specifed, the latest run will be automatically consideried.

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on design verification

.. option:: --dryrun

  .. warning:: This is ONLY for debugging use

  When enabled, only scripts of design verification will be generated. Third party tool will not be run

.. option:: --new_thread_wait_time <int>

  Specify the time (in sec.) to wait before starting a new thread (when --jobs > 2). By default, it is 1 sec.

.. option:: --sim_start_wait_time <int>

  Specify the time (in sec.) to wait before starting a new thread for simulation (when --jobs > 2). By default, it is 1 sec.

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.

.. _arcxpress_dv_commands_run_preconfigured_simulation:

run_preconfigured_simulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run preconfigured simulation based on the selected netlists and benchmarks

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner.  Use the keyword ``all`` to sweep all the available corners. 

.. note:: Expect long runtime and high-usage on computing resource when you choose to sweep all the corners

.. option:: --benchmark <string>

  Specify the name of the benchmark to be considered. Must be a valid one and not disabled. If not specified, all the benchmarks (not disabled) will be considered.

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on simulations

.. option:: --new_thread_wait_time <int>

  Specify the time (in sec.) to wait before starting a new thread (when --jobs > 2). By default, it is 1 sec.

.. option:: --sim_start_wait_time <int>

  Specify the time (in sec.) to wait before starting a new thread for simulation (when --jobs > 2). By default, it is 1 sec.

.. option:: --log_check_wait_time <int>

  Specify the time (in sec.) to wait before starting a new thread for checking simulation logs (when --jobs > 2)

.. option:: --simulator <string>

  Specify the simulator to be used when running simulation. Available options are [ ``synopsys_vcs`` | ``mentor_modelsim`` | ``mentor_questa`` ]. By default, vcs is considered

.. option:: --netlist_type <string>

  Specify the type of netlist to be considered when running simulation. Available options are [ ``rtl`` | ``gl`` | ``pl`` ]. By default, ``rtl`` is considered

.. option::	--dump_waveform <string>

  Specify if the waveform file should be outputted when running simulation. Available options are [ none | fsdb | vcd ]. By default is none, implicating no waveform files are outputted.

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is ``0``, leading to minimum logging messages.

.. _arcxpress_arch_eval_commands_run_benchmark_power_analysis:

run_benchmark_power_analysis
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run a Qorso-based power analysis on an eFPGA fabric mapped to a specific benchmark under a specific design verification and a selected corner

.. option:: --run <string>

  Specify the run id for the design verification. If not specified, consider the latest run.

.. option:: --corner <string>

  Specify the corner name to be considered. If not specified, the default corner will be considered. Use the keyword ``all`` to sweep all the available corners. 

.. option:: --benchmark <string>

  Specify the name of the benchmark to be considered.

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on power analysis

.. option:: --dryrun

  .. warning:: This is ONLY for debugging use

  When enabled, only scripts of power analysis will be generated. Third party tool will not be run

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is ``0``, leading to minimum logging messages.

run_benchmark_sta
~~~~~~~~~~~~~~~~~

Run a STA analysis on an eFPGA fabric mapped to a specific benchmark under a specific design verification and a selected corner

.. option:: --run <string>

  Specify the run id for the design verification. If not specified, consider the latest run.

.. option:: --corner <string>

  Specify the corner name to be considered. If not specified, the default corner will be considered. Use the keyword ``all`` to sweep all the available corners. 

.. option:: --benchmark <string>

  Specify the name of the benchmark to be considered.

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on power analysis

.. option:: --dryrun

  .. warning:: This is ONLY for debugging use

  When enabled, only scripts of power analysis will be generated. Third party tool will not be run

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is ``0``, leading to minimum logging messages.

.. option:: --clock_freq <int>

  Specify the clock frequency to be applied. The clock frequency is expected to be provided in ``MHz``. If not specified, a default value 100 MHz is used.
  
.. option:: --netlist_type <string>

  Specify the type of netlist to be considered when running STA. Available options are [ ``gl`` | ``pl`` ]. By default, ``gl`` is considered
