 #    Copyright (c) 2014, Dinamica Srl
 #    Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
 #    All rights reserved.
 #    See COPYING for license details.

macro(_sgp4_check_version)
  message(STATUS "Checking for SGP4 in: " ${SGP4_BASE_PATH})    
  file(READ "${SGP4_BASE_PATH}/VERSION" _sgp4_header)

  string(REGEX MATCH "define[ \t]+SGP4_VERSION_MAJOR[ \t]+([0-9]+)" sgp4_major_version_match "${_sgp4_header}")
  set(SGP4_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+SGP4_VERSION_MINOR[ \t]+([0-9]+)" sgp4_minor_version_match "${_sgp4_header}")
  set(SGP4_MINOR_VERSION "${CMAKE_MATCH_1}")

  set(SGP4_VERSION ${SGP4_MAJOR_VERSION}.${SGP4_MINOR_VERSION})

  if(${SGP4_VERSION} VERSION_LESS ${SGP4_FIND_VERSION})
    set(SGP4_VERSION_OK FALSE)
  else(${SGP4_VERSION} VERSION_LESS ${SGP4_FIND_VERSION})
    set(SGP4_VERSION_OK TRUE)
  endif(${SGP4_VERSION} VERSION_LESS ${SGP4_FIND_VERSION})

  if(NOT SGP4_VERSION_OK)
    message(STATUS "SGP4 version ${SGP4_VERSION} found in ${SGP4_INCLUDE_DIR}, "
                   "but at least version ${SGP4_FIND_VERSION} is required!")
  endif(NOT SGP4_VERSION_OK)

  set(SGP4_LIBRARY "sgp4")
  set(SGP4_INCLUDE_DIR ${SGP4_BASE_PATH}/../)
  set(SGP4_LIBRARYDIR ${SGP4_BASE_PATH}/build/cpp/)
  link_directories(${SGP4_LIBRARYDIR})
endmacro(_sgp4_check_version)

if (SGP4_BASE_PATH)
  # in cache already
  _sgp4_check_version()
  set(SGP4_FOUND ${SGP4_VERSION_OK})

else (SGP4_BASE_PATH)
  find_path(SGP4_BASE_PATH NAMES VERSION
      PATHS
      C:
      "$ENV{ProgramFiles}"
      /usr/local      
      ${PROJECT_SOURCE_DIR}/..
      ${PROJECT_SOURCE_DIR}/../..
      ${PROJECT_SOURCE_DIR}/../../..
      ${PROJECT_SOURCE_DIR}/../../../..
      PATH_SUFFIXES sgp4 
    )

  if(SGP4_BASE_PATH)
    _sgp4_check_version()
  endif(SGP4_BASE_PATH)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SGP4 DEFAULT_MSG SGP4_BASE_PATH SGP4_VERSION_OK)

  mark_as_advanced(SGP4_BASE_PATH)

endif(SGP4_BASE_PATH)

 #    References
 #      FindEigen3.cmake.
 #
 #    This script tries to find the SGP4 library. Note that this is the original SGP4 library
 #    released on CelesTrak (http://celestrak.com/publications/AIAA/2006-6753/).
 #
 #    This module supports requiring a minimum version, e.g. you can do version, e.g. you can do
 #    find_package(SGP4 3.1.2) to require version 3.1.2 or newer of SGP4.
 #
 #    Once done, this will define:
 #
 #        SGP4_FOUND - system has SGP4 lib with correct version;
 #        SGP4_INCLUDE_DIR - the SGP4 include directory.
 #
 #    Original copyright statements (from FindEigen3.cmake:
 #        Copyright (c) 2006, 2007 Montel Laurent, <montel@kde.org>
 #        Copyright (c) 2008, 2009 Gael Guennebaud, <g.gael@free.fr>
 #        Copyright (c) 2009 Benoit Jacob <jacob.benoit.1@gmail.com>
 #
 #    FindEigen3.cmake states that redistribution and use is allowed according to the terms of
 #    the 2-clause BSD license.