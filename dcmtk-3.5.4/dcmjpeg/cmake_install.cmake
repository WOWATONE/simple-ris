# Install script for directory: D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "D:/workspace/dcmtk-3.5.4-trunk/../dcmtk-3.5.4-win32-i386")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Release")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/libsrc/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/libijg8/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/libijg12/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/libijg16/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/apps/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/include/dcmtk/dcmjpeg/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

