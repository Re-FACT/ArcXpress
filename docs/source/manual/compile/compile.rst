.. _manual_compile:

How to Compile
--------------

.. note:: Before start, please ensure all the dependencies are installed on your system. See details in :ref:`manual_dependency`

Supported Operating Systems
~~~~~~~~~~~~~~~~~~~~~~~~~~~

ArcXpress Engineering is continously tested on

- Ubuntu 22.04 
- CentOS 9
- CentOS 7.9

It might work with earlier versions and other distributions.

Build Steps
~~~~~~~~~~~
OpenFPGA uses CMake to generate the Makefile scripts.
In general, please follow the steps to compile

.. code-block:: shell

  git clone 
  cd ArcXpress
  make all

.. note:: Requires gcc/g++ version >= 9 and clang version >= 11.

.. note:: cmake3.12+ is recommended to compile ArcXpress with GUI

.. note:: Require boost version > 1.75 when compile with license control. See details in :ref:`tutorial_compile_build_options`

.. note:: Recommend using ``make -j<int>`` to accelerate the compilation, where ``<int>`` denotes the number of cores to be used in compilation.

.. _tutorial_compile_build_options:

Build Options
~~~~~~~~~~~~~

General build targets are available in the top-level makefile. Call help desk to see details

.. code-block:: shell

  make help

The following options are available for a custom build

.. option:: BUILD_TYPE=<string>

  Specify the type of build. Can be either ``release`` or ``debug``. By default, release mode is selected (full optimization on runtime)

.. option:: EDITION_TYPE=<string>

  Specify the type of edition. Can be either ``evaluation`` or ``prototyping`` or ``engineering``. By default, engineering edition is selected. See details at :ref:`manual_introduction``

.. option:: BUILD_WITH_LICENSE_CONTROL=[ON|OFF]

  Enable the license control in the compilation. Once enabled, the binary exectuables will require a valid license to be launched. By default, it is ``OFF``

.. option:: INSTALL_FULL_DEVICE=[ON|OFF]

  Enable all the device modelss to be build. By default, it is ``OFF``, resulting in only alkaidL device is included.

.. option:: BUILD_WITH_GRAPHVIZ=[ON|OFF]

  Enable graphviz-related features, e.g., schematic visualizer. By default, it is ``ON``.

.. option:: BUILD_PLUGIN_SLANG=[ON|OFF]

  Specify if the Yosys-pluging on SystemVerilog parser should be built or not. BY default, it is ``ON``.

.. option:: BUILD_CUSTOM_PYTHON=[ON|OFF]

  Use a prebuilt version of Python3 instead of build the one from the source codes in this repository. By default, it is ``ON``. Ensure that the prebuild Python3 version meets the minimum requirements > 3.9.25. This option is recommended for Windows environment when Python source code is not yet adapted for the Windows platform. When enabled, ensure to set the external python3 path and included libraries in the boot configuration

.. option:: BUILD_IPO=[ON|OFF]

  Enable interprocedural compiler optimization or not. By default, it is ON

.. option:: BUILD_ON_EXTERNAL_YOSYS=[ON|OFF]

  Use a prebuilt version of Yosys instead of build the Yosys from the source codes in this repository. By default, it is ``OFF``. Ensure that the Yosys version meets the minimum requirements > 0.56. This option is recommended for Windows environment when Yosys source code is not yet adapted for the Windows platform. When enabled, use option``EXTERNAL_YOSYS_BIN`` to specify the path to the directory which contains Yosys binaries. For example,

.. code-block::

  # The Yosys binary should be under /mingw64/bin/yosys.exe
  make all BUILD_ON_EXTERNAL_YOSYS=ON EXTERNAL_YOSYS_BIN=/mingw64


.. option:: CMAKE_FLAGS=<string>

  Force build flags to CMake. The following flags are available

  - ``DARKANGEL_WITH_VERSION=[ON|OFF]``: Enable/Disable the build of version number. When disabled, version number will be displayed as an empty string.

.. option:: CMAKE_GOALS=<string>

  Specify the build target for CMake system. For example, ``cmake_goals=arkangel`` indicates that only openfpga binary will be compiled. For a detailed list of targets, use ``make list_cmake_targets`` to show. By default, all the build targets will be included. 

Installer
~~~~~~~~~

To create an installer, run the following command:

.. code-block::

  make prebuild INSTALL_DOC=ON INSTALL_CENTOS7P9_ENV=OFF
  make installer INSTALLER_TYPE=IFW -j32

when accomplished, a GUI installer will be generated. 

Note that the following options are available during the prebuild step

.. option:: INSTALL_DOC=[ON|OFF]

  Enable the documentation to be packed into the installer or not. By default, it is enabled.

.. option:: INSTALL_CENTOS7P9_ENV=[ON|OFF]

  .. warning:: This is only applicable to a specific CentOS 7.9 machine!!! Please double check with your manager before moving on.

  Enable a custom envoirnment on CentOS/Redhat 7.9 system image to be packed into the installer or not. By default, it is disabled.

.. option:: INSTALL_MINGTW64_ENV=[ON|OFF]

  .. note:: When enabled, please define the root directory of the Ming64 system in a envoirnment variable ``CUSTOM_ENV_ROOT``

  .. warning:: This is only applicable to a specific windows machine!!! Please double check with your manager before moving on.

  Enable a custom envoirnment on a Mingw64 on a Windows system image to be packed into the installer or not. By default, it is disabled.

.. note:: Ensure the QtIFW is installed, and the ``QTIFW_HOME`` is properly set.

For example,

.. code-block::

  export QTIFW_HOME=/opt/Qt/QtIFW-4.8.1

