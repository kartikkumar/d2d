# Copyright (c) 2014 Kartik Kumar (me@kartikkumar.com)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# Include script to build external library with CMake.
include(ExternalProject)

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(rapidjson)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT RAPIDJSON_FOUND)
  message(STATUS "RapidJSON will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  ExternalProject_Add(rapidjson-lib
    PREFIX ${MYEXT_PATH}/RapidJson
    #--Download step--------------
    URL https://github.com/miloyip/rapidjson/archive/master.zip
    TIMEOUT 120
    #--Update/Patch step----------
    #--Configure step-------------
    CONFIGURE_COMMAND ""
    #--Build step-----------------
    BUILD_COMMAND ""
    #--Install step---------------
    INSTALL_COMMAND ""
    #--Output logging-------------
    LOG_DOWNLOAD ON
  )
  ExternalProject_Get_Property(rapidjson-lib source_dir)
  set(RAPIDJSON_INCLUDE_DIRS ${source_dir}/include
    CACHE INTERNAL "Path to include folder for RapidJSON")
  add_dependencies(${MYPROJ_LIB} rapidjson-lib)
endif(NOT RAPIDJSON_FOUND)

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${RAPIDJSON_INCLUDE_DIRS}")
else(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${RAPIDJSON_INCLUDE_DIRS}\"")
endif(NOT APPLE)

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(KeplerianToolbox)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT KEPLERIANTOOLBOX_FOUND)
  message(STATUS "KeplerianToolbox will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  ExternalProject_Add(keplerian_toolbox
    PREFIX ${MYEXT_PATH}/KeplerianToolbox
    #--Download step--------------
    URL https://github.com/esa/pykep/archive/master.zip
    TIMEOUT 120
    #--Update/Patch step----------
    #--Configure step-------------
    #--Build step-----------------
    BUILD_IN_SOURCE 1
    #--Install step---------------
    INSTALL_COMMAND ""
    #--Output logging-------------
    LOG_DOWNLOAD ON
  )
  ExternalProject_Get_Property(keplerian_toolbox source_dir)
  set(KEPLERIANTOOLBOX_INCLUDE_DIRS ${source_dir}/src
    CACHE INTERNAL "Path to include folder for KeplerianToolbox")
  set(KEPLERIANTOOLBOX_LIBRARY_DIR ${source_dir}
    CACHE INTERNAL "Path to include folder for KeplerianToolbox")
  set(KEPLERIANTOOLBOX_LIBRARY "keplerian_toolbox_static")
  add_dependencies(${MYPROJ_LIB} keplerian_toolbox)
endif(NOT KEPLERIANTOOLBOX_FOUND)

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${KEPLERIANTOOLBOX_INCLUDE_DIRS}")
else(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${KEPLERIANTOOLBOX_INCLUDE_DIRS}\"")
endif(NOT APPLE)
link_directories(${KEPLERIANTOOLBOX_LIBRARY_DIR})

find_package(Threads)

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(sqlite3)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT SQLITE3_FOUND)
  message(STATUS "SQLite3 will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  ExternalProject_Add(sqlite3-lib
    PREFIX ${MYEXT_PATH}/SQLite3
    #--Download step--------------
    URL https://github.com/kartikkumar/sqlite3-cmake/archive/master.zip
    TIMEOUT 120
    #--Update/Patch step----------
    #--Configure step-------------
    #--Build step-----------------
    BUILD_IN_SOURCE 1
    #--Install step---------------
    INSTALL_COMMAND ""
    #--Output logging-------------
    LOG_DOWNLOAD ON
  )
  ExternalProject_Get_Property(sqlite3-lib source_dir)
  set(SQLITE3_INCLUDE_DIR ${source_dir}/src CACHE INTERNAL "Path to include folder for SQLite3")
  set(SQLITE3_LIBRARY_DIR ${source_dir} CACHE INTERNAL "Path to library folder for SQLite3")
  set(SQLITE3_LIBRARY "sqlite3-static")
  add_dependencies(${MYPROJ_LIB} sqlite3-lib)
endif(NOT SQLITE3_FOUND)
link_directories(${SQLITE3_LIBRARY_DIR})

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${SQLITE3_INCLUDE_DIR}")
else(NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SQLITE3_INCLUDE_DIR}\"")
endif(NOT APPLE)

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(SQLiteCpp 0.099)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT SQLITECPP_FOUND)
  message(STATUS "SQLiteCpp will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  if(NOT SQLITE3_FOUND)
    ExternalProject_Add(sqlitecpp-lib
      DEPENDS sqlite3-lib
      PREFIX ${MYEXT_PATH}/SQLiteCpp
      #--Download step--------------
      URL https://github.com/kartikkumar/sqlitecpp/archive/master.zip
      TIMEOUT 120
      #--Update/Patch step----------
      #--Configure step-------------
      #--Build step-----------------
      BUILD_IN_SOURCE 1
      #--Install step---------------
      INSTALL_COMMAND ""
      #--Output logging-------------
      LOG_DOWNLOAD ON
    )
  else(NOT SQLITE3_FOUND)
    ExternalProject_Add(sqlitecpp-lib
    PREFIX ${MYEXT_PATH}/SQLiteCpp
    #--Download step--------------
    URL https://github.com/kartikkumar/sqlitecpp/archive/master.zip
    TIMEOUT 120
    #--Update/Patch step----------
    #--Configure step-------------
    #--Build step-----------------
    BUILD_IN_SOURCE 1
    #--Install step---------------
    INSTALL_COMMAND ""
    #--Output logging-------------
    LOG_DOWNLOAD ON
  )
  endif(NOT SQLITE3_FOUND)
  ExternalProject_Get_Property(sqlitecpp-lib source_dir)
  set(SQLITECPP_INCLUDE_DIRS ${source_dir}/include
    CACHE INTERNAL "Path to include folder for SQLiteCpp")
  set(SQLITECPP_LIBRARY_DIR ${source_dir} CACHE INTERNAL "Path to library folder for SQLiteCpp")
  set(SQLITECPP_LIBRARY "SQLiteCpp")
  add_dependencies(${MYPROJ_LIB} sqlitecpp-lib)
endif(NOT SQLITECPP_FOUND)

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${SQLITECPP_INCLUDE_DIRS}")
else(NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SQLITECPP_INCLUDE_DIRS}\"")
endif(NOT APPLE)
link_directories(${SQLITECPP_LIBRARY_DIR})

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(SGP4)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT SGP4_FOUND)
  message(STATUS "SGP4 will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  ExternalProject_Add(sgp4-deorbit
    PREFIX ${MYEXT_PATH}/SGP4
    #--Download step--------------
    URL https://github.com/kartikkumar/sgp4deorbit/archive/master.zip
    TIMEOUT 120
    #--Update/Patch step----------
    #--Configure step-------------
    #--Build step-----------------
    BUILD_IN_SOURCE 1
    #--Install step---------------
    INSTALL_COMMAND ""
    #--Output logging-------------
    LOG_DOWNLOAD ON
  )
  ExternalProject_Get_Property(sgp4-deorbit source_dir)
  set(SGP4_INCLUDE_DIRS ${source_dir} CACHE INTERNAL "Path to include folder for SGP4")
  set(SGP4_LIBRARY_DIR ${source_dir}/libsgp4 CACHE INTERNAL "Path to library folder for SGP4")
  set(SGP4_LIBRARY "sgp4")
  add_dependencies(${MYPROJ_LIB} sgp4-deorbit)
endif(NOT SGP4_FOUND)
link_directories(${SGP4_LIBRARY_DIR})

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${SGP4_INCLUDE_DIRS}")
else(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SGP4_INCLUDE_DIRS}\"")
endif(NOT APPLE)

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(SML)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT SML_FOUND)
  message(STATUS "SML will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  ExternalProject_Add(sml-lib
    PREFIX ${MYEXT_PATH}/SML
    #--Download step--------------
    URL https://github.com/kartikkumar/sml/archive/master.zip
    TIMEOUT 120
    #--Update/Patch step----------
    #--Configure step-------------
    CONFIGURE_COMMAND ""
    #--Build step-----------------
    BUILD_COMMAND ""
    #--Install step---------------
    INSTALL_COMMAND ""
    #--Output logging-------------
    LOG_DOWNLOAD ON
  )
  ExternalProject_Get_Property(sml-lib source_dir)
  set(SML_INCLUDE_DIRS ${source_dir}/include CACHE INTERNAL "Path to include folder for SML")
  add_dependencies(${MYPROJ_LIB} sml-lib)
endif(NOT SML_FOUND)

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${SML_INCLUDE_DIRS}")
else(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SML_INCLUDE_DIRS}\"")
endif(NOT APPLE)

if(NOT FORCE_DEPENDENCIES_BUILD)
  find_package(SAM)
endif(NOT FORCE_DEPENDENCIES_BUILD)

if(NOT SAM_FOUND)
  message(STATUS "SAM will be downloaded when ${CMAKE_PROJECT_NAME} is built")
  if(NOT SML_FOUND)
    ExternalProject_Add(sam-lib
      DEPENDS sml-lib
      PREFIX ${MYEXT_PATH}/SAM
      #--Download step--------------
      URL https://github.com/kartikkumar/sam/archive/master.zip
      TIMEOUT 120
      #--Update/Patch step----------
      #--Configure step-------------
      CONFIGURE_COMMAND ""
      #--Build step-----------------
      BUILD_COMMAND ""
      #--Install step---------------
      INSTALL_COMMAND ""
      #--Output logging-------------
      LOG_DOWNLOAD ON
    )
  else(NOT SML_FOUND)
    ExternalProject_Add(sam-lib
      PREFIX ${MYEXT_PATH}/SAM
      #--Download step--------------
      URL https://github.com/kartikkumar/sam/archive/master.zip
      TIMEOUT 120
      #--Update/Patch step----------
      #--Configure step-------------
      CONFIGURE_COMMAND ""
      #--Build step-----------------
      BUILD_COMMAND ""
      #--Install step---------------
      INSTALL_COMMAND ""
      #--Output logging-------------
      LOG_DOWNLOAD ON
    )
  endif(NOT SML_FOUND)
  ExternalProject_Get_Property(sam-lib source_dir)
  set(SAM_INCLUDE_DIRS ${source_dir}/include CACHE INTERNAL "Path to include folder for SAM")
  add_dependencies(${MYPROJ_LIB} sam-lib)
endif(NOT SAM_FOUND)

if(NOT APPLE)
  include_directories(SYSTEM AFTER "${SAM_INCLUDE_DIRS}")
else(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${SAM_INCLUDE_DIRS}\"")
endif(NOT APPLE)

if(BUILD_TESTS)
  if(NOT CATCH_FOUND)
    message(STATUS "Catch will be downloaded when ${CMAKE_PROJECT_NAME} is built")
    ExternalProject_Add(catch
      PREFIX ${MYEXT_PATH}/Catch
      #--Download step--------------
      URL https://github.com/kartikkumar/Catch/archive/master.zip
      TIMEOUT 120
      #--Update/Patch step----------
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      #--Configure step-------------
      CONFIGURE_COMMAND ""
      #--Build step-----------------
      BUILD_COMMAND ""
      #--Install step---------------
      INSTALL_COMMAND ""
      #--Output logging-------------
      LOG_DOWNLOAD ON
    )
    ExternalProject_Get_Property(catch source_dir)
    set(CATCH_INCLUDE_DIRS ${source_dir}/include CACHE INTERNAL "Path to include folder for Catch")
  endif(NOT CATCH_FOUND)

  if(NOT APPLE)
    include_directories(SYSTEM AFTER "${CATCH_INCLUDE_DIRS}")
  else(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${CATCH_INCLUDE_DIRS}\"")
  endif(NOT APPLE)

  if(BUILD_TESTS_WITH_EIGEN)
    if(NOT FORCE_DEPENDENCIES_BUILD)
      find_package(Eigen3)
    endif(NOT FORCE_DEPENDENCIES_BUILD)

    if(NOT EIGEN3_FOUND)
      message(STATUS "Eigen will be downloaded when ${CMAKE_PROJECT_NAME} is built")
      ExternalProject_Add(eigen-lib
        PREFIX ${MYEXT_PATH}/Eigen
        #--Download step--------------
        URL http://bitbucket.org/eigen/eigen/get/3.2.2.tar.gz
        TIMEOUT 120
        #--Update/Patch step----------
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        #--Configure step-------------
        CONFIGURE_COMMAND ""
        #--Build step-----------------
        BUILD_COMMAND ""
        #--Install step---------------
        INSTALL_COMMAND ""
        #--Output logging-------------
        LOG_DOWNLOAD ON
      )
      ExternalProject_Get_Property(eigen-lib source_dir)
      set(EIGEN3_INCLUDE_DIR ${source_dir} CACHE INTERNAL "Path to include folder for Eigen")
    endif(NOT EIGEN3_FOUND)

    if(NOT APPLE)
      include_directories(SYSTEM AFTER "${EIGEN3_INCLUDE_DIR}")
    else(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${EIGEN3_INCLUDE_DIR}\"")
    endif(NOT APPLE)
  endif(BUILD_TESTS_WITH_EIGEN)
endif(BUILD_TESTS)
