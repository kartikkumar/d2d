# Copyright (c) 2010-2014, Delft University of Technology
# Copyright (c) 2010-2014, K. Kumar (me@kartikkumar.com)
# All rights reserved.
# See http://bit.ly/12SHPLR for license details.

# Specify minimum CMake version required.
cmake_minimum_required(VERSION 2.6)

# Specify project name.
project(D2D)

# Define the directory with the source code.
set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Define the project root directory.
set(PROJECT_ROOT "${SOURCE_ROOT}/..")

# Set path to local CMake modules.
set(CMAKE_MODULE_PATH ${PROJECT_ROOT}/CMakeModules)

# Set testing options based on platform.
enable_testing()

# Set platform-specific compiler flags.
if (WIN32)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /Ox /W3 /FC -D_SCL_SECURE_NO_WARNINGS")
    # Because we are using static boost libraries, with static runtime, we need to force MSVC to
    # also use static runtime: (from http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace).
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

# -------------------------------------------------------------------------------------------------
# DO NOT MANUALLY EDIT ANY SETTINGS AFTER THIS POINT IN THE FILE.
# 
# Pass any additional settings directly to CMake (e.g., via command-line).
# -------------------------------------------------------------------------------------------------

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

# Set lib, bin and test directories.
if(NOT LIB_ROOT)
  set(LIB_ROOT "${PROJECT_BINARY_DIR}/../lib")
endif(NOT LIB_ROOT)
if(NOT BIN_ROOT)
  set(BIN_ROOT "${PROJECT_BINARY_DIR}/..//bin")
endif(NOT BIN_ROOT)
if(NOT TEST_ROOT)
  set(TEST_ROOT "${PROJECT_BINARY_DIR}/../test")
endif(NOT TEST_ROOT)

# Set the global macros for setting up targets.
macro(setup_library_target target_name)
  set_property(TARGET ${target_name} PROPERTY LIBRARY_OUTPUT_DIRECTORY "${LIB_ROOT}")
  set_property(TARGET ${target_name} PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${LIB_ROOT}")
endmacro(setup_library_target)

macro(setup_executable_target target_name)
  set_property(TARGET ${target_name} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${BIN_ROOT}")
  install(TARGETS ${target_name} RUNTIME DESTINATION "${BIN_ROOT}")
endmacro(setup_executable_target)

macro(setup_test_target target_name)
  set_property(TARGET ${target_name} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${TEST_ROOT}")
  get_property(CUSTOM_TEST_PROGRAM_NAME TARGET ${target_name} PROPERTY OUTPUT_NAME)
  add_test("${target_name}" "${TEST_ROOT}/${target_name}")
endmacro(setup_test_target)

# Include the top-level directories.
include_directories(AFTER "${PROJECT_ROOT}")

# Find required libraries on local system.
#find_package(Atom REQUIRED)
find_package(CATCH REQUIRED)
find_package(GSL REQUIRED)
find_package(SGP4 REQUIRED)
find_package(TudatCore 2 REQUIRED)
find_package(Tudat 2 REQUIRED)

# Include directories.
# Set CMake flag to suppress warnings on Apple.
if(NOT APPLE)
  include_directories(AFTER SYSTEM
                      #"${ATOM_INCLUDE_DIR}"
                      "${CATCH_INCLUDE_DIR}"
                      "${GSL_INCLUDE_DIRS}" 
                      "${SGP4_INCLUDE_DIRS}"
                      "${SAM_INCLUDE_DIRS}"
                      "${SML_INCLUDE_DIRS}")

else(APPLE)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${ATOM_INCLUDE_DIR}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${CATCH_INCLUDE_DIR}\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${GSL_INCLUDE_DIRS}\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SGP4_INCLUDE_DIRS}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${TUDAT_CORE_INCLUDE_DIR}\"")  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${TUDAT_INCLUDE_DIR}\"")        
endif(NOT APPLE)

# If documentation should be built, find Doxygen package and setup config file.
if(BUILD_DOCS)
  find_package(Doxygen)

  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR 
            "Doxygen is needed to build the documentation. Please install it correctly.")
  endif(NOT DOXYGEN_FOUND)

  # Configure the Template Doxyfile for our specific project.
  configure_file(${PROJECT_ROOT}/Doxyfile.in 
                 ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)

  # Add a custom target to run Doxygen when ever the project is built.
  # If you do NOT want the documentation to be generated EVERY time you build the project
  # then leave out the 'ALL' keyword from the command below.
  add_custom_target (docs ALL COMMAND ${DOXYGEN_EXECUTABLE} 
                     ${PROJECT_BINARY_DIR}/Doxyfile 
                     SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif(BUILD_DOCS)

# Add tests for Atom.
set(D2D_TESTS
#  "${SOURCE_ROOT}/Tests/testD2d.cpp"
)

#add_executable(d2d_tests ${D2D_TESTS})
#setup_test_target(d2d_tests)
#target_link_libraries(d2d_tests
#                      ${SGP4_LIBRARY}
#                      ${GSL_LIBRARIES})

# Add D2D app.
add_executable(d2d "${SOURCE_ROOT}/d2d.cpp")
setup_executable_target(d2d)
target_link_libraries(d2d)
                      