# Special for Yosys, which does not use CMake to install. Do manual install configuration here
if (BUILD_ARKANGEL_WITH_BUILTIN_FRONTEND)
  include(GNUInstallDirs)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs/arkangel_frontend/yosys/bin/ 
          DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT engine_package
          PATTERN "*"
          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
  )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs/arkangel_frontend/yosys/share/ 
          DESTINATION ${CMAKE_INSTALL_DATADIR}
          COMPONENT engine_package
  )
endif()

# Installer should include all the etc files except the arch generator source files
install(DIRECTORY etc/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
        COMPONENT device_package
        PATTERN "*.cpp" EXCLUDE
        PATTERN "*.h" EXCLUDE
        PATTERN "*.hpp" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "etc/benchmarks" EXCLUDE  # Excludes all benchmarks 
        PATTERN "etc/benchmark_config" EXCLUDE  # Excludes all benchmarks 
)
# Only include the checkin benchmarks
include(InstallDereferencedDirectory)
install_dereferenced_directory(
   "${CMAKE_SOURCE_DIR}/etc/benchmarks/essential"
   "${CMAKE_INSTALL_SYSCONFDIR}/benchmarks/essential"
)
install(FILES etc/benchmark_config/checkin.xml
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/benchmarks/"
        COMPONENT device_package
)

# Installer includes the complete env
# Use the result in conditional logic
if (WIN32)
  if (DEFINED ENV{MSYSTEM})
    if (INSTALLER_WITH_CUSTOM_ENV STREQUAL "ON")
      message(STATUS "Include $ENV{MSYSTEM} env in the installer")
      cmake_path(SET NORM_ENV_PATH "$ENV{CUSTOM_ENV_ROOT}")
      cmake_path(NORMAL_PATH NORM_ENV_PATH)
      install(DIRECTORY  ${NORM_ENV_PATH}
              DESTINATION env/eda
              COMPONENT env_package
      )
    endif() 
  endif() 
else()
  if("${OS_FLAVOR}" STREQUAL "RHEL" OR "${OS_FLAVOR}" STREQUAL "CentOS")
    message(STATUS "Detected Red Hat/CentOS based system. OS flavor: ${OS_FLAVOR}")
    # Add your specific RHEL/CentOS instructions here
    # Only available in centos 7.9
    if (INSTALLER_WITH_CUSTOM_ENV STREQUAL "ON")
      message(STATUS "Include CentOS/RedHat 7.9 env in the installer")
      install(DIRECTORY  /eda/tools/anaconda3/envs/eda/
              DESTINATION env/eda
              COMPONENT env_package
      )
    endif()
  elseif("${OS_FLAVOR}" STREQUAL "Ubuntu")
    message(STATUS "Detected Ubuntu system.")
    # Add your specific Ubuntu instructions here
  elseif("${OS_FLAVOR}" STREQUAL "Ubuntu")
    message(STATUS "Detected other Linux flavor or generic Linux: ${OS_FLAVOR}")
  endif()
endif()


if (INSTALLER_WITH_DOC STREQUAL "ON")
  # Include all the documentation
  message(STATUS "Installer will include documentation")
  install(DIRECTORY docs/build
          DESTINATION ${CMAKE_INSTALL_DOCDIR}
          COMPONENT doc_package
  )
endif()

# Include licenses
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION "."
        COMPONENT engine_package
)
