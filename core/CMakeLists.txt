cmake_minimum_required(VERSION 3.9)

project("arcxpress_shell")

file(GLOB_RECURSE EXEC_SOURCE main.cpp)
file(GLOB_RECURSE LAUNCHER_SOURCE aaee.cpp)
file(GLOB_RECURSE LIB_SOURCES */*.cpp)
file(GLOB_RECURSE LIB_HEADERS */*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCE})
list(REMOVE_ITEM LIB_SOURCES ${LAUNCHER_SOURCE})

if (ARCXPRESS_WITH_SWIG)
# SWIG library
  SwigLib(NAME      arcxpress_shell
          NAMESPACE std
          LANGUAGE  tcl
          I_FILE    interface/arcxpress_shell.i)
  target_include_directories(arcxpress_shell PUBLIC ${LIB_INCLUDE_DIRS})
  target_link_libraries(arcxpress_shell
                        libarcxpress)
endif()

#Create the library
#Static linked library for other C++ libraries
add_library(libarcxpress STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})
target_include_directories(libarcxpress PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libarcxpress PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libarcxpress
                      libtapeoutmanager
                      libbenchmarkmanager
                      libpdkmanager
                      libbootconfig
                      libarcxpressutil
                      libsignalhandlermanager
                      libvprstats
                      libarchconfiger
                      libfilesignature
                      libopenfpga)

#Create the test executable
add_executable(arcxpress ${EXEC_SOURCE})
target_link_libraries(arcxpress libarcxpress)

# Show nested tree for header file being read for debugging compilation error when occurred
if (MSVC)
    target_compile_options(arcxpress PRIVATE "/showIncludes")
endif()

#Suppress IPO link warnings if IPO is enabled
get_target_property(ARCXPRESS_USES_IPO arcxpress INTERPROCEDURAL_OPTIMIZATION)
if (ARCXPRESS_USES_IPO)
    set_property(TARGET arcxpress APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

# Create launcher
add_executable(aaee ${LAUNCHER_SOURCE})
target_link_libraries(aaee libarcxpress)

# Show nested tree for header file being read for debugging compilation error when occurred
if (MSVC)
    target_compile_options(aaee PRIVATE "/showIncludes")
endif()

#Suppress IPO link warnings if IPO is enabled
get_target_property(ARCXPRESS_USES_IPO aaee INTERPROCEDURAL_OPTIMIZATION)
if (ARCXPRESS_USES_IPO)
    set_property(TARGET aaee APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

# Install targets
install(TARGETS libarcxpress arcxpress aaee
        DESTINATION bin
        COMPONENT engine_package
)

