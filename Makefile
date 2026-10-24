# 
# Top Makefile
# ------------
#
# Top-level makefile to compile the codebase
#
# Following options are available
# 
# .. option:: BUILD_TYPE=<string>
#
#  Pick the type of compilation. Can be either ``release`` or ``debug``. By default, release mode is selected (full optimization on runtime).
#
# .. option:: CMAKE_FLAGS=<string>
#
#  Force compilation flags for CMake to generate Makefiles

BUILD_TYPE ?= release
# Convert to lower case for consistency
BUILD_TYPE := $(shell echo ${BUILD_TYPE} | tr '[:upper:]' '[lower]')
# Trim any _pgo or _strict in the build type name (since it would not match any of CMake's standard build types
CMAKE_BUILD_TYPE := $(shell echo ${BUILD_TYPE} | sed 's/_\?pgo//' | sed 's/_\?strict//')

# Set build with graphviz or not
BUILD_WITH_GRAPHVIZ ?= ON
# Set build with graphical user interface or not
BUILD_WITH_GUI ?= ON

BUILD_IPO ?= ON
ifeq ($(BUILD_IPO), ON)
CMAKE_FLAGS += -DARCXPRESS_IPO_BUILD=ON
else
CMAKE_FLAGS += -DARCXPRESS_IPO_BUILD=OFF
endif

ifeq ($(BUILD_WITH_GRAPHVIZ), ON)
CMAKE_FLAGS += -DARCXPRESS_WITH_GRAPHVIZ=ON
else
CMAKE_FLAGS += -DARCXPRESS_WITH_GRAPHVIZ=OFF
endif

ifeq ($(BUILD_WITH_GUI), ON)
CMAKE_FLAGS += -DARCXPRESS_WITH_GUI=ON
else
CMAKE_FLAGS += -DARCXPRESS_WITH_GUI=OFF
endif

INSTALL_DOC ?= ON
INSTALL_DOC := $(shell echo ${INSTALL_DOC} | tr '[:upper:]' '[lower]')
ifeq ($(INSTALL_DOC),OFF)
CMAKE_FLAGS += -DINSTALLER_WITH_DOC=OFF
endif

# Only for lccgen package currently
# 1. Detect if it's Windows or Linux
ifeq ($(OS),Windows_NT)
    OS_NAME := Windows
else
    # 2. If Linux, determine the distribution
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        # Use lsb_release or os-release to find Ubuntu vs CentOS
        DISTRO := $(shell lsb_release -si 2>/dev/null || \
                   grep -Po '(?<=^ID=).*' /etc/os-release | tr -d '"' || \
                   echo "Unknown")
        ifeq ($(DISTRO),Ubuntu)
            OS_NAME := Ubuntu
        else ifeq ($(DISTRO),centos)
            OS_NAME := CentOS
        else
            OS_NAME := Linux
        endif
    endif
endif

#Detect the operating system
UNAME_S := $(shell uname -s)
# Cmake generator has to be Ninja on MSVC
CMAKE_GEN = Unix Makefiles
ifeq ($(OS),Windows_NT)
CMAKE_GEN = Ninja
# Msys2 can still use Linux gcc
ifeq ($(MSYSTEM),UCRT64)
CMAKE_GEN = Unix Makefiles
endif
endif
ifeq ($(UNAME_S),Darwin)
CMAKE_GEN = Ninja
endif

CMAKE_BUILD_CONFIG=
CPACK_CONFIG_TYPE=
ifeq ($(CMAKE_GEN),Ninja)
CPACK_CONFIG_TYPE=-C ${CMAKE_BUILD_CONFIG_VAL}
CMAKE_BUILD_CONFIG=--config ${CMAKE_BUILD_CONFIG_VAL}
endif



# Allow users to pass parameters to cmake, without defining build types
# e.g. make CMAKE_FLAGS="-DCMAKE_CXX_COMPILER=g++-9'
override CMAKE_FLAGS := -G '${CMAKE_GEN}' -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${CMAKE_FLAGS}

# -s : Suppresss makefile output (e.g. entering/leaving directories)
# --output-sync target : For parallel compilation ensure output for each target is synchronized (make version >= 4.0)
MAKEFLAGS := -s

# Directory to build the codes
# Branch on OS
ifeq ($(OS),Windows_NT)
ifeq ($(MSYSTEM),UCRT64)
SOURCE_DIR := $(PWD)
else
SOURCE_DIR := $(shell powershell -NoProfile -Command "(Get-Location).Path")
SOURCE_DIR := $(subst \,/,$(SOURCE_DIR))
endif
else
SOURCE_DIR := $(PWD)
endif
BUILD_DIR ?= build

CMAKE_GOALS = all
INSTALLER_TYPE=STGZ
SUBMODULE=
NUM_JOBS = 8
# The default boot config for Arkangel which build from source codes
REGTEST_INSTALL = OFF
ARCXPRESS_BOOT_CONFIG_FILE = ${PWD}/etc/boot_config/build.xml
ifeq (${REGTEST_INSTALL}, ON)
ARCXPRESS_BOOT_CONFIG_FILE = ${PWD}/etc/boot_config/install.xml
endif

# Find CMake command from system variable, otherwise use a default one
#Check for the cmake executable
CMAKE_COMMAND := $(shell command -v cmake 2> /dev/null)
ifeq ($(OS),Windows_NT)
ifneq ($(MSYSTEM),UCRT64)
CMAKE_COMMAND := cmake.exe
endif
endif

# Define executables
PYTHON_EXEC ?= python3
CLANG_FORMAT_EXEC ?= clang-format
XML_FORMAT_EXEC ?= xmllint
PYTHON_FORMAT_EXEC ?= black
YAMLLINTER_EXEC=${PWD}/etc/utils/yamllint/yaml_linter.py

VERSION_FILE = VERSION.md
TAGGED_COMMIT_FILE = .TAGGED_COMMIT
VERSION_BUMP_TYPE = minor
FORCE_COMMIT_VERSION_UPDATE = off
INFRA_ROOT = ${PWD}/utils/refact_infra/

# Reg test options
REG_TEST_TYPE = basic
REG_TEST_SELECT = all
REG_TEST_DRYRUN = OFF
REG_TEST_FLAGS = 
ifeq ($(REG_TEST_DRYRUN),ON)
REG_TEST_FLAGS += --dryrun
endif
# Suggest to increase when the machine is slow in disk read/write
REG_TEST_NEW_THREAD_WAIT_TIME=1

# For unit tests only
# TODO: May move unit tests to separated directory
UNIT_TEST_OUTPUT_DIR = ${PWD}/_unit_test_outputs
FLRPLN_BIN = ${PWD}/${BUILD_DIR}/libs/libfloorplan/fpga_floorplan_kisok
FLRPLN_BASIC_INPUT_DIR = ${PWD}/libs/libfloorplan/example/homo
FLRPLN_STRONG0_INPUT_DIR = ${PWD}/libs/libfloorplan/example/hetero
FLRPLN_STRONG1_INPUT_DIR = ${PWD}/libs/libfloorplan/example/hetero_mini
FLRPLN_STRONG2_INPUT_DIR = ${PWD}/libs/libfloorplan/example/sd_ntwk

# Put it first so that "make" without argument is like "make help".
export COMMENT_EXTRACT

# Put it first so that "make" without argument is like "make help".
help:
	@${PYTHON_EXEC} -c "$$COMMENT_EXTRACT"

.PHONY: help

checkout_build_submodule: 
# Checkout a submodule for code build only. Specify the path to the submodule through option SUBMODULE=<string>. For example, SUBMODULE=thirdpart/OpenFPGA. Note that this step will require the submodule has a Makefile with a valid target 'make checkout_build'.
	git submodule init ${SUBMODULE}
	git submodule update --init --depth 1 ${SUBMODULE}
	currDir=$${PWD} && cd ${SUBMODULE} && make checkout_build && cd $${currDir}

checkout_build:
# Checkout the submodules required to build source codes
	@+${MAKE} checkout_build_submodule SUBMODULE=thirdparty/OpenFPGA
	@+${MAKE} checkout_build_submodule SUBMODULE=thirdparty/Yosuf

prebuild:
# Run cmake to generate Makefile under the build directory, before compilation
	@mkdir -p ${BUILD_DIR}
	echo "cd ${BUILD_DIR} && ${CMAKE_COMMAND} ${CMAKE_FLAGS} ${SOURCE_DIR}"
	cd ${BUILD_DIR} && ${CMAKE_COMMAND} ${CMAKE_FLAGS} ${SOURCE_DIR}

compile: prebuild
# Compile the code base. By default, all the targets will be compiled
# Following options are available
# .. option:: CMAKE_GOALS
#
#   Define the target for cmake to compile. for example, ``cmake_goals=openfpga`` indicates that only openfpga binary will be compiled
	echo "Building target(s): ${CMAKE_GOALS}" && \
	echo "${CMAKE_COMMAND} --build ${BUILD_DIR} ${CMAKE_BUILD_CONFIG} --target ${CMAKE_GOALS}"
	+${CMAKE_COMMAND} --build ${BUILD_DIR} ${CMAKE_BUILD_CONFIG} --target ${CMAKE_GOALS}

installer:
# Create the package for users to install on their computer with pre-built binaries
#
# Following options are available
# .. option:: INSTALLER_TYPE
#
#   Define the type of installer, can be [STGZ|DEB|IFW] (default: STGZ). for example, ``INSTALLER_TYPE=DEB`` indicates to create a DEB package
	cpack --config ${BUILD_DIR}/CPackConfig.cmake -G ${INSTALLER_TYPE}

list_cmake_targets: prebuild
# Show the targets available to be built, which can be specified through ``CMAKE_GOALS`` when compile
	cd ${BUILD_DIR} && make help && cd -

all: checkout_build
# A shortcut command to run checkout and compile in serial
	@+${MAKE} compile

format-cpp:
# Format all the C/C++ files under this project, excluding submodules
	for f in `find libs/lib* core -iname *.cpp -o -iname *.hpp -o -iname *.c -o -iname *.h`; \
	do \
	${CLANG_FORMAT_EXEC} --style=file -i $${f} || exit 1; \
	done

check-format-cpp:
# Check if all the C/C++ files are in the expected format
	${INFRA_ROOT}/scripts/check-format.sh -cpp

format-xml:
# Format all the XML files under this project, excluding submodules
	for f in `find etc/device/*/arch etc/device/*/flow_conf etc/device/*/craft_files etc/device/*/openfpga_settings libs/lib*/example -iname *.xml`; \
	do \
	XMLLINT_INDENT="  " && ${XML_FORMAT_EXEC} --format $${f} --output $${f} || exit 1; \
	done

check-format-xml:
# Check if all the XML files are in the expected format
	${INFRA_ROOT}/scripts/check-format.sh -xml

format-py:
# Format all the python scripts under this project, excluding submodules
	for f in `find test/scripts etc/utils/pin_table_gen etc/utils/bus_group_gen etc/utils/netlist_makeup -iname *.py`; \
	do \
	${PYTHON_FORMAT_EXEC} $${f} --line-length 100 || exit 1; \
	done

check-format-py:
# Check if all the python files are in the expected format
	${INFRA_ROOT}/scripts/check-format.sh -py

format-yaml:
# Format all the YAML under this project, excluding submodules
	${PYTHON_EXEC} ${YAMLLINTER_EXEC} 'etc/device/*/arch' --fix --no-backup || exit 1; \

check-format-yaml:
# Check if all the YAML files are in the expected format
	${INFRA_ROOT}/scripts/check-format.sh -yaml

format-all: format-cpp format-xml format-py format-yaml
# Format all the C/C++, XML and Python codes

clean:
# Remove current build results
# One exception is the arcxpress gui, where the working directory is the subumodule itself
	rm -rf ${BUILD_DIR}
	rm -rf projects

update_version:
# Update the patch count in the version number
	echo "======== Bump up patch count in the version number ========"; \
	${PYTHON_EXEC} ${INFRA_ROOT}/scripts/version_updater.py --version_file ${VERSION_FILE} --tagged_commit ${TAGGED_COMMIT_FILE} --force_commit ${FORCE_COMMIT_VERSION_UPDATE}	

release_version:
# Update the patch count in the version number
	echo "======== Bump up release in the version number ========"; \
	${PYTHON_EXEC} ${INFRA_ROOT}/scripts/version_updater.py --version_file ${VERSION_FILE} --tagged_commit ${TAGGED_COMMIT_FILE} --release --bump_type ${VERSION_BUMP_TYPE}	

generate_initial_tagged_commit:
# Create the first version of tagged commit file, used for version update
	git rev-list --max-parents=0 --abbrev-commit HEAD > ${TAGGED_COMMIT_FILE}

unit_test:
# Run unit tests
	${FLRPLN_BIN} --cpp 0.19 --sc_height 0.98 --strategy ${FLRPLN_BASIC_INPUT_DIR}/floorplan_strategy.xml --tile_qor ${FLRPLN_BASIC_INPUT_DIR}/tile_qor.csv --tile_ref ${FLRPLN_BASIC_INPUT_DIR}/tile_reference.yaml --shaping_result ${UNIT_TEST_OUTPUT_DIR}/O0-homo-shaping.txt --report ${UNIT_TEST_OUTPUT_DIR}/O1-homo-floorplan_result.csv --shrink_factor 0.9 --precision 3 --verbosity 2 || exit 1;
	${FLRPLN_BIN} --cpp 0.19 --sc_height 0.98 --strategy ${FLRPLN_STRONG0_INPUT_DIR}/floorplan_strategy.xml --tile_qor ${FLRPLN_STRONG0_INPUT_DIR}/R2_tile_qor.csv --tile_ref ${FLRPLN_STRONG0_INPUT_DIR}/R0-tile_reference.yaml --shaping_result ${UNIT_TEST_OUTPUT_DIR}/O0-hetero-shaping.txt --report ${UNIT_TEST_OUTPUT_DIR}/O1-hetero-floorplan_result.csv --shrink_factor 0.9 --precision 3 --verbosity 2 || exit 1;
	${FLRPLN_BIN} --cpp 0.21 --sc_height 1.50 --strategy ${FLRPLN_STRONG1_INPUT_DIR}/floorplan_strategy.xml --tile_qor ${FLRPLN_STRONG1_INPUT_DIR}/R2-tile_cell_area.csv --tile_ref ${FLRPLN_STRONG1_INPUT_DIR}/R0-tile_reference.yaml --shaping_result ${UNIT_TEST_OUTPUT_DIR}/O0-hetero-shaping.txt --report ${UNIT_TEST_OUTPUT_DIR}/O1-hetero-floorplan_result.csv --floorplan_signal_distribution_network --signal_distribution_network_report_file ${UNIT_TEST_OUTPUT_DIR}/O2-floorplan_sig_distr_ntwk.xml --io_coordinates ${FLRPLN_STRONG1_INPUT_DIR}/R5-io_coordinates.xml --io_metal_layer M2 --io_physical_location_report ${UNIT_TEST_OUTPUT_DIR}/O3-io_phy_loc_rpt.txt --tile_pin_location ${FLRPLN_STRONG1_INPUT_DIR}/R1-fabric_pin_location_tiles.xml --tile_pin_metal_layer M1 --tile_pin_physical_location_report ${UNIT_TEST_OUTPUT_DIR}/O4-tile_pin_phy_loc_rpt.txt --shrink_factor 0.9 --precision 3 --verbosity 2 || exit 1;
	${FLRPLN_BIN} --cpp 0.21 --sc_height 1.50 --strategy ${FLRPLN_STRONG2_INPUT_DIR}/floorplan_strategy.xml --tile_qor ${FLRPLN_STRONG2_INPUT_DIR}/R2_tile_qor.csv --tile_ref ${FLRPLN_STRONG2_INPUT_DIR}/R0-tile_reference.yaml --shaping_result ${UNIT_TEST_OUTPUT_DIR}/O0-hetero-shaping.txt --report ${UNIT_TEST_OUTPUT_DIR}/O1-hetero-floorplan_result.csv --floorplan_signal_distribution_network --signal_distribution_network_report_file ${UNIT_TEST_OUTPUT_DIR}/O2-floorplan_sig_distr_ntwk.xml --io_coordinates ${FLRPLN_STRONG2_INPUT_DIR}/R5-fpga_io_location.xml --io_metal_layer M2 --io_physical_location_report ${UNIT_TEST_OUTPUT_DIR}/O3-io_phy_loc_rpt.txt --tile_pin_location ${FLRPLN_STRONG2_INPUT_DIR}/R1-fabric_pin_location_tiles.xml --tile_pin_metal_layer M1 --tile_pin_physical_location_report ${UNIT_TEST_OUTPUT_DIR}/O4-tile_pin_phy_loc_rpt.txt --result_figure ${UNIT_TEST_OUTPUT_DIR}/O5-floorplan.png --figure_scale_factor 4 --dominant_tile tile_1__2_ --shrink_factor 0.9 --precision 3 --verbosity 2 || exit 1;

reg_test:
# Run regression tests. Use REG_TEST_TYPE=[checkin|basic|basic_alkaidL_only|strong_alkaidL|strong_alkaidC|strong_alkaidD] to select the testcases. Use REG_TEST_SELECT=[task1|task2|...] to select single task run. Enable dryrun mode using REG_TEST_DRYRUN=ON
	${PYTHON_EXEC} ${PWD}/test/scripts/run_arcxpress_test.py \
	--config ${PWD}/test/common/test_config.yaml \
	--task ${PWD}/test/common/${REG_TEST_TYPE}_tests.yaml \
	--arcxpress_root ${PWD} \
	--arcxpress_build_path ${PWD}/build/core \
	--arcxpress_boot_config ${ARCXPRESS_BOOT_CONFIG_FILE} \
	--task_select ${REG_TEST_SELECT} \
	--new_thread_wait_time ${REG_TEST_NEW_THREAD_WAIT_TIME} \
	--j ${NUM_JOBS} \
	--output_html_file ${PWD}/test/reports/${REG_TEST_TYPE}_reg_test_status_report.html \
	${REG_TEST_FLAGS} || exit 1;

# Functions to extract comments from Makefiles
define COMMENT_EXTRACT
import re
with open ('Makefile', 'r' ) as f:
    matches = re.finditer('^([a-zA-Z-_]*):.*\n#(.*)', f.read(), flags=re.M)
    for _, match in enumerate(matches, start=1):
        header, content = match[1], match[2]
        print(f"  {header:10} {content}")
endef
