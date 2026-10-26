.. _arcxpress_netlist_dev_commands:

Netlist Development Commands
----------------------------

.. _arcxpress_netlist_dev_commands_generate_netlist:

generate_netlist
~~~~~~~~~~~~~~~~

Generate the RTL-level netlists for the eFPGA architecture.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --arch_run <string>

  Specify the run id of architecture development. See details in :ref:`arcxpress_arch_dev_commands`. Note that the architecture development should be finished successfully. If not specifed, the latest run will be automatically consideried.

.. option:: --netlist_makeup

  Specify if the optimization should be applied or not. By default, it is off.

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.

.. _arcxpress_netlist_dev_commands_optimize_netlist:

optimize_netlist
~~~~~~~~~~~~~~~~

Optimize the RTL-level netlists for the eFPGA architecture and generate gate-level netlists based on the selected PDK.

.. note:: Netlist synthesis will be applied. It requires Synopsys Design Compiler 2019.04 or later version to be installed.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on synthesis

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner.

.. option:: --strategy <string>

  Specify the custom synthesis strategy file when the default synthesis strategy cannot satisfy your needs. See details w.r.t. the format of synthesis strategy file in :ref:`file_format_synth_strategy_file`

.. option:: --stage <string>

  .. warning:: This is ONLY for debugging use

  Specify the stage of the optimization to be run. By default, all the stages will be executed. Can be [ ``gen_gl`` ]

.. option:: --dryrun

  .. warning:: This is ONLY for debugging use

  When enabled, only scripts of netlist synthesis and analyze will be generated. Third party tool will not be run

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.

.. _arcxpress_netlist_dev_commands_coarse_analyze_netlist:

coarse_analyze_netlist
~~~~~~~~~~~~~~~~~~~~~~

Run a coarse analyze on the area of the gate-level netlist.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on synthesis

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner.

.. option:: --dryrun

  .. warning:: This is ONLY for debugging use

  When enabled, only scripts of netlist synthesis and analyze will be generated. Third party tool will not be run

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.

.. _arcxpress_netlist_dev_commands_coarse_floorplan:

coarse_floorplan
~~~~~~~~~~~~~~~~

Run a coarse floorplan on the area of the eFPGA based on the gate-level netlist.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --report_file <string>

  Specify the file path to flooplanning report. If not specified, the report will only be printed out on screen

.. option:: --shaping_report_file <string>

  Specify the output path for shaping.txt, which stores floorplanning instances and their coordinates. If not specified, the file is written to the netlist report directory

.. option:: --precision <int>

  Control the precision of numbers to be shown in report. By default, it is `2``, which results in showing ``2.05`` for number ``2.0501``

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.

.. option:: --strategy <string>
  
  Specify the XML file containing the custom floorplan strategy,If not specified, a default strategy will be applied.

.. _arcxpress_netlist_dev_commands_extract_netlist_qor:

extract_netlist_qor
~~~~~~~~~~~~~~~~~~~

Run a *Quality-of-Result* (QoR) extraction on the gate-level netlist, including detailed timing and power analyses.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --type <string>

  Specify the type of QoR extraction to be performed. Can be [ ``timing`` | ``leakage`` | ``config_power`` | ``timing_gclk`` | ``timing_grst`` ]. If not specified, all the types will be considered.

.. option:: --task_name <string>

  .. note:: This is mainly to ease eFPGA architecture developer in debugging. Please avoid to use unless necessary.

  Specify the name of QoR extraction task to be performed. Must be a valid one specified in the selected type. If not specified, all the tasks under the selected type will be considered.

.. option:: --clock_freq <float>

  Specify the clock frequency to be applied when running timing extraction on global signals. The unit is ``MHz``. If not specified, a default value of ``100MHz`` will be considered

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner. Use the keyword ``all`` to sweep all the available corners. 

.. option:: --jobs <int>

  Specify the max. number of jobs to be applied on synthesis

.. option:: --dryrun

  .. warning:: This is ONLY for debugging use

  When enabled, only scripts of netlist synthesis and analyze will be generated. Third party tool will not be run

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.
