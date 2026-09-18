.. _arkangel_arch_eval_commands:

Arch Evaluation Commands
------------------------

eval_arch
~~~~~~~~~

  Evaluate the architecture by benchmarking its *Performance, Power and Area* (P.P.A.). For each benchmark, a full flow of HDL-to-Bitstream generation will be performed. If finished without any error, this is the step to generate golden bitstream, for the future use in design verification.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner. Use the keyword ``all`` to sweep all the available corners. 

.. note:: Expect long runtime and high-usage on computing resource when you choose to sweep all the corners

.. option:: --arch_run <string>

  Specify the run id of architecture development. See details in :ref:`arkangel_arch_dev_commands`. Note that the architecture development should be finished successfully. If not specifed, the latest run will be automatically consideried.

.. option:: --force_copy_arch_data

  Instead of creating a link to the architecture data, copy the data to the runtime directory. This improves the portability of project workspace but at a cost of additional disk space. This is automatically enabled on windows system.

.. option:: --report_qor <string>

  Specify the file path where the *Quality-of-Results* (QoR) should be written to. The file format is csv. See below for an example file which is outputted.

.. csv-table:: Example of arch evaluation QoR report file
  :file: examples/arch_eval_qor.csv
  :header-rows: 1

.. option:: --job <int>

  Specify the number of processers available to run bitstream generaton jobs in parallel. By default, only 1 processor is allowed. 

  .. note:: In Windows version, only 1 processor is always allowed. 

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is `0``, leading to minimum logging messages.

.. _arkangel_dv_commands_run_benchmark_simulation:

run_benchmark_simulation
~~~~~~~~~~~~~~~~~~~~~~~~

Run benchmark simulation based on the selected benchmark and its netlists that are generated during bitstream generation stages.

.. note:: The concepts of rtl, post_synth and post_implementation are applicable to benchmark netlists only, representing the types of netlists during HDL-to-Bitstream stages. It differs from the fabric netlist mentioned in command ``run_preconfigured_simulation``.

.. option:: --run <string>

  Specify the run id. Each run is independent from each other. If not specified, a default name will be provided. 

  .. note:: If you specify an existing run, the data will be overwritten.

.. option:: --corner <string>

  Specify the corner defined through project file. If not specified, use the default corner.  Use the keyword ``all`` to sweep all the available corners. 

.. note:: Expect long runtime and high-usage on computing resource when you choose to sweep all the corners

  Specify the time (in sec.) to wait before starting a new thread for checking simulation logs (when --jobs > 2)

.. option:: --simulator <string>

  Specify the simulator to be used when running simulation. Available options are [ ``vcs`` | ``modelsim`` | ``icarus`` ]. By default, ``vcs`` is considered.

.. option:: --benchmark <string>

  Specify the name of the benchmark to be used for application simulation.

.. option:: --type <string>

  Specify the type of simulation to be considered. Available options are [ ``rtl`` | ``post_synth`` | ``post_implementation`` ].

.. option::	--dump_waveform <string>

  Specify if the waveform file should be outputted when running simulation. Available options are [ none | fsdb | vcd ]. By default is none, implicating no waveform files are outputted.

.. option:: --verbosity <int>

  Control the verbosity of the messages. For debugging usage, recommend to set to ``1``. By default, it is ``0``, leading to minimum logging messages.


