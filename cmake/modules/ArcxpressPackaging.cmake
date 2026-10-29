# Installer creation
set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "The resulting package name")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ArkAngel Engineering design tool suites" CACHE STRING "ArkAngel Engineering design tool suites")
set(CPACK_PACKAGE_VENDOR "RapidFlex")
set(CPACK_PACKAGE_CONTACT "info@rapid-flex.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "RapidFlex")
# Due to complicated environment setup in windows (package installation and management are not done by single tool like apt)
# Cannot use the dependency list here
if (NOT WIN32)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libreadline, libstdc++6, iverilog, libffi-dev, libgcc1, libc6 libssl-dev")
endif()
set(CPACK_PACKAGE_VERSION ${VERSION_NUMBER})
set(CPACK_PACKAGE_VERSION_MAJOR ${ARKANGEL_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${ARKANGEL_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${ARKANGEL_VERSION_PATCH})
# Pass on the build type so that cpack can find files under debug build
set(CPACK_BUILD_CONFIG ${CMAKE_BUILD_TYPE})

if (ARKANGEL_BUILD_LICENSED_RELEASE)
# ignore rsa files and header files for licensed release
# add information
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/dev/general_license_info.txt)
    set(CPACK_RESOURCE_FILE_WELCOME ${CMAKE_CURRENT_SOURCE_DIR}/dev/general_welcome_info.txt)
    set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/dev/general_description_info.txt)
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# Try to improve the runtime, use as many cores as possible
set(CPACK_THREADS "0")
# we are interested in cross-platform support. IFW is the only generator that allows for it, so set it explicitly
if (INSTALLER_WITH_GUI) 
  set(CPACK_GENERATOR "IFW")
else()
  set(CPACK_GENERATOR DEB)
endif()

if (CPACK_GENERATOR STREQUAL "IFW")
  # set some IFW specific variables, which can be derived from the more generic variables given above
  set(CPACK_IFW_VERBOSE ON)
  set(CPACK_IFW_PACKAGE_TITLE ${CPACK_PACKAGE_NAME})
  set(CPACK_IFW_PACKAGE_PUBLISHER ${CPACK_PACKAGE_VENDOR})
  set(CPACK_IFW_PRODUCT_URL "https://www.rapid-flex.com")
  set(CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH OFF)
  
  # create a more memorable name for the maintenance tool (used for uninstalling the package)
  set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME ${PROJECT_NAME}_MaintenanceTool)
  set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE ${CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME}.ini)

  # customise the theme if required
  set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Modern")
  
  # adjust the default size
  set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT 400)
  
  # set the installer icon and logo
  if(WIN32)
    set(CPACK_IFW_PACKAGE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/dev/resources/logo.ico)
    elseif(APPLE)
    set(CPACK_IFW_PACKAGE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/dev/resources/logo.icns)
  endif()
  set(CPACK_IFW_PACKAGE_WINDOWS_LOGO ${CMAKE_CURRENT_SOURCE_DIR}/dev/resources/logo.png)
  set(CPACK_IFW_PACKAGE_LOGO ${CMAKE_CURRENT_SOURCE_DIR}/dev/resources/logo.png)
endif() 

# Resolve the bug where install dir is the same as the build dir 
set(CPACK_BUILD_CONFIG ${CMAKE_BUILD_TYPE})
# Variables must be defined before including the CPACK module
include(CPack)
include(CPackIFW)

cpack_add_component(Unspecified
  REQUIRED
  DISPLAY_NAME "ArkAngel Tools"
  DESCRIPTION "ArkAngel Foundational Tools"
)

cpack_add_component(engine_package
  REQUIRED
  DISPLAY_NAME "ArkAngel Engine"
  DESCRIPTION "Main engine for ArkAngel Design Tool Suites"
)

cpack_add_component(device_package
  REQUIRED
  DISPLAY_NAME "RapidFlex eFPGA devices"
  DESCRIPTION "RapidFlex eFPGA device libraries, including architecture template, built-in programmable IPs and utility tools"
)

if (INSTALLER_WITH_CUSTOM_CENTOS_ENV STREQUAL "ON")
  cpack_add_component(env_package
    REQUIRED
    DISPLAY_NAME "RapidFlex CentOS 7.9 env"
    DESCRIPTION "RapidFlex CentOS 7.9 env"
  )
endif()

if (INSTALLER_WITH_DOC STREQUAL "ON")
  message(STATUS "Installer has a dedicated component for documentation")
  cpack_add_component(doc_package
    DISPLAY_NAME "Documentation"
    DESCRIPTION "Documentation and Tutorials"
  )
endif()

if (CPACK_GENERATOR STREQUAL "IFW")
  cpack_ifw_configure_component(Unspecified
    REQUIRED
    SORTING_PRIORITY 3
    LICENSES "License" ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE 
  )

  cpack_ifw_configure_component(engine_package
    REQUIRED
    SORTING_PRIORITY 2
    LICENSES "License" ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE 
  )

  cpack_ifw_configure_component(device_package
    REQUIRED
    SORTING_PRIORITY 1
    LICENSES "License" ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE 
  )

  if (INSTALLER_WITH_CUSTOM_CENTOS_ENV STREQUAL "ON")
    cpack_ifw_configure_component(env_package
      REQUIRED
      SORTING_PRIORITY 1
      LICENSES "License" ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE 
    )
  endif()

  if (INSTALLER_WITH_DOC STREQUAL "ON")
    cpack_ifw_configure_component(doc_package
      REQUIRED
      SORTING_PRIORITY 0
      LICENSES "License" ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE 
    )
  endif() 
endif() 
