cmake_minimum_required(VERSION 3.9)

project("libarchgensofahd")

file(GLOB_RECURSE EXEC_SOURCES test/*.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
#list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})

#Create the library
add_library(libarchgensofahd STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})
target_include_directories(libarchgensofahd PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libarchgensofahd PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libarchgensofahd
                      libtapeoutmanager
                      libarchgensofa
)

#Create the test executable
foreach(testsourcefile ${EXEC_SOURCES})
    # Use a simple string replace, to cut off .cpp.
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    # Make sure the library is linked to each test executable
    target_link_libraries(${testname} libarchgensofahd)
    install(TARGETS ${testname}
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/arch_gen
            COMPONENT device_package
    )
endforeach(testsourcefile ${EXEC_SOURCES})

install(TARGETS libarchgensofahd 
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/arch_gen
        COMPONENT device_package
)
