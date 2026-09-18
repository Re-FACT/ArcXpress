cmake_minimum_required(VERSION 3.9)

project("arkangel_shell")

file(GLOB_RECURSE EXEC_SOURCE main.cpp)
file(GLOB_RECURSE LAUNCHER_SOURCE aaee.cpp)
file(GLOB_RECURSE LIB_SOURCES */*.cpp)
file(GLOB_RECURSE LIB_HEADERS */*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCE})
list(REMOVE_ITEM LIB_SOURCES ${LAUNCHER_SOURCE})

if (ARKANGEL_WITH_SWIG)
# SWIG library
  SwigLib(NAME      arkangel_shell
          NAMESPACE std
          LANGUAGE  tcl
          I_FILE    interface/arkangel_shell.i)
  target_include_directories(arkangel_shell PUBLIC ${LIB_INCLUDE_DIRS})
  target_link_libraries(arkangel_shell
                        libarkangel)
endif()

#Create the library
#Static linked library for other C++ libraries
add_library(libarkangel STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})
target_include_directories(libarkangel PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libarkangel PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libarkangel
                      libtapeoutmanager
                      libbenchmarkmanager
                      libpdkmanager
                      libbootconfig
                      libarkangelutil
                      libsignalhandlermanager
                      libvprstats
                      libarchconfiger
                      libfilesignature
                      libopenfpga)

if (BUILD_ARKANGEL_WITH_LICENSE)
target_link_libraries(libarkangel
                      liblicensemanager)
endif()

#Create the test executable
add_executable(arkangel ${EXEC_SOURCE})
target_link_libraries(arkangel libarkangel)

# Show nested tree for header file being read for debugging compilation error when occurred
if (MSVC)
    target_compile_options(arkangel PRIVATE "/showIncludes")
endif()

#Suppress IPO link warnings if IPO is enabled
get_target_property(ARKANGEL_USES_IPO arkangel INTERPROCEDURAL_OPTIMIZATION)
if (ARKANGEL_USES_IPO)
    set_property(TARGET arkangel APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

# Create launcher
add_executable(aaee ${LAUNCHER_SOURCE})
target_link_libraries(aaee libarkangel)

# Show nested tree for header file being read for debugging compilation error when occurred
if (MSVC)
    target_compile_options(aaee PRIVATE "/showIncludes")
endif()

#Suppress IPO link warnings if IPO is enabled
get_target_property(ARKANGEL_USES_IPO aaee INTERPROCEDURAL_OPTIMIZATION)
if (ARKANGEL_USES_IPO)
    set_property(TARGET aaee APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

# Install targets
install(TARGETS libarkangel arkangel aaee
        DESTINATION bin
        COMPONENT engine_package
)

