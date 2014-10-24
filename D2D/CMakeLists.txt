# Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
# All rights reserved.

cmake_minimum_required(VERSION 2.6)
project(D2D)

enable_testing()

set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_ROOT "${SOURCE_ROOT}/..")
set(CMAKE_MODULE_PATH ${PROJECT_ROOT}/CMakeModules)

OPTION(INSTALL_HEADERS          "Install header files" OFF)
OPTION(BUILD_DOCS               "Build docs" OFF)
OPTION(BUILD_TESTS              "Build tests" OFF)

# Set platform-specific compiler flags.
if (WIN32)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /Ox /W3 /FC -D_SCL_SECURE_NO_WARNINGS")
    # We need to force MSVC to use static runtime: 
    # (from http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace).
    foreach(flag_var
          CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      # Find all dynamic runtime (MD) references and replace with static (MT)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  else()
    set(CMAKE_CXX_FLAGS 
        "${CMAKE_CXX_FLAGS} -Wall -g -Woverloaded-virtual -Wold-style-cast -Wnon-virtual-dtor")
  endif()
else()
  set(CMAKE_CXX_FLAGS 
      "${CMAKE_CXX_FLAGS} -Wall -g -Woverloaded-virtual -Wold-style-cast -Wnon-virtual-dtor")
endif()

# Set CMake build-type. If it not supplied by the user, the default built type is "Release". 
if((NOT CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL "Release"))
    message(STATUS "WARNING: building release version!")
    set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "WARNING: building debug version!")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

# Set build-system directories.
if(NOT LIB_ROOT)
  set(LIB_ROOT "${PROJECT_BINARY_DIR}/../lib")
endif(NOT LIB_ROOT)
if(NOT BIN_ROOT)
  set(BIN_ROOT "${PROJECT_BINARY_DIR}/../bin")
endif(NOT BIN_ROOT)

include_directories(AFTER "${PROJECT_ROOT}")

# Install header files. Destination is set by CMAKE_INSTALL_PREFIX and defaults to usual locations,
# unless overridden by user.
install(DIRECTORY ${PROJECT_ROOT}/D2D 
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING PATTERN "*.hpp"
                       PATTERN "Tests" EXCLUDE)

# Find required libraries on local system.
find_package(Atom REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(GSL REQUIRED)
find_package(SAM REQUIRED)
find_package(SML REQUIRED)
find_package(SGP4 REQUIRED)
find_package(TudatCore 2 REQUIRED)
find_package(Tudat 2 REQUIRED)
if(NOT APPLE)
  include_directories(AFTER SYSTEM
                      "${ATOM_INCLUDE_DIR}"
                      "${EIGEN3_INCLUDE_DIR}"                      
                      "${GSL_INCLUDE_DIRS}" 
                      "${SAM_INCLUDE_DIRS}"
                      "${SML_INCLUDE_DIRS}"
                      "${SGP4_INCLUDE_DIRS}"
                      "${TUDAT_CORE_INCLUDE_DIR}"
                      "${TUDAT_INCLUDE_DIR}")

else(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${ATOM_INCLUDE_DIR}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${EIGEN3_INCLUDE_DIR}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${GSL_INCLUDE_DIRS}\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SAM_INCLUDE_DIRS}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SML_INCLUDE_DIRS}\"")    
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SGP4_INCLUDE_DIRS}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${TUDAT_CORE_INCLUDE_DIR}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${TUDAT_INCLUDE_DIR}\"")        
endif(NOT APPLE)

# If documentation should be built, find Doxygen package and setup config file.
if(BUILD_DOCS)
  find_package(Doxygen)

  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR 
            "Doxygen is needed to build the documentation. Please make sure it is installed.")
  endif(NOT DOXYGEN_FOUND)

  configure_file(${PROJECT_ROOT}/Doxyfile.in 
                 ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)

  # Add a custom target to run Doxygen whenever the project is built.
  # If you do NOT want the documentation to be generated EVERY time you build the project
  # then leave out the 'ALL' keyword from the command below.
  add_custom_target (docs ALL COMMAND ${DOXYGEN_EXECUTABLE} 
                     ${PROJECT_BINARY_DIR}/Doxyfile 
                     SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif(BUILD_DOCS)

# Add tests for D2D.
if(BUILD_TESTS)
  if(NOT TEST_ROOT)
    set(TEST_ROOT "${PROJECT_BINARY_DIR}/../test")
  endif()

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TEST_ROOT})

  find_package(CATCH REQUIRED)
  if(NOT APPLE)
    include_directories(SYSTEM AFTER "${CATCH_INCLUDE_DIR}")
  else(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${CATCH_INCLUDE_DIR}\"")
  endif(NOT APPLE)

  set(D2D_TESTS
    "${SOURCE_ROOT}/Tests/testD2d.cpp"   
  )

  add_executable(d2d_tests ${D2D_TESTS})
  target_link_libraries(d2d_tests)
  add_test(NAME d2d_tests COMMAND "${TEST_ROOT}/d2d_tests") 
endif(BUILD_TESTS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_ROOT})
add_executable(d2d "${SOURCE_ROOT}/d2d.cpp")
target_link_libraries(d2d)
                      