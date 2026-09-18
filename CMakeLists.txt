cmake_minimum_required(VERSION 3.5)

# Include user-defined functions
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
# Auto-enable ccache
include(SupportCcache)
# Include user-defined functions
include(FilesToDirs)
include(SwigLib)

## Set the default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build: None, Debug, Release, RelWithDebInfo, MinSizeRel"
        FORCE)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

## Set the default edition type if not specified
set(ARKANGEL_EDITION_TYPE "engineering" CACHE STRING "Choose the type of edition: evaluation, prototyping, engineering")
message(STATUS "EDITION_TYPE: ${ARKANGEL_EDITION_TYPE}")

if(${ARKANGEL_EDITION_TYPE} STREQUAL "evaluation")
    add_definitions("-DARKANGEL_EVAL_EDITION")
elseif(${ARKANGEL_EDITION_TYPE} STREQUAL "prototyping")
    add_definitions("-DARKANGEL_PROTOTYPING_EDITION")
elseif(${ARKANGEL_EDITION_TYPE} STREQUAL "engineering")
    add_definitions("-DARKANGEL_ENGINEERING_EDITION")
else()
    message(FATAL_ERROR "Invalid type of editions: must be [evaluation|prototyping|engineering]")
endif()

# Licensed build

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds not allowed. Use the Makefile wrapper (e.g. make), or create a new build directory and call cmake manually from there (e.g. mkdir -p build && cd build && cmake .. && make). You may need to 'rm -rf CMakeCache.txt CMakeFiles' first.")
endif()

#We install to the source directory by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "default install path" FORCE)
endif()

set(ARKANGEL_IPO_BUILD "auto" CACHE STRING "Should ArkAngel be compiled with interprocedural compiler optimizations?")
set_property(CACHE ARKANGEL_IPO_BUILD PROPERTY STRINGS auto on off)
# Disable IPO on Windows MSYS2 to avoid linker errors with complex STL templates
# LTO causes "two or more sections for .gnu.lto__ZNSt..." errors on MinGW64
if (DEFINED ENV{MSYSTEM})
    message(STATUS "Disabling IPO on MSYS2 environment to avoid LTO linker conflicts")
    set(ARKANGEL_IPO_BUILD "off" CACHE STRING "Should Arkangel be compiled with interprocedural compiler optimizations?" FORCE)
endif()

#Allow the user to configure how much assertion checking should occur
set(VTR_ASSERT_LEVEL "2" CACHE STRING "VTR assertion checking level. 0: no assertions, 1: fast assertions, 2: regular assertions, 3: additional assertions with noticable run-time overhead, 4: all assertions (including those with significant run-time cost)")
set_property(CACHE VTR_ASSERT_LEVEL PROPERTY STRINGS 0 1 2 3 4)

#Create the project 
project("ArkAngel_Engineering" C CXX)

# Version number
file (STRINGS "VERSION.md" VERSION_NUMBER)
string (REPLACE "." ";" VERSION_LIST ${VERSION_NUMBER})
list(GET VERSION_LIST 0 ARKANGEL_VERSION_MAJOR)
list(GET VERSION_LIST 1 ARKANGEL_VERSION_MINOR)
list(GET VERSION_LIST 2 ARKANGEL_VERSION_PATCH)
set(ARKANGEL_VERSION_PRERELEASE "dev")

# Set the assertion level
add_definitions("-DVTR_ASSERT_LEVEL=${VTR_ASSERT_LEVEL}")

# compiler flag configuration checks
include(CheckCXXCompilerFlag)

# Options
## General options
option(ARKANGEL_WITH_VERSION "Enable version always-up-to-date when building codebase. Disable only when you do not care an accurate version number" ON)
option(BUILD_ARKANGEL_WITH_LICENSE "Build arkangel with license control" OFF)
option(ARKANGEL_WITH_GRAPHVIZ "Enable graphviz-related features when building codebase. Disable only when you do not need netlist visualizer" ON)
option(ARKANGEL_WITH_GUI "Enable GUI when building codebase. Disable only when you do not need visualizer" ON)
# Installer options
option(INSTALLER_WITH_GUI "Create an GUI installer" ON)
option(INSTALLER_WITH_DOC "Installer should include documentation" ON)
option(INSTALLER_WITH_CUSTOM_ENV "Installer should include a custom environment on Mingw64/CentOS 7.9. When enabled, the root directory of the custom enviornment should be specified in the system env variable CUSTOM_ENV_ROOT" OFF)
option(INSTALLER_WITH_FULL_DEVICE "Include alkaidC/alkaidD devices in the build" OFF)
option(BUILD_PYTHON_FROM_SOURCE, "Use an custom python3 from the source codes in this project" ON)
option(BUILD_ARKANGEL_WITH_BUILTIN_FRONTEND, "Include built-in frontend to be compiled from source codes in this project" ON)

# Options pass on to OpenFPGA
set(OPENFPGA_ENABLE_VERSION ${ARKANGEL_WITH_VERSION} CACHE BOOL "Enable version always-up-to-date when building codebase. Disable only when you do not care an accurate version number")
set(OPENFPGA_IPO_BUILD ${ARKANGEL_IPO_BUILD} CACHE STRING "Should OpenFPGA be compiled with interprocedural compiler optimizations?" FORCE)
set(VTR_IPO_BUILD ${ARKANGEL_IPO_BUILD} CACHE STRING "Should VTR be compiled with interprocedural compiler optimizations?" FORCE)
set(OPENFPGA_WITH_YOSYS OFF CACHE BOOL "Enable building Yosys")
set(OPENFPGA_WITH_YOSYS_PLUGIN OFF CACHE BOOL "Enable building Yosys plugin")
set(OPENFPGA_WITH_TEST OFF CACHE BOOL "Enable testing build for codebase. Once enabled, make test can be run")
set(OPENFPGA_WITH_SWIG OFF CACHE BOOL "Enable SWIG interface when building codebase. Disable when you do not need high-level interfaces, such as Tcl/Python")
set(OPENFPGA_GUI_INSTALLER OFF CACHE BOOL "Enable installer with a graphical user interface")
set(OPENFPGA_WITH_INSTALLER OFF CACHE BOOL  "Enable installer to be built")
set(OPENFPGA_INSTALL_DOC OFF CACHE BOOL  "Installer will include documentation")
set(ENABLE_PLUGIN_SLANG ON CACHE BOOL "Enable the build on yosys-plugin on SystemVerilog")
option(USE_EXTERNAL_YOSYS, "Use an external prebuilt yosys and slang. When enabled, only RapidFlex plugin will be built and it is based on the prebuilt yosys and slang" OFF)

# Options pass on to VTR
set(WITH_ABC OFF CACHE BOOL "Enable building ABC in Verilog-to-Routing")
set(WITH_ODIN OFF CACHE BOOL "Enable building Odin in Verilog-to-Routing")
set(WITH_ODIN OFF CACHE BOOL "Enable building Odin in Verilog-to-Routing")
set(ODIN_DEBUG OFF CACHE BOOL "Enable building odin with debug flags in Verilog-to-Routing")
set(ODIN_WARN OFF CACHE BOOL "Enable building odin with extra warning flags in Verilog-to-Routing")
set(ODIN_COVERAGE OFF CACHE BOOL "Enable building odin with coverage flags in Verilog-to-Routing")
set(ODIN_TIDY OFF CACHE BOOL "Enable building odin with clang tidy in Verilog-to-Routing")
set(ODIN_SANITIZE OFF CACHE BOOL "Enable building odin with sanitize flags in Verilog-to-Routing")
set(WITH_YOSYS OFF CACHE BOOL "Enable building Yosys in Verilog-to-Routing")
set(ODIN_YOSYS OFF CACHE BOOL "Enable building odin with yosys in Verilog-to-Routing")
set(YOSYS_SV_UHDM_PLUGIN OFF CACHE BOOL "Enable building and installing Yosys SystemVerilog and UHDM plugins in Verilog-to-Routing")
set(VTR_ENABLE_VERSION ${AKRANGEL_WITH_VERSION} CACHE BOOL "Enable version always-up-to-date when building codebase. Disable only when you do not care an accurate version number")
set(FORCE_YOSYS_BUILD OFF CACHE BOOL "Force to build yosys regardless of caches")

# Override WGET Options
if (WIN32)
  if(MSVC)
    set(WGET_OUTPUT_OPTION "-o" CACHE STRING "Wget output option" FORCE)
  elseif(DEFINED ENV{MSYSTEM})
    message(STATUS "Wget specifically using the Msys2 toolchain")
    set(WGET_OUTPUT_OPTION "-O" CACHE STRING "Wget output option" FORCE)
  endif()
elseif(UNIX AND NOT APPLE)
  set(WGET_OUTPUT_OPTION "-O" CACHE STRING "Wget output option" FORCE)
endif()

#Compiler flag configuration checks
include(CheckCXXCompilerFlag)

#
# We require c++20 support
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #No compiler specific extensions

#
# Interprocedural optimization
#
#   Note that we manually clear the INTERPROCEDURAL_OPTIMIZATION flag on ABC later
#   to avoid cmake warnings
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
if (OPENFPGA_IPO_BUILD STREQUAL "on")
    if (IPO_SUPPORTED)
        message(STATUS "Building with IPO: on")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(ERROR "Building with IPO unsupported with this compiler!")
    endif()
elseif(OPENFPGA_IPO_BUILD STREQUAL "auto")
    if (IPO_SUPPORTED AND NOT CMAKE_BUILD_TYPE STREQUAL "debug")
        message(STATUS "Building with IPO: on (auto)")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(STATUS "Building with IPO: off (auto)")
    endif()
else()
    message(STATUS "Building with IPO: off")
endif()

if(NOT MSVC)
    # for GCC and Clang
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
endif()

# To ease debugging, include files for every file in your project
if (MSVC)
    add_compile_options("/showIncludes")
endif()
# Set WARN FLAGS
set(WARN_FLAGS "")

if (MSVC)
  # Visual studio warnings 
  # Note that we do not use /Wall since it generates warnings about standard library headers
  set(WARN_FLAGS_TO_CHECK # the flags to check if the compiler supports
      "/W4" # Most warnings
     )
else ()
  set(WARN_FLAGS_TO_CHECK # the flags to check if the compiler supports
      #GCC-like
      "-Wall"                         #Most warnings, typically good
      "-Wextra"                       #Extra warning, usually good
      "-Wpedantic"                    #Ensure ISO compliance (i.e. no non-standard extensions)
      "-Wcast-qual"                   #Warn if cast removes qualifier (e.g. const char* -> char*)
      "-Wcast-align"                  #Warn if a cast causes memory alignment changes
      "-Wshadow"                      #Warn if local variable shadows another variable
      "-Wformat=2"                    #Sanity checks for printf-like formatting
      "-Wno-format-nonliteral"        # But don't worry about non-literal formtting (i.e. run-time printf format strings)
      "-Wlogical-op"                  #Checks for logical op when bit-wise expected
      "-Wmissing-declarations"        #Warn if a global function is defined with no declaration
      "-Wmissing-include-dirs"        #Warn if a user include directory is missing
      "-Wredundant-decls"             #Warn if there are overlapping declarations
      "-Wswitch-default"              #Warn if a switch has no default
      "-Wundef"                       #Warn if #if() preprocessor refers to an undefined directive
      "-Wunused"                      #Warn about unused variables/parameters
      "-Wunused-variable"             #Warn about variables that are not used
      "-Wunused-parameter"            #Warn about function parameters which are unused
      "-Wdisabled-optimization"       #Warn when optimizations are skipped (usually due to large/complex code)
      "-Wnoexcept"                    #Warn when functions should be noexcept (i.e. compiler know it doesn't throw)
      "-Woverloaded-virtual"          #Warn when a function declaration overrides a virtual method
      "-Wctor-dtor-privacy"           #Warn about inaccessible constructors/destructors
      "-Wnon-virtual-dtor"            #Warn about missing virtual destructors
      "-Wduplicated-cond"             #Warn about identical conditions in if-else chains
      "-Wduplicated-branches"         #Warn when different branches of an if-else chain are equivalent
      "-Wnull-dereference"            #Warn about null pointer dereference execution paths
      "-Wuninitialized"               #Warn about unitialized values
      "-Winit-self"                   #Warn about self-initialization
      "-Wcatch-value=3"               #Warn when catch statements don't catch by reference
      "-Wextra-semi"                  #Warn about redudnant semicolons
      "-Wimplicit-fallthrough=3"      #Warn about case fallthroughs, but allow 'fallthrough' comments to suppress warnings
      #GCC-like optional
      #"-Wsuggest-final-types"         #Suggest where 'final' would help if specified on a type methods
      #"-Wsuggest-final-methods"       #Suggest where 'final' would help if specified on methods
      #"-Wsuggest-override"            #Suggest where 'override' should be specified
      #"-Wold-style-cast"              #Warn about using c-style casts
      #"-Wconversion"                  #Warn when type conversions may change value
      #"-Wsign-conversion"             #Warn if a conversion may change the sign
      #"-Wpadded"                      #Will warn if additional padding is introduced to a struct/class. Turn on if optimizing class memory layouts
      #"-Wstrict-overflow=2"           #Warn if the compiler optimizes assuming signed overflow does not occur
      #"-Wfloat-equal"                 #Warn about using direct floating point equality
      #"-Wunsafe-loop-optimizations"   #Warn when loops can't be optimized
      #"-Wswitch-enum"                 #Warn about uncovered enumeration values in a switch (even if there is a default)
      #"-Wsign-promo"                  #Warn when overload resolution converts an unsigned type to signed when an unsigned overload exists
      #"-Wdouble-promotion"            #Warn when float is implicitly propted to double
      #"-Wuseless-cast"                #Warn about casts to the same type
      #"-Wzero-as-null-pointer-constant" #Warn about using '0' instead of nullptr
      )
endif()

# check and see if the compiler supports the various warning flags
# and add valid flags
foreach (flag ${WARN_FLAGS_TO_CHECK})
  CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
  if (CXX_COMPILER_SUPPORTS_${flag})
    # flag supported, so enable it
    set (WARN_FLAGS "${WARN_FLAGS} ${flag}")
  endif()
endforeach()

#Suppress IPO link warnings
set(IPO_LINK_WARN_SUPRESS_FLAGS " ")
set(IPO_LINK_WARN_SUPRESS_FLAGS_TO_CHECK
    "-Wno-null-dereference"
    )
foreach(flag ${IPO_LINK_WARN_SUPRESS_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
    if(CXX_COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(IPO_LINK_WARN_SUPRESS_FLAGS "${IPO_LINK_WARN_SUPRESS_FLAGS} ${flag}")
    endif()
endforeach()
# Undef min() and max() function in windows as STL is used
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
#
# Sanitizer flags
#
set(SANITIZE_FLAGS "")
if(ARKANGEL_ENABLE_SANITIZE)
    #Enable sanitizers
    # -fuse-ld=gold force the gold linker to be used (required for sanitizers, but not enabled by default on some systems)
    set(SANITIZE_FLAGS "-g -fsanitize=address -fsanitize=leak -fsanitize=undefined -fuse-ld=gold")
    message(STATUS "SANTIIZE_FLAGS: ${SANITIZE_FLAGS}")
    link_libraries("-static-libasan") #Fixes 'ASan runtime does not come first in initial library list'
endif()

# Disable slang in C++ codes
if (NOT ENABLE_PLUGIN_SLANG)
  add_compile_definitions(ARKANGEL_NO_SLANG)
endif()

# Extra flags
set(SWIG_SHARED_FLAGS "")
if (ARKANGEL_WITH_SWIG)
# Find TCL
    include(FindTCL)
    message(STATUS "tcl.h path is : ${TCL_INCLUDE_PATH}")
    message(STATUS "libtcl.so path is : ${TCL_LIBRARY}")
    # for gui TCL/API test only
    if (ARKANGEL_SHELL_GUI_TEST)
        message(STATUS "Build sandbox for GUI/TCL API test ")
        add_definitions("-DARKANGEL_SHELL_GUI_TEST")
    endif()
# Find SWIG
find_package(SWIG 3.0 REQUIRED)
    if (SWIG_VERSION VERSION_GREATER_EQUAL "4.1.0")
        message(WARNING "Using SWIG >= ${SWIG_VERSION} -flatstaticmethod flag for python")
    endif()
    include(UseSWIG)

    set(SWIG_SHARED_FLAGS "-fpic")
endif()
add_compile_options(${SWIG_SHARED_FLAGS})

# Set final flags
#
separate_arguments(
    ADDITIONAL_FLAGS UNIX_COMMAND "${SANITIZE_FLAGS} ${PROFILING_FLAGS} ${COVERAGE_FLAGS} ${LOGGING_FLAGS} ${COLORED_COMPILE} ${EXTRA_FLAGS} ${SWIG_SHARED_FLAGS}"
    )
separate_arguments(
    WARN_FLAGS UNIX_COMMAND "${WARN_FLAGS}"
    )

# Unit Testing
#
if (ARKANGEL_WITH_TEST)
    enable_testing()
endif()

#include(FindGraphviz)
#message(STATUS "Graphviz version is : ${Graphviz_VERSION}")
#message(STATUS "Graphviz header files is under : ${Graphviz_INCLUD_DIR}")
#message(STATUS "Graphviz lib is under : ${Graphviz_LIBRARIES}")
#
# Build with license
#
if (BUILD_ARKANGEL_WITH_LICENSE)
    add_definitions("-DARKANGEL_LICENSED_BUILD")
    message(STATUS "License control is enabled")
else()
    message(STATUS "License control is disabled")
endif()

# 
# Sub-projects to apply current complier settings
#
add_subdirectory(libs)
add_subdirectory(core)

# ALKDL ALWAYS included
add_subdirectory(etc/device/ALKDL/utils)

# ALKDC / ALKDL only when full-enabled
if(INSTALLER_WITH_FULL_DEVICE)
    message(STATUS "Full device enabled: Including ALKDC / ALKDD")
    add_subdirectory(etc/device/ALKDC/utils)
    add_subdirectory(etc/device/ALKDD/utils)
    message(STATUS "Full device enabled: Including ALKDLe")
    add_subdirectory(etc/device/ALKDLe/utils)
else()
    message(STATUS "Full device disabled: Only including ALKDL")
endif()

#PugiXml has some deliberate switch fallthrough cases (as indicated by comments), but they
#are tagged as warnings with g++-7 (the comments don't match g++-7's suppression regexes).
#Since we don't want to change PugiXml (it is developed externally), we relax the warning
#level so no fallthrough warnings are generated
CHECK_CXX_COMPILER_FLAG("-Wimplicit-fallthrough=0" CXX_COMPILER_SUPPORTS_-Wimplicit-fallthrough=0)
if(CXX_COMPILER_SUPPORTS_-Wimplicit-fallthrough=0)
    target_compile_options(libpugixml PRIVATE "-Wimplicit-fallthrough=0")
endif()

# Add extra compilation flags to suppress warnings from some libraries/tools
# Note that target_compile_options() *appends* to the current compilation options of
# the specified target

# Check system id and branch on dep build-up
# CentOS/Redhat: a special env will be applied in the ArkangelInstaller.cmake
# Ubuntu: grab all the dep lib into an env
include(GetOsReleaseId)
# Call the function to populate the variable OS_FLAVOR
get_os_release_id(OS_FLAVOR)

# Add python3 subsystem to the installer
if (BUILD_PYTHON_FROM_SOURCE) 
  message(STATUS "Build custom python from source")
  include(PackPythonEnv)
  # Add a custom target to ensure the Python build/install happens before the main install
  add_custom_target(install_python DEPENDS PythonExternal)
  add_custom_target(install_shared_python DEPENDS PythonSharedExternal)
  add_dependencies(install_python arkangel openfpga) # Ensure your main target depends on this
  add_dependencies(install_shared_python arkangel openfpga) # Ensure your main target depends on this
  # Now, install the wheels using the newly installed python and bundled wheels
  add_custom_target(
      py_wheel_sitepackages ALL
      # Build the wheelhouse 
      COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHON_WHEEL_DIR}
      COMMAND LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${PYTHON_SHARED_LIB_DIR} ${PYTHON_EXEC} -m pip install --upgrade pip
  #    COMMAND LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${PYTHON_SHARED_LIB_DIR} ${PYTHON_EXEC} -m pip download -r ${CMAKE_SOURCE_DIR}/requirements.txt -d ${PYTHON_WHEEL_DIR}
      COMMAND LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${PYTHON_SHARED_LIB_DIR} ${PYTHON_EXEC} -m pip install -r ${CMAKE_SOURCE_DIR}/requirements.txt --force-reinstall --target ${PYTHON_SITE_PACKAGE_DIR} --upgrade
      DEPENDS ${PYTHON_EXEC}
  )
  add_dependencies(py_wheel_sitepackages install_python install_shared_python) # Ensure your main target depends on this
  # Use install(DIRECTORY ...) to include the contents of the Python install dir
  install(
      DIRECTORY "${PYTHON_INSTALL_DIR}/"
      DESTINATION "env/python3" # Installs to the root of the CPack package
      COMPONENT python_package
      USE_SOURCE_PERMISSIONS
  )
  # Install python shared library 
  install(
      DIRECTORY "${PYTHON_SHARED_LIB_DIR}/"
      DESTINATION "env/python3/lib" # Installs to the root of the CPack package
      COMPONENT python_package
      PATTERN "*.so.*"
      PATTERN "*.so"
  )
endif()

# For installer
include(ArkangelInstaller)
if(NOT "${OS_FLAVOR}" STREQUAL "RHEL" AND NOT "${OS_FLAVOR}" STREQUAL "CentOS")
  message(STATUS "Include dependeny on dynamic linked lib in the installer")
  include(ArkangelDeps)
endif()
include(ArkangelPackaging)
