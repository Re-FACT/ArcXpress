cmake_minimum_required(VERSION 3.9)

project("libfilesignature")

file(GLOB_RECURSE EXEC_SOURCES test/*.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})
#Create the library
add_library(libfilesignature STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})
target_include_directories(libfilesignature PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libfilesignature PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

# Find dependency of OpenSSL
# For version > 3.0.0, set a flag as new API should be used
# For version < 3.0.0, set a flag as old API should be used
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL found")
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
else() 
  message(FATAL_ERROR "OpenSSL not found")
endif()
find_package(OpenSSL 3.0.0)
if(OPENSSL_FOUND)
  message(STATUS "Will use OpenSSL >3 APIs")
  add_definitions(-DUSE_OPENSSL3_API)
else()
  message(STATUS "Will use OpenSSL <3 APIs")
endif()

#Specify link-time dependancies
target_link_libraries(libfilesignature
                      libopenfpgautil
                      libvtrutil
                      yaml-cpp
                      libopenfpgashell
                      ${OPENSSL_LIBRARIES})

#Create the test executable
foreach(testsourcefile ${EXEC_SOURCES})
    # Use a simple string replace, to cut off .cpp.
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    # Make sure the library is linked to each test executable
    target_link_libraries(${testname} libfilesignature)
endforeach(testsourcefile ${EXEC_SOURCES})

install(TARGETS libfilesignature
        DESTINATION bin
        COMPONENT engine_package
)
