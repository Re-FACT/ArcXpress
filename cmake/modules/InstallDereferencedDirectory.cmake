function(install_dereferenced_directory source_dir destination_dir)
  # Find all files recursively in the source directory
  file(GLOB_RECURSE all_files RELATIVE "${source_dir}" "${source_dir}/*")

  set(resolved_files "")
  foreach(_file ${all_files})
    # Construct the full path to the file
    set(SRC_FILE_PATH "${source_dir}/${_file}")
    # Resolve the real path (dereference symlinks)
    #get_filename_component(resolved_full_path "${full_path}" REALPATH)
    # MSYS2 cannot follow symbolic link use native solution in cmake
    if(IS_SYMLINK "${SRC_FILE_PATH}")
        # Explicitly read where the link points
        file(READ_SYMLINK "${SRC_FILE_PATH}" SRC_LINK_TARGET)
        
        # If the symlink is relative, combine it with the source directory path
        if(NOT IS_ABSOLUTE "${SRC_LINK_TARGET}")
            get_filename_component(SRC_FILE_DIR "${SRC_FILE_PATH}" DIRECTORY)
            set(SRC_LINK_TARGET "${SRC_FILE_DIR}/${SRC_LINK_TARGET}")
        endif()
        
        # Normalize the path structure
        get_filename_component(SRC_REAL_PATH "${SRC_LINK_TARGET}" ABSOLUTE)
    else()
        # It's a normal file, get the absolute path
        get_filename_component(SRC_REAL_PATH "${SRC_FILE_PATH}" ABSOLUTE)
    endif()

    # Ensure the resolved file exists before trying to install it
    if(EXISTS "${SRC_REAL_PATH}")
      list(APPEND resolved_files "${SRC_REAL_PATH}")
    else()
      message(WARNING "Resolved file not found: ${SRC_REAL_PATH} (from symlink ${SRC_FILE_PATH})")
    endif()
    # Install the resolved files into the destination directory
    set(DES_FULL_PATH "${destination_dir}/${_file}")
    get_filename_component(DES_FULL_DIR ${DES_FULL_PATH} DIRECTORY)
    # CMake automatically handles the relative path from the resolved file path to the CMAKE_INSTALL_PREFIX
    install(FILES ${SRC_REAL_PATH}
            DESTINATION "${DES_FULL_DIR}"
    )
  endforeach()

endfunction()
