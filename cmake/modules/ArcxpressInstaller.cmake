# Special for Yosys, which does not use CMake to install. Do manual install configuration here
include(GNUInstallDirs)

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
