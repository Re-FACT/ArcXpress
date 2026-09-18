# Define a function to get the OS release ID
function(get_os_release_id _result_var)
    # Try to use 'lsb_release' command (might not be installed by default on minimal installs)
    find_program(LSB_RELEASE_EXEC lsb_release)
    if(LSB_RELEASE_EXEC)
        execute_process(
            COMMAND ${LSB_RELEASE_EXEC} -is
            OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(LSB_RELEASE_ID_SHORT)
            set(${_result_var} "${LSB_RELEASE_ID_SHORT}" PARENT_SCOPE)
            return()
        endif()
    endif()

    # Fallback method: check for common release files
    if(EXISTS "/etc/redhat-release")
        file(READ "/etc/redhat-release" OS_RELEASE_CONTENT)
        # Check if it contains "Red Hat" or "CentOS"
        if("${OS_RELEASE_CONTENT}" MATCHES "Red Hat")
            set(${_result_var} "RHEL" PARENT_SCOPE)
            return()
        elseif("${OS_RELEASE_CONTENT}" MATCHES "CentOS")
            set(${_result_var} "CentOS" PARENT_SCOPE)
            return()
        endif()
    endif()

    # Generic Linux fallback
    set(${_result_var} "Linux" PARENT_SCOPE)
endfunction()
