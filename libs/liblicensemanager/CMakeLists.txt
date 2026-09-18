cmake_minimum_required(VERSION 3.9)

project("liblicensemanager")

function(files_to_dirs file_list directory_list)
    #Extract the directories for each header file
    foreach(file ${${file_list}})
        get_filename_component(dir ${file} DIRECTORY)
        list(APPEND dir_list ${dir})
    endforeach()

    #Remove any duplicates
    list(LENGTH "${dir_list}" length)
    if(${length} GREATER 1) #Avoid error with zero-length lists
        list(REMOVE_DUPLICATES ${dir_list})
    endif()

    #Set the second argument in the caller's scope
    set(${directory_list} ${dir_list} PARENT_SCOPE)
endfunction(files_to_dirs)

# for arkangel integration
file(GLOB_RECURSE EXEC_SOURCES test/*.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

# Set paths and variables
set(GIT_SUBMODULE OFF)
set(LICENSECC_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/licensecc/install/lib/cmake/licensecc/")
set(LCC_PROJECT_NAME "AAEE")
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/licensecc/install")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# For debug 
if(CMAKE_BUILD_TYPE STREQUAL "debug") 
	message("[DEBUG]")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3")
else()
    # or else licensecc will prints some information we don't want to disclose
    add_definitions("-DNDEBUG")
endif()

cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 

# Add subdirectory for licensecc
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/licensecc)
if (NOT WIN32)
  target_compile_options(licensecc_static PRIVATE
      # Add any other desired compiler options here
      -Wno-missing-field-initializers
  )
endif()

# Custom target for installing licensecc
add_custom_target(install_licensecc ALL
    COMMENT "Building and installing licensecc"
    COMMAND $(MAKE) install
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/licensecc
)
add_dependencies(install_licensecc licensecc_static)

add_library(liblicensemanager STATIC
            ${LIB_HEADERS}
            ${LIB_SOURCES})
target_include_directories(liblicensemanager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR}/licensecc/install/include ${CMAKE_CURRENT_SOURCE_DIR}/licensecc/install/include/licensecc/${LCC_PROJECT_NAME} ${LIB_INCLUDE_DIRS})

# test lic example
add_executable(test_lic src/license_manager.h src/license_manager.cpp test/test_lic.cpp)
target_include_directories(test_lic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR}/licensecc/install/include ${CMAKE_CURRENT_SOURCE_DIR}/licensecc/install/include/licensecc/${LCC_PROJECT_NAME})
target_link_libraries(test_lic
    libarkangelutil
	liblicensemanager)
add_dependencies(test_lic install_licensecc)

target_compile_options(liblicensemanager PRIVATE
    # Add any other desired compiler options here
    -Wno-missing-field-initializers
)

# link licensecc to liblicensemanager
cmake_policy(SET CMP0028 NEW)
target_link_libraries(liblicensemanager 
    libarkangelutil
    licensecc_static)
add_dependencies(liblicensemanager install_licensecc)

# install lib
install(TARGETS liblicensemanager DESTINATION bin)
