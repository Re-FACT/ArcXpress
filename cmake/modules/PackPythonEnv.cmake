include(ExternalProject)

# Define where the external Python build will be installed temporarily
if(DEFINED ENV{MSYSTEM})
  set(PYTHON_VERSION "mingw-v3.14.4")
  set(PYTHON_SRC_URL "https://github.com/msys2-contrib/cpython-mingw.git")
else()
  set(PYTHON_VERSION "3.9.25")
  set(PYTHON_SRC_URL "https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz")
endif()
set(PYTHON_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/python${PYTHON_VERSION}")
set(PYTHON_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/_py_src/python-source")
set(PYTHON_EXEC "${PYTHON_INSTALL_DIR}/bin/python3.9")
set(PYTHON_WHEEL_DIR "${PYTHON_INSTALL_DIR}/vendor/wheels")

# Just for shared library creation 
set(PYTHON_SHARED_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/shared_python${PYTHON_VERSION}")
set(PYTHON_SHARED_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/_py_src_shared/python-source")

# Cocotb requires the site package binaries stay with the python binary
set(PYTHON_SITE_PACKAGE_DIR "${PYTHON_INSTALL_DIR}/vendor/site_packages")

# Extract directory paths from the found library files for Autotools
get_filename_component(TCL_LIB_DIR "${TCL_LIBRARY}" DIRECTORY)

# MINGW requires a special fork or python codes
if(DEFINED ENV{MSYSTEM})
  ExternalProject_Add(
      PythonExternal
      GIT_REPOSITORY "${PYTHON_SRC_URL}"
      GIT_TAG "${PYTHON_VERSION}"
      PREFIX "${PYTHON_SRC_DIR}"
      CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PYTHON_INSTALL_DIR} --enable-optimizations CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
      BUILD_COMMAND make -j${CMAKE_JOB_POOL}
      INSTALL_COMMAND make altinstall TEST_MODULES= TESTSUBDIRS=
  )
  
  ExternalProject_Add(
      PythonSharedExternal
      GIT_REPOSITORY "${PYTHON_SRC_URL}"
      GIT_TAG "${PYTHON_VERSION}"
      PREFIX "${PYTHON_SHARED_SRC_DIR}"
      CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PYTHON_SHARED_INSTALL_DIR} --enable-shared CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CPPFLAGS=${CMAKE_CXX_FLAGS} LDFLAGS=${CMAKE_SHARED_LINKER_FLAGS}
      BUILD_COMMAND make python -j${CMAKE_JOB_POOL}
      INSTALL_COMMAND ""
  )
else()
  ExternalProject_Add(
      PythonExternal
      URL "${PYTHON_SRC_URL}"
      SOURCE_DIR "${PYTHON_SRC_DIR}"
      CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PYTHON_INSTALL_DIR} --enable-optimizations CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} --with-tcltk-includes=-I${TCL_INCLUDE_PATH}\ -I${TK_INCLUDE_PATH} --with-tcltk-libs=-L${TCL_LIB_DIR}\ -ltcl${TCL_VERSION_MAJOR}.${TCL_VERSION_MINOR}\ -L${TK_LIB_DIR}\ -ltk${TK_VERSION_MAJOR}.${TK_VERSION_MINOR}
      BUILD_COMMAND make -j${CMAKE_JOB_POOL}
      INSTALL_COMMAND make altinstall TEST_MODULES= TESTSUBDIRS=
  )
  
  ExternalProject_Add(
      PythonSharedExternal
      URL "${PYTHON_SRC_URL}"
      SOURCE_DIR "${PYTHON_SHARED_SRC_DIR}"
      CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PYTHON_SHARED_INSTALL_DIR} --enable-shared CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CPPFLAGS=${CMAKE_CXX_FLAGS} LDFLAGS=${CMAKE_SHARED_LINKER_FLAGS}
      BUILD_COMMAND make python -j${CMAKE_JOB_POOL}
      INSTALL_COMMAND ""
  )
endif()
# Get shared library location
ExternalProject_Get_Property(PythonSharedExternal BINARY_DIR)
set(PYTHON_SHARED_LIB_DIR "${BINARY_DIR}")
