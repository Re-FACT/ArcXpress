# FindGraphviz.cmake
# - Try to find Graphviz
#
# Once done, this will define:
#  Graphviz_FOUND        - System has Graphviz
#  Graphviz_INCLUDE_DIRS - Graphviz include directories
#  Graphviz_LIBRARIES    - Libraries needed to use Graphviz
#  Graphviz_VERSION      - Graphviz version
#
# Optional COMPONENTS: gvc cgraph cdt
#
# Created based on the reference from:
# barista/cmake/FindGraphviz.cmake at master · usc-sail/barista

# Use pkg-config if available
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GRAPHVIZ QUIET libgvc)
endif()

# Find include directory
find_path(Graphviz_INCLUDE_DIR
  NAMES cgraph.h gvc.h version.h graphviz_version.h
  PATHS ${PC_GRAPHVIZ_INCLUDE_DIRS}
        /usr/include/graphviz
        /usr/local/include
        /opt/local/include
        /sw/include
)

# Find libraries
find_library(GRAPHVIZ_GVC_LIBRARY
  NAMES gvc
  PATHS ${PC_GRAPHVIZ_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
)

find_library(GRAPHVIZ_CGRAPH_LIBRARY
  NAMES cgraph
  PATHS ${PC_GRAPHVIZ_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
)

find_library(GRAPHVIZ_CDT_LIBRARY
  NAMES cdt
  PATHS ${PC_GRAPHVIZ_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
)

# Optional: find pathplan library
find_library(GRAPHVIZ_PATHPLAN_LIBRARY
  NAMES pathplan
  PATHS ${PC_GRAPHVIZ_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
)

# Get version from pkg-config
if(PC_GRAPHVIZ_VERSION)
  set(Graphviz_VERSION ${PC_GRAPHVIZ_VERSION})
else()
  # Try to extract version from header if pkg-config didn't provide it
  if(Graphviz_INCLUDE_DIR)
    # Check if version.h or graphviz_version.h exists before attempting to read it
    if(EXISTS "${Graphviz_INCLUDE_DIR}/version.h")
      file(STRINGS "${Graphviz_INCLUDE_DIR}/version.h" GRAPHVIZ_VERSION_MAJOR
           REGEX "#define PACKAGE_VERSION_MAJOR [0-9]+")
      file(STRINGS "${Graphviz_INCLUDE_DIR}/version.h" GRAPHVIZ_VERSION_MINOR
           REGEX "#define PACKAGE_VERSION_MINOR [0-9]+")
      file(STRINGS "${Graphviz_INCLUDE_DIR}/version.h" GRAPHVIZ_VERSION_PATCH
           REGEX "#define PACKAGE_VERSION_PATCHLEVEL [0-9]+")

      if(GRAPHVIZ_VERSION_MAJOR AND GRAPHVIZ_VERSION_MINOR AND GRAPHVIZ_VERSION_PATCH)
        string(REGEX REPLACE "#define PACKAGE_VERSION_MAJOR ([0-9]+)" "\\1" GRAPHVIZ_VERSION_MAJOR ${GRAPHVIZ_VERSION_MAJOR})
        string(REGEX REPLACE "#define PACKAGE_VERSION_MINOR ([0-9]+)" "\\1" GRAPHVIZ_VERSION_MINOR ${GRAPHVIZ_VERSION_MINOR})
        string(REGEX REPLACE "#define PACKAGE_VERSION_PATCHLEVEL ([0-9]+)" "\\1" GRAPHVIZ_VERSION_PATCH ${GRAPHVIZ_VERSION_PATCH})
        set(Graphviz_VERSION "${GRAPHVIZ_VERSION_MAJOR}.${GRAPHVIZ_VERSION_MINOR}.${GRAPHVIZ_VERSION_PATCH}")
      endif()
    elseif(EXISTS "${Graphviz_INCLUDE_DIR}/graphviz_version.h")
      # If version.h doesn't exist, try to read graphviz_version.h
      file(STRINGS "${Graphviz_INCLUDE_DIR}/graphviz_version.h" GRAPHVIZ_VERSION
           REGEX "#define PACKAGE_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+)\"")
      if(GRAPHVIZ_VERSION)
        set(Graphviz_VERSION "${GRAPHVIZ_VERSION}")
      else()
        message(WARNING "Unable to find valid version in graphviz_version.h")
      endif()
    else()
      message(WARNING "Graphviz version header 'version.h' or 'graphviz_version.h' not found at ${Graphviz_INCLUDE_DIR}/graphviz/")
    endif()
  endif()
endif()

# Handle the QUIETLY and REQUIRED arguments and set Graphviz_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Graphviz
  FOUND_VAR Graphviz_FOUND
  REQUIRED_VARS
    Graphviz_INCLUDE_DIR
    GRAPHVIZ_GVC_LIBRARY
    GRAPHVIZ_CGRAPH_LIBRARY
    GRAPHVIZ_CDT_LIBRARY
  VERSION_VAR Graphviz_VERSION
)

if(Graphviz_FOUND)
  # Set Graphviz variables
  set(Graphviz_INCLUDE_DIRS ${Graphviz_INCLUDE_DIR})

  # Collect libraries
  set(Graphviz_LIBRARIES
    ${GRAPHVIZ_GVC_LIBRARY}
    ${GRAPHVIZ_CGRAPH_LIBRARY}
    ${GRAPHVIZ_CDT_LIBRARY}
  )

  # Add optional pathplan library if found
  if(GRAPHVIZ_PATHPLAN_LIBRARY)
    list(APPEND Graphviz_LIBRARIES ${GRAPHVIZ_PATHPLAN_LIBRARY})
  endif()

  # Create imported targets for modern CMake usage
  if(NOT TARGET Graphviz::gvc)
    add_library(Graphviz::gvc UNKNOWN IMPORTED)
    set_target_properties(Graphviz::gvc PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Graphviz_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${GRAPHVIZ_GVC_LIBRARY}"
    )
  endif()

  if(NOT TARGET Graphviz::cgraph)
    add_library(Graphviz::cgraph UNKNOWN IMPORTED)
    set_target_properties(Graphviz::cgraph PROPERTIES
      IMPORTED_LOCATION "${GRAPHVIZ_CGRAPH_LIBRARY}"
    )
  endif()

  if(NOT TARGET Graphviz::cdt)
    add_library(Graphviz::cdt UNKNOWN IMPORTED)
    set_target_properties(Graphviz::cdt PROPERTIES
      IMPORTED_LOCATION "${GRAPHVIZ_CDT_LIBRARY}"
    )
  endif()

  if(NOT TARGET Graphviz::Graphviz)
    add_library(Graphviz::Graphviz INTERFACE IMPORTED)
    set_target_properties(Graphviz::Graphviz PROPERTIES
      INTERFACE_LINK_LIBRARIES "Graphviz::gvc;Graphviz::cgraph;Graphviz::cdt"
      INTERFACE_INCLUDE_DIRECTORIES "${Graphviz_INCLUDE_DIRS}"
    )
  endif()

  # Mark internal variables as advanced
  mark_as_advanced(
    Graphviz_INCLUDE_DIR
    GRAPHVIZ_GVC_LIBRARY
    GRAPHVIZ_CGRAPH_LIBRARY
    GRAPHVIZ_CDT_LIBRARY
    GRAPHVIZ_PATHPLAN_LIBRARY
  )
endif()
