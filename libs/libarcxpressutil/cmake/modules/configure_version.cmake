#
# Versioning information
#
#Figure out the git revision
find_package(Git QUIET)
if(GIT_FOUND)
    exec_program(${GIT_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}
                 ARGS describe --always --long --dirty
                 OUTPUT_VARIABLE ARCXPRESS_VCS_REVISION
                 RETURN_VALUE GIT_DESCRIBE_RETURN_VALUE)

    if(NOT GIT_DESCRIBE_RETURN_VALUE EQUAL 0)
        #Git describe failed, usually this means we
        #aren't in a git repo -- so don't set a VCS 
        #revision
        set(ARCXPRESS_VCS_REVISION "unkown")
    endif()

    #Call again with exclude to get the revision excluding any tags
    #(i.e. just the commit ID and dirty flag)
    exec_program(${GIT_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}
                 ARGS describe --always --long --dirty --exclude '*'
                 OUTPUT_VARIABLE ARCXPRESS_VCS_REVISION_SHORT
                 RETURN_VALUE GIT_DESCRIBE_RETURN_VALUE)
    if(NOT GIT_DESCRIBE_RETURN_VALUE EQUAL 0)
        #Git describe failed, usually this means we
        #aren't in a git repo -- so don't set a VCS 
        #revision
        set(ARCXPRESS_VCS_REVISION_SHORT "unkown")
    endif()
else()
    #Couldn't find git, so can't look-up VCS revision
    set(ARCXPRESS_VCS_REVISION "unkown")
    set(ARCXPRESS_VCS_REVISION_SHORT "unkown")
endif()


#Set the version according to semver.org
set(ARCXPRESS_VERSION "${ARCXPRESS_VERSION_MAJOR}.${ARCXPRESS_VERSION_MINOR}.${ARCXPRESS_VERSION_PATCH}")
if(ARCXPRESS_VERSION_PRERELEASE)
    set(ARCXPRESS_VERSION "${ARCXPRESS_VERSION}-${ARCXPRESS_VERSION_PRERELEASE}")
endif()
set(ARCXPRESS_VERSION_SHORT ${ARCXPRESS_VERSION})
if(ARCXPRESS_VCS_REVISION)
    set(ARCXPRESS_VERSION "${ARCXPRESS_VERSION}+${ARCXPRESS_VCS_REVISION_SHORT}")
endif()

#Other build meta-data
string(TIMESTAMP ARCXPRESS_BUILD_TIMESTAMP)
set(ARCXPRESS_BUILD_TIMESTAMP "${ARCXPRESS_BUILD_TIMESTAMP}")
set(ARCXPRESS_BUILD_INFO "${ARCXPRESS_BUILD_INFO}")

message(STATUS "ArkAngel Version: ${ARCXPRESS_VERSION}")

configure_file(${IN_FILE} ${OUT_FILE})
