cmake_minimum_required(VERSION 3.9)

project("libarkangelutil")

# Find Graphviz (set to OPTIONAL to skip if not found)
if (ARKANGEL_WITH_GRAPHVIZ)
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
set(ARKANGEL_VERSION_FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/src/arkangel_version.cpp.in)
set(ARKANGEL_VERSION_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/arkangel_version.cpp)

# Compiler info
set(ARKANGEL_COMPILER_INFO "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} on ${CMAKE_SYSTEM} ${CMAKE_SYSTEM_PROCESSOR}")
set(ARKANGEL_BUILD_INFO "${CMAKE_BUILD_TYPE}")

# Set default version numbers in case not specified
if (NOT DEFINED ARKANGEL_VERSION_MAJOR)
    set(ARKANGEL_VERSION_MAJOR 0)
endif()

if (NOT DEFINED ARKANGEL_VERSION_MINOR)
    set(ARKANGEL_VERSION_MINOR 0)
endif()

if (NOT DEFINED ARKANGEL_VERSION_PATCH)
    set(ARKANGEL_VERSION_PATCH 0)
endif()

set(ARKANGEL_BUILD_INFO "${ARKANGEL_BUILD_INFO} ASSERT_LEVEL=${VTR_ASSERT_LEVEL}")

# We always update the arkangel_version.cpp file every time the project is built, 
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
if (ARKANGEL_WITH_VERSION) 
    add_custom_target(arkangel_version ALL
        COMMAND ${CMAKE_COMMAND} -E touch ${ARKANGEL_VERSION_FILE_IN})

    add_custom_command(OUTPUT ${ARKANGEL_VERSION_FILE_OUT}
        COMMAND ${CMAKE_COMMAND} 
                        -D IN_FILE=${ARKANGEL_VERSION_FILE_IN}
                        -D OUT_FILE=${ARKANGEL_VERSION_FILE_OUT}
                        -D ARKANGEL_VERSION_MAJOR=${ARKANGEL_VERSION_MAJOR}
                        -D ARKANGEL_VERSION_MINOR=${ARKANGEL_VERSION_MINOR}
                        -D ARKANGEL_VERSION_PATCH=${ARKANGEL_VERSION_PATCH}
                        -D ARKANGEL_VERSION_PRERELEASE=${ARKANGEL_VERSION_PRERELEASE}
                        -D ARKANGEL_COMPILER_INFO=${ARKANGEL_COMPILER_INFO}
                        -D ARKANGEL_BUILD_INFO=${ARKANGEL_BUILD_INFO}
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/configure_version.cmake
        MAIN_DEPENDENCY ${ARKANGEL_VERSION_FILE_IN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
else()
    configure_file(${ARKANGEL_VERSION_FILE_IN} ${ARKANGEL_VERSION_FILE_OUT})
endif()

# Find source files
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS */*.h)

# Ensure the include directories are populated
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

# Add version file to the sources
list(APPEND LIB_SOURCES ${ARKANGEL_VERSION_FILE_OUT})

# If Graphviz is not found, remove the Graphviz-related source file
#if (NOT USE_GRAPHVIZ)
#    list(REMOVE_ITEM LIB_SOURCES src/arkangel_graphviz_utils.cpp)
#endif()

# Create the library
add_library(libarkangelutil STATIC
            ${LIB_SOURCES}
            ${LIB_HEADERS})

# Include directories for the library
target_include_directories(libarkangelutil PUBLIC ${LIB_INCLUDE_DIRS})
if (Graphviz_FOUND)
  target_include_directories(libarkangelutil PUBLIC ${Graphviz_INCLUDE_DIRS})
  target_include_directories(libarkangelutil PRIVATE ${Graphviz_PARENT_INCLUDE_DIRS})
endif()

# Pass the USE_GRAPHVIZ macro to all targets in the project
if (Graphviz_FOUND)
  target_compile_definitions(libarkangelutil PUBLIC USE_GRAPHVIZ)
endif()

# Avoid 'lib' prefix in the target
set_target_properties(libarkangelutil PROPERTIES PREFIX "")

# Ensure version is up-to-date by requiring 'version' to be run first
if (ARKANGEL_WITH_VERSION) 
    add_dependencies(libarkangelutil arkangel_version)
endif()

# Specify link-time dependencies
target_link_libraries(libarkangelutil
                      libopenfpgautil
                      libopenfpga
                      libsignalhandlermanager
                      libvtrutil)

# If Graphviz is enabled, link Graphviz libraries
if (Graphviz_FOUND)
  target_link_libraries(libarkangelutil ${Graphviz_LIBRARIES})
endif()

# Install the target
install(TARGETS libarkangelutil
        DESTINATION bin
        COMPONENT engine_package)
