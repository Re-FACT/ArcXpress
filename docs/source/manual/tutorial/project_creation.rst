.. _tutorial_project_creation:

Project Creation
================

This tutorial provides a step-by-step walkthrough for creating and loading an ArcXpress tape-out project. Starting from scratch, you will learn how to create a test case and then execute the three key commands: ``create_tapeout``, ``save_tapeout``, and ``load_tapeout``. By following this guide, users can understand the entire execution flow of these commands and learn how to determine whether each operation has completed successfully.

.. note::

   ArcXpress uses *Project* as the fundamental design unit.
   Only **one project** can be created or loaded in a single ArcXpress session.
   To work with multiple projects, please launch separate ArcXpress sessions.


Project File Structure and Configuration
----------------------------------------

To facilitate file management, AAEE adopts the following format to run management files. You can also name the files yourself and place them, as long as the file path is correct.The AAEE flow requires a well-organized directory structure to manage configurations, scripts, and results. Below is the recommended structure for running AAEE test cases.

Directory Structure
-------------------

The following diagram illustrates the typical structure of an AAEE test case directory:

::

    test_case/                    # Main test case directory (can be created anywhere outside installation)
    │
    ├── config/                   # Configuration directory
    │   ├── project.xml           # Project configuration file
    │   └── run.arcxpress          # AAEE command script

File Descriptions
-----------------

**test_case/** - *Root Directory*
   This is the main directory for your AAEE test case.
   Can be created anywhere outside the AAEE installation directory.
   Contains all project files, configurations, inputs, and results.

**config/** - *Configuration Directory*
   Contains all configuration files required by AAEE.
   Essential for setting up and customizing the design flow.

**project.xml** - *Project Configuration File*
   XML file defining project parameters:
   
   - **FPGA device selection**
   - **PDK configuration**
   - **Corner selection**
   - **Benchmark circuit paths**
   - **Architecture template file path**
   - **Output directory settings**

**run.arcxpress** - *AAEE Command Script*
   Contains the sequence of AAEE commands to execute.
   Defines the complete flow from synthesis to netlist generation.
   Can include multiple tool invocations and data processing steps.

.. note::
   This run.arcxpress script is optional; you can also execute the commands one by one in interactive mode.


Prerequisites
-------------

Before starting this tutorial, ensure that:

- ArcXpress (aaee) is properly installed
- Required environment variables are correctly configured
- You have write permission for the project working directory

Preparing the Project Configuration File
----------------------------------------

ArcXpress projects are defined using a **tape-out project XML configuration file**.
The following is a complete example of a ``project.xml`` file used in this tutorial.

.. code-block:: xml

   <?xml version="1.0"?>
   <tapeout name="alkaid_ex" family="alkaidL" path="${PWD}/alkaid_exto">
     <stamp>
       <author>RapidFlex</author>
       <dates>
         <created>Sat Dec 31 03:14:41 PM PST 2022</created>
         <last_modified>Sat Dec 31 03:14:41 PM PST 2022</last_modified>
       </dates>
       <tool_version>0.0.15-dev+023fa82</tool_version>
     </stamp>
     <!-- Specify the PDK configuration file -->
     <pdk>${ROOT_PATH}/etc/pdk_config/s40ulp.xml</pdk>
     <!-- Specify the benchmark configuration files
          The default benchmark is the one will be used for all the flows except benchmarking
       -->
     <benchmarks default="and2">
       <file>${ROOT_PATH}/etc/benchmark_config/checkin.xml</file>
       <file>${ROOT_PATH}/etc/benchmark_config/counter.xml</file>
     </benchmarks>
     <!-- Define your custom corner if it does not exist in the built-in corner.
          User may select to overwrite the default corner here
          User may define any duplicated corner in the same name as built-in corners, the user-defined one has a higher priority
       -->
     <corner_map default="tt_25C_v0p9">
     </corner_map>
     <arch_template_file>${ROOT_PATH}/etc/device/ALKDL/flow_conf/efpga_arch_template.xml</arch_template_file>
     <!-- Record each run and the status
          frozen means a protected run where no changes are allowed unless user flips the flag
          failed runs will block the downstream steps
       -->
   </tapeout>

Detailed Specifications of the `project.xml` 
--------------------------------------------

This document provides a detailed explanation of the key parameters defined in the `project.xml` configuration file, which governs the AAEE flow execution.

1. Workspace Creation Location
------------------------------
*   **Configuration**: The ``project.xml`` file defines a ``tapeout`` section.
*   **Result Location**: All generated results (netlists, reports, intermediate files) from the AAEE flow are stored within a subdirectory named ``alkaid_exto``, located inside the ``config`` directory.
*   **Naming Convention**: Files generated at each stage of the flow are prefixed with ``alkaid_ex`` for identification.

2. Selected FPGA Device & Location of Alternatives
--------------------------------------------------
*   **Selected Device**: The specific FPGA device used is defined within the ``family`` tag under the ``tapeout`` section in ``project.xml``.
*   **Location of Alternative Devices**: A comprehensive list of all supported FPGA devices can be found in the `/etc/device/` directory within the AAEE installation package path.

3. Selected PDK Configuration & Location of Alternatives
--------------------------------------------------------
*   **Selected PDK**: The PDK (Process Design Kit) is specified by the ``pdk`` tag in ``project.xml``.
*   **Location of Alternative PDKs**: All available PDK configurations are stored in the ``/etc/pdk_config/`` directory within the AAEE installation package path.

4. Benchmarks Considered
------------------------
*   **Configuration**: The ``benchmarks`` section in ``project.xml`` is used to specify the benchmark circuits for the flow.
*   **Location of Benchmark Files**: The corresponding benchmark circuit files can be found in the ``/etc/benchmark_config/`` directory within the AAEE installation package path.

5. Corner Map Configuration & Default Selection
-----------------------------------------------
*   **Corner Map Used**: The ``corner_map`` definition in ``project.xml`` specifies the Process-Voltage-Temperature (PVT) corner used for analysis (e.g., ``tt_25C_v0p9``).
*   **Corner Definition Source**: The components of the corner name (e.g., ``tt``, ``25C``, ``v0p9``) are derived from the corresponding PDK configuration file (e.g., ``s40ulp.xml``):
    *   ``tt`` originates from ``process="tt"``
    *   ``25C`` originates from ``temp="25"``
    *   ``v0p9`` originates from ``vdd="0.9"`` (formatted as ``v0p9``)
*   **Location of Corner Definitions**: The complete mapping and definitions for all available corners are contained within the specific PDK configuration file referenced by the project.

6. Architecture Template File & Location
----------------------------------------
*   **Configuration**: The ``arch_template_file`` tag in ``project.xml`` specifies the path to the FPGA architecture template file.
*   **File Location**: The architecture template file (e.g., ``efpga_arch_template.xml``) is located within the installation package under the path: ``/etc/device/<device_name>/flow_conf/``, where ``<device_name>`` corresponds to the selected FPGA device.

If you have any questions about the parameter definitions in the project xml file, please refer to :ref:`file_format_project_file`

Creating a New Project
----------------------

Before you can create a new tape-out project, you need to **launch the ArcXpress shell**.

Launch ArcXpress
~~~~~~~~~~~~~~~~

ArcXpress provides a shell-like interface to integrate all tools in a modular way.  
You can launch it in **interactive mode** (type commands one by one), which is useful for step-by-step exploration and debugging.

**Interactive mode** (``--interactive`` or ``-i``):

- Launch ArcXpress and type commands one by one.
- Useful for step-by-step exploration and debugging.

.. code-block:: bash

   aaee -i

.. note::

   For more details on launching ArcXpress and parameter definitions, refer to :ref:`launch_arcxpress_shell`.



Execution Flow of ``create_tapeout``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run the project creation command:

   .. code-block:: bash

      arcxpress> create_tapeout --config_file ./config/project.xml

When the ``create_tapeout`` command is executed, ArcXpress will
produce console logs in the following order.

Successful Creation
~~~~~~~~~~~~~~~~~~~

A **complete and successful execution** of ``create_tapeout`` has the following
characteristics:

- No error messages are reported
- All steps complete successfully
- The final log message is:

  .. code-block:: text

     Reading configuration file '<your_config_file>'...
     Validating configuration file...
     Done
     Initializing tapeout project '<project_name>'...

This message indicates that the tape-out project has been successfully created
and initialized.

.. note::

   ArcXpress does **not** print an explicit "Success" message after project
   creation. Success is determined by the absence of errors and the successful
   completion of all execution steps.

After successfully executing ``create_tapeout``, users can choose to run commands
such as:

- ``report_corners``: Display all available corners and their details
- ``show_dashboard``: Show project dashboard with configuration summary

These commands can help validate the project setup and provide insights into
the configured parameters before proceeding with further steps.

If project creation fails, ArcXpress will display clear error messages, such as:

- Configuration file does not exist
- Invalid or malformed configuration file
- Missing or unsupported dependencies

In these cases, the project will not be created, and the reported issues must be
resolved before retrying the ``create_tapeout`` command. If you want to learn more, please refer to :ref:`create_tapeout <arcxpress_project_commands>`.

Execution Flow of ``save_tapeout``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After making modifications to a project, use the ``save_tapeout`` command to
save the updated project data to the configuration file.

Run the project saving command:

.. code-block:: bash

   arcxpress> save_tapeout --config_file ./config/project.xml

When the ``save_tapeout`` command is executed successfully, ArcXpress will produce
console logs in the following order:

.. code-block:: text

   Saving results of each step and run...
   Saving results of step '<step_name>'...
       For run '<run_name>'...
   Done
   Saving configuration file '<config_file>'...
   Done


Successful Saving
~~~~~~~~~~~~~~~~~~~

A **complete and successful execution** of ``save_tapeout`` has the following
characteristics:

- No error messages are reported
- All steps complete successfully
- The final log message is:

  ``Saving configuration file '<config_file>'... Done``

This message indicates that the tape-out project has been successfully saved
to the configuration file.

.. note::

   ArcXpress does **not** print an explicit "Success" message after project
   saving. Success is determined by the absence of errors and the successful
   completion of all execution steps.

If project saving fails, ArcXpress will display clear error messages, such as:

- Insufficient permissions to write to the configuration file
- Invalid project data

In these cases, the project data will not be saved, and the reported issues must be
resolved before retrying the ``save_tapeout`` command. If you want to learn more, please refer to :ref:`save_tapeout <arcxpress_project_commands>`.

Execution Flow of ``load_tapeout``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To continue working on an existing tape-out project, use the
``load_tapeout`` command.

.. code-block:: bash

   arcxpress> load_tapeout --config_file ./config/project.xml


When the ``load_tapeout`` command is executed successfully, ArcXpress will produce
console logs similar to the following:

.. code-block:: text

   Reading configuration file '<your_config_file>'...
   Validating configuration file...
   Done
   Checking results of each step and run...
   Checking results of step '<step_name>'...
       For run '<run_name>'...
   Done

Successful Loading
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Unlike ``create_tapeout``, the ``load_tapeout`` command does **not** print a
distinct final message such as ``Initializing tapeout project``.

A **successful load operation** is indicated by:

- No error messages reported during execution
- All validation and result-checking steps completed
- The final output ends with ``Done``

This confirms that the project has been loaded successfully into the current
ArcXpress session.

.. note::

   ``load_tapeout`` does not display explicit messages such as "Success" or
   "Loaded successfully". Completion without errors indicates success.

If issues occur during project loading, ArcXpress will report clear error
messages, such as:

- Configuration file does not exist
- Invalid configuration file format
- Missing dependent files or resources

In these cases, the project will not be loaded, and the reported errors must be
resolved before retrying the ``load_tapeout`` command. If you want to learn more, please refer to :ref:`load_tapeout <arcxpress_project_commands>`.

Next Steps
----------

After successfully creating or loading a project, you can proceed with:

- Pre-architecture analysis
- Architecture development
- Netlist generation and optimization
- Netlist validation
- Design Verification

.. note::

   While this tutorial focuses on using **interactive mode** (executing commands one by one), you can alternatively choose to use **script mode** to execute commands from a script file. In script mode, you can run multiple commands sequentially by specifying the script file, for example:

   .. code-block:: bash

      aaee -f run.arcxpress  # Run commands from script file

run.arcxpress example
--------------------

The following are example commands for `run.arcxpress`:

.. code:: bash
   
   create_tapeout --config_file ./config/project.xml
   save_tapeout --config_file ./config/project.xml
   load_tapeout --config_file ./config/project.xml
