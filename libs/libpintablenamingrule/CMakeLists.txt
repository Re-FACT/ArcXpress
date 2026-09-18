cmake_minimum_required(VERSION 3.9)

project("libpintablenamingrule")

file(GLOB_RECURSE EXEC_SOURCES test/*.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})

#Create the library
add_library(libpintablenamingrule STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})
target_include_directories(libpintablenamingrule PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libpintablenamingrule PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libpintablenamingrule
                      libopenfpgautil
                      libvtrutil
                      libarchopenfpga
                      libpugiutil)

#Create the test executable
foreach(testsourcefile ${EXEC_SOURCES})
    # Use a simple string replace, to cut off .cpp.
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    # Make sure the library is linked to each test executable
    target_link_libraries(${testname} libpintablenamingrule)
endforeach(testsourcefile ${EXEC_SOURCES})

install(TARGETS libpintablenamingrule
        DESTINATION bin
        COMPONENT engine_package
)
