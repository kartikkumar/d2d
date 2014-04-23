 #    Copyright (c) 2014, Dinamica Srl
 #    Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
 #    All rights reserved.
 #    See COPYING for license details.

macro(_pagmo_check_version)
  file(READ "${PAGMO_INCLUDE_DIR}/../VERSION" _pagmo_header)

  string(REGEX MATCH "define[ \t]+PAGMO_VERSION_MAJOR[ \t]+([0-9]+)" _pagmo_major_version_match "${_pagmo_header}")
  set(PAGMO_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+PAGMO_VERSION_MINOR[ \t]+([0-9]+)" _pagmo_minor_version_match "${_pagmo_header}")
  set(PAGMO_MINOR_VERSION "${CMAKE_MATCH_1}")

  set(PAGMO_VERSION ${PAGMO_MAJOR_VERSION}.${PAGMO_MINOR_VERSION})

  if(${PAGMO_VERSION} VERSION_LESS ${PaGMO_FIND_VERSION})
    set(PAGMO_VERSION_OK FALSE)
  else(${PAGMO_VERSION} VERSION_LESS ${PaGMO_FIND_VERSION})
    set(PAGMO_VERSION_OK TRUE)
  endif(${PAGMO_VERSION} VERSION_LESS ${PaGMO_FIND_VERSION})

  if(NOT PAGMO_VERSION_OK)
    message(STATUS "PaGMO version ${PAGMO_VERSION} found in ${PAGMO_INCLUDE_DIR}, "
                   "but at least version ${PaGMO_FIND_VERSION} is required!")
  endif(NOT PAGMO_VERSION_OK)

  set(PAGMO_LIBRARY "pagmo")
  link_directories(${PAGMO_LIBRARYDIR})
endmacro(_pagmo_check_version)

if (PAGMO_INCLUDE_DIR)
  # in cache already
  _pagmo_check_version()
  set(PAGMO_FOUND ${PAGMO_VERSION_OK})

else (PAGMO_INCLUDE_DIR)
  find_path(PAGMO_BASE_PATH NAMES VERSION
      PATHS
      ${PROJECT_SOURCE_DIR}/../pagmo
      ${PROJECT_SOURCE_DIR}/../../pagmo
      ${PROJECT_SOURCE_DIR}/../../../pagmo
      ${PROJECT_SOURCE_DIR}/../../../../pagmo
      PATH_SUFFIXES pagmo 
    )
  set(PAGMO_INCLUDE_DIR ${PAGMO_BASE_PATH}/src/)
  set(PAGMO_LIBRARYDIR ${PAGMO_BASE_PATH}/build/src/)

  if(PAGMO_INCLUDE_DIR)
    _pagmo_check_version()
  endif(PAGMO_INCLUDE_DIR)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PaGMO DEFAULT_MSG PAGMO_INCLUDE_DIR PAGMO_VERSION_OK)

  mark_as_advanced(PAGMO_INCLUDE_DIR)

endif(PAGMO_INCLUDE_DIR)

 #    References
 #      FindEigen3.cmake.
 #
 #    This script tries to find the PaGMO library. This module supports requiring a minimum 
 #    version, e.g. you can do version, e.g. you can do find_package(PaGMO 3.1.2) to require
 #    version 3.1.2 or newer of PaGMO.
 #
 #    Once done, this will define:
 #
 #        PAGMO_FOUND - system has PaGMO lib with correct version;
 #        PAGMO_INCLUDE_DIR - the PaGMO include directory.
 #
 #    Original copyright statements (from FindEigen3.cmake:
 #        Copyright (c) 2006, 2007 Montel Laurent, <montel@kde.org>
 #        Copyright (c) 2008, 2009 Gael Guennebaud, <g.gael@free.fr>
 #        Copyright (c) 2009 Benoit Jacob <jacob.benoit.1@gmail.com>
 #
 #    FindEigen3.cmake states that redistribution and use is allowed according to the terms of
 #    the 2-clause BSD license.