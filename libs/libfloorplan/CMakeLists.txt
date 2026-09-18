cmake_minimum_required(VERSION 3.9)

project("libfloorplan")

file(GLOB_RECURSE EXEC_SOURCES test/*.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})
#Create the library
add_library(libfloorplan STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})

target_include_directories(libfloorplan PUBLIC ${LIB_INCLUDE_DIRS})

set_target_properties(libfloorplan PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

if (ARKANGEL_WITH_GUI)
  find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)
  if(Qt5_FOUND)
    # Set necessary Qt variables
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    message(STATUS "Qt5 found, building with GUI")
  endif()
endif()

if (Qt5_FOUND) 
  # Add a definition so code knows Qt is present
  target_compile_definitions(libfloorplan PUBLIC USE_QT)
endif()


#Specify link-time dependancies
target_link_libraries(libfloorplan
                      libopenfpgautil
                      libvtrutil
                      libargparse
                      libarchopenfpga
                      libopenfpga
                      yaml-cpp 
                      libtilenaming
                      libarkangelutil
                      libpugiutil
                      libtableprinter)

if (Qt5_FOUND) 
  target_link_libraries(libfloorplan
                        Qt5::Widgets
                        Qt5::Core
                        Qt5::Gui)
endif()

#Create the test executable
foreach(testsourcefile ${EXEC_SOURCES})
    # Use a simple string replace, to cut off .cpp.
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    # Make sure the library is linked to each test executable
    target_link_libraries(${testname} libfloorplan)
endforeach(testsourcefile ${EXEC_SOURCES})

install(TARGETS libfloorplan
        DESTINATION bin
        COMPONENT engine_package
       )
