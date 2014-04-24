 #    Copyright (c) 2014, Dinamica Srl
 #    Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
 #    All rights reserved.
 #    See COPYING for license details.

macro(_sgp4danrw_check_version)
  message(STATUS "Checking for SGP4DanRW in: " ${SGP4DANRW_BASE_PATH})    
  file(READ "${SGP4DANRW_BASE_PATH}/VERSION" _sgp4danrw_header)

  string(REGEX MATCH "define[ \t]+SGP4DANRW_VERSION_MAJOR[ \t]+([0-9]+)" sgp4danrw_major_version_match "${_sgp4danrw_header}")
  set(SGP4DANRW_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+SGP4DANRW_VERSION_MINOR[ \t]+([0-9]+)" sgp4danrw_minor_version_match "${_sgp4danrw_header}")
  set(SGP4DANRW_MINOR_VERSION "${CMAKE_MATCH_1}")

  set(SGP4DANRW_VERSION ${SGP4DANRW_MAJOR_VERSION}.${SGP4DANRW_MINOR_VERSION})

  if(${SGP4DANRW_VERSION} VERSION_LESS ${SGP4DanRW_FIND_VERSION})
    set(SGP4DANRW_VERSION_OK FALSE)
  else(${SGP4DANRW_VERSION} VERSION_LESS ${SGP4DanRW_FIND_VERSION})
    set(SGP4DANRW_VERSION_OK TRUE)
  endif(${SGP4DANRW_VERSION} VERSION_LESS ${SGP4DanRW_FIND_VERSION})

  if(NOT SGP4DANRW_VERSION_OK)
    message(STATUS "SGP4 version ${SGP4DANRW_VERSION} found in ${SGP4DANRW_INCLUDE_DIR}, "
                   "but at least version ${SGP4DanRW_FIND_VERSION} is required!")
  endif(NOT SGP4DANRW_VERSION_OK)

  set(SGP4DANRW_LIBRARY "sgp4")
  set(SGP4DANRW_INCLUDE_DIR ${SGP4DANRW_BASE_PATH})
  set(SGP4DANRW_LIBRARYDIR ${SGP4DANRW_BASE_PATH}/libsgp4/)
  link_directories(${SGP4DANRW_LIBRARYDIR})
endmacro(_sgp4danrw_check_version)

if (SGP4DANRW_BASE_PATH)
  # in cache already
  _sgp4danrw_check_version()
  set(SGP4DANRW_FOUND ${SGP4DANRW_VERSION_OK})

else (SGP4DANRW_BASE_PATH)
  find_path(SGP4DANRW_BASE_PATH NAMES VERSION
      PATHS
      C:
      "$ENV{ProgramFiles}"
      /usr/local      
      ${PROJECT_SOURCE_DIR}/..
      ${PROJECT_SOURCE_DIR}/../..
      ${PROJECT_SOURCE_DIR}/../../..
      ${PROJECT_SOURCE_DIR}/../../../..
      PATH_SUFFIXES sgp4danrw
    )

  if(SGP4DANRW_BASE_PATH)
    _sgp4danrw_check_version()
  endif(SGP4DANRW_BASE_PATH)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SGP4 DEFAULT_MSG SGP4DANRW_BASE_PATH SGP4DANRW_VERSION_OK)

  mark_as_advanced(SGP4DANRW_BASE_PATH)

endif(SGP4DANRW_BASE_PATH)

 #    References
 #      FindEigen3.cmake.
 #
 #    This script tries to find the SGP4 library. Note that this is the original SGP4 library
 #    released on CelesTrak (http://celestrak.com/publications/AIAA/2006-6753/).
 #
 #    This module supports requiring a minimum 
 #    version, e.g. you can do version, e.g. you can do find_package(SGP4 3.1.2) to require
 #    version 3.1.2 or newer of SGP4.
 #
 #    Once done, this will define:
 #
 #        SGP4DANRW_FOUND - system has SGP4 lib with correct version;
 #        SGP4DANRW_INCLUDE_DIR - the SGP4 include directory.
 #
 #    Original copyright statements (from FindEigen3.cmake:
 #        Copyright (c) 2006, 2007 Montel Laurent, <montel@kde.org>
 #        Copyright (c) 2008, 2009 Gael Guennebaud, <g.gael@free.fr>
 #        Copyright (c) 2009 Benoit Jacob <jacob.benoit.1@gmail.com>
 #
 #    FindEigen3.cmake states that redistribution and use is allowed according to the terms of
 #    the 2-clause BSD license.