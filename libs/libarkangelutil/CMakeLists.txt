cmake_minimum_required(VERSION 3.9)

project("libarcxpressutil")

# Find Graphviz (set to OPTIONAL to skip if not found)
if (ARCXPRESS_WITH_GRAPHVIZ)
  find_package(Graphviz REQUIRED)
  if (Graphviz_FOUND)
    message(STATUS "Graphviz version is : ${Graphviz_VERSION}")
    message(STATUS "Graphviz header files is under : ${Graphviz_INCLUDE_DIRS}")
    message(STATUS "Graphviz lib is under : ${Graphviz_LIBRARIES}")
    # For graphviz find the parent directory and include it
    set(Graphviz_PARENT_INCLUDE_DIRS "")
    cmake_path(GET Graphviz_INCLUDE_DIRS PARENT_PATH Graphviz_PARENT_INCLUDE_DIRS)
    message(STATUS "Graphviz header files is under : ${Graphviz_PARENT_INCLUDE_DIRS}")
  # If Graphviz is not found, define a variable to disable related features
  else()
    message(WARNING "Graphviz not found. Graphviz-related features will be disabled.")
  endif()
endif()

# Version info
set(ARCXPRESS_VERSION_FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/src/arcxpress_version.cpp.in)
set(ARCXPRESS_VERSION_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/arcxpress_version.cpp)

# Compiler info
set(ARCXPRESS_COMPILER_INFO "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} on ${CMAKE_SYSTEM} ${CMAKE_SYSTEM_PROCESSOR}")
set(ARCXPRESS_BUILD_INFO "${CMAKE_BUILD_TYPE}")

# Set default version numbers in case not specified
if (NOT DEFINED ARCXPRESS_VERSION_MAJOR)
    set(ARCXPRESS_VERSION_MAJOR 0)
endif()

if (NOT DEFINED ARCXPRESS_VERSION_MINOR)
    set(ARCXPRESS_VERSION_MINOR 0)
endif()

if (NOT DEFINED ARCXPRESS_VERSION_PATCH)
    set(ARCXPRESS_VERSION_PATCH 0)
endif()

set(ARCXPRESS_BUILD_INFO "${ARCXPRESS_BUILD_INFO} ASSERT_LEVEL=${VTR_ASSERT_LEVEL}")

# We always update the arcxpress_version.cpp file every time the project is built, 
# to ensure the git revision and dirty status are up to date.
#
# We need to do this in two stages:
#
# 1) We build a custom target 'version' (which is always out of date) so it will always be run.
#    It touches the unprocessed version input file so it too will always be out of date.
#
# 2) The custom command depends on the touched version input file and generates the processed 
#    version file, with updated values. The custom command uses the configure_version.cmake 
#    script to generate the up-to-date openfpga_version.cpp
if (ARCXPRESS_WITH_VERSION) 
    add_custom_target(arcxpress_version ALL
        COMMAND ${CMAKE_COMMAND} -E touch ${ARCXPRESS_VERSION_FILE_IN})

    add_custom_command(OUTPUT ${ARCXPRESS_VERSION_FILE_OUT}
        COMMAND ${CMAKE_COMMAND} 
                        -D IN_FILE=${ARCXPRESS_VERSION_FILE_IN}
                        -D OUT_FILE=${ARCXPRESS_VERSION_FILE_OUT}
                        -D ARCXPRESS_VERSION_MAJOR=${ARCXPRESS_VERSION_MAJOR}
                        -D ARCXPRESS_VERSION_MINOR=${ARCXPRESS_VERSION_MINOR}
                        -D ARCXPRESS_VERSION_PATCH=${ARCXPRESS_VERSION_PATCH}
                        -D ARCXPRESS_VERSION_PRERELEASE=${ARCXPRESS_VERSION_PRERELEASE}
                        -D ARCXPRESS_COMPILER_INFO=${ARCXPRESS_COMPILER_INFO}
                        -D ARCXPRESS_BUILD_INFO=${ARCXPRESS_BUILD_INFO}
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/configure_version.cmake
        MAIN_DEPENDENCY ${ARCXPRESS_VERSION_FILE_IN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
else()
    configure_file(${ARCXPRESS_VERSION_FILE_IN} ${ARCXPRESS_VERSION_FILE_OUT})
endif()

# Find source files
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS */*.h)

# Ensure the include directories are populated
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

# Add version file to the sources
list(APPEND LIB_SOURCES ${ARCXPRESS_VERSION_FILE_OUT})

# If Graphviz is not found, remove the Graphviz-related source file
#if (NOT USE_GRAPHVIZ)
#    list(REMOVE_ITEM LIB_SOURCES src/arcxpress_graphviz_utils.cpp)
#endif()

# Create the library
add_library(libarcxpressutil STATIC
            ${LIB_SOURCES}
            ${LIB_HEADERS})

# Include directories for the library
target_include_directories(libarcxpressutil PUBLIC ${LIB_INCLUDE_DIRS})
if (Graphviz_FOUND)
  target_include_directories(libarcxpressutil PUBLIC ${Graphviz_INCLUDE_DIRS})
  target_include_directories(libarcxpressutil PRIVATE ${Graphviz_PARENT_INCLUDE_DIRS})
endif()

# Pass the USE_GRAPHVIZ macro to all targets in the project
if (Graphviz_FOUND)
  target_compile_definitions(libarcxpressutil PUBLIC USE_GRAPHVIZ)
endif()

# Avoid 'lib' prefix in the target
set_target_properties(libarcxpressutil PROPERTIES PREFIX "")

# Ensure version is up-to-date by requiring 'version' to be run first
if (ARCXPRESS_WITH_VERSION) 
    add_dependencies(libarcxpressutil arcxpress_version)
endif()

# Specify link-time dependencies
target_link_libraries(libarcxpressutil
                      libopenfpgautil
                      libopenfpga
                      libsignalhandlermanager
                      libvtrutil)

# If Graphviz is enabled, link Graphviz libraries
if (Graphviz_FOUND)
  target_link_libraries(libarcxpressutil ${Graphviz_LIBRARIES})
endif()

# Install the target
install(TARGETS libarcxpressutil
        DESTINATION bin
        COMPONENT engine_package)
