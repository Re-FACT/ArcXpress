.. _launch_arkangel_shell:

Launch ArcXpress Shell
---------------------

ArcXpress employs a shell-like user interface, in order to integrate all the tools in a well-modularized way.
To launch ArcXpress shell, users can choose two modes ``interactive`` or ``script``.

.. option::	--interactive or -i

  Launch ArcXpress in interactive mode where users type-in command by command and get runtime results

.. option::	--file or -f

  Launch ArcXpress in script mode where users write commands in scripts and FPGA will execute them

.. option::	--batch_execution or -batch

  Execute ArcXpress script in batch mode. This option is only valid for script mode.

  - If in batch mode, ArcXpress will abort immediately when fatal errors occurred.
  - If not in batch mode, ArcXpress will enter interactive mode when fatal errors occurred.

.. option:: --root_path

  Specify the root path where ArcXpress is installed. This is required.

.. option:: --boot_config

  Specify the file path to the boot configuration for ArcXpress. This is required. See details in :ref:`file_format_boot_config_file`

.. option::	--version or -v

  Print version information of ArcXpress

.. option::	--help or -h
	
  Show the help desk

