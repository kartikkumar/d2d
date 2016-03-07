Debris-2-Debris
===================

\cond [![MIT license](http://img.shields.io/badge/license-MIT-brightgreen.svg)](http://opensource.org/licenses/MIT) [![Build Status](https://travis-ci.org/astropnp/d2d.svg?branch=master)](https://travis-ci.org/astropnp/d2d)[![Coverity Scan Build Status](https://scan.coverity.com/projects/3700/badge.svg)](https://scan.coverity.com/projects/3700) [![Coverage Status](https://coveralls.io/repos/astropnp/d2d/badge.png)](https://coveralls.io/r/astropnp/d2d) \endcond

D2D (Debris-2-Debris) is a toolbox that can be used to rapidly search through a catalog of space debris objects to find candidate optimal debris-to-debris transfers. These candidate pairs can be fed into a full-scale optimization problem in the context of mission analysis for an Active Debris Removal (ADR) space mission.

For an example use of this toolbox, take a look at [Kumar, et al. (2016)](#temp).

Features
------

  - Search-space pruning tool to reduce candidates based on static constraints (e.g., minimum and maximum semi-major axis, eccentricity, inclination)
  - Lambert targeter to assess high-thrust transfer cost (DeltaV) for comparison
  - Fully-parameterized characterization of transfer cost (DeltaV) using [Atom](https://github.com/astropnp/atom) solver
  - Structured I/O using JSON (input) and CSV or SQLite (output)

Requirements
------

To install this project, please ensure that you have installed the following (install guides are provided on the respective websites):

  - [Git](http://git-scm.com)
  - A C++ compiler, e.g., [GCC](https://gcc.gnu.org/), [clang](http://clang.llvm.org/), [MinGW](http://www.mingw.org/)
  - [CMake](http://www.cmake.org)
  - [Boost](http://www.boost.org/) (collection of C++ libraries)
  - [Doxygen](http://www.doxygen.org "Doxygen homepage") (optional)
  - [Gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) (optional)
  - [LCOV](http://ltp.sourceforge.net/coverage/lcov.php) (optional)

The [Boost](http://www.boost.org/) libraries cannot be automatically downloaded and installed, as is possible for the libraries listed below. It is recommended that pre-build binaries are installed using e.g., [Homebrew](http://brewformulas.org/Boost) on Mac OS X or [apt-get](https://launchpad.net/ubuntu/+source/boost) on Ubuntu.

 In addition, D2D depends on the following libraries:

  - [SML](https://www.github.com/astropnp/sml) (math library)
  - [Astro](https://www.github.com/astropnp/astro) (astrodynamics library)
  - [Atom](https://www.github.com/astropnp/sam) (modified-Lambert solver using SGP4/SDP4)
  - [GSL](http://www.gnu.org/software/gsl) (GNU scientific library that includes non-linear root-finders used)
  - [SGP4](https://www.github.com/astropnp/sgp4deorbit) (SGP4/SDP4 library)
  - [PyKep](https://www.github.com/esa/pykep) (astrodynamics library including [Izzo Lambert targeter](http://arxiv.org/abs/1403.2705); depends on [Boost](http://www.boost.org/))
  - [RapidJSON](https://github.com/miloyip/rapidjson) (JSON library)
  - [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) (C++ interface to C library for SQLite)
  - [CATCH](https://www.github.com/philsquared/Catch) (unit testing library necessary for `BUILD_TESTS` option)
  - [Eigen](http://eigen.tuxfamily.org/) (linear algebra library necessary for `BUILD_TESTS_WITH_EIGEN` option)

These dependencies will be downloaded and configured automagically if not already present locally (requires an internet connection). It takes a while to install [GSL](http://www.gnu.org/software/gsl) automagically, so it is recommended to pre-install if possible using e.g., [Homebrew](http://brewformulas.org/Gsl) on Mac OS X, [apt-get](http://askubuntu.com/questions/490465/install-gnu-scientific-library-gsl-on-ubuntu-14-04-via-terminal) on Ubuntu, [Gsl](http://gnuwin32.sourceforge.net/packages/gsl.htm)) on Windows.

------

Run the following commands to download, build, and install this project.

    git clone https://www.github.com/astropnp/d2d
    cd d2d
    git submodule init && git submodule update
    mkdir build && cd build
    cmake .. && cmake --build .

To install the header files, libraries and executables, run the following from within the `build` directory:

    make install

Note that dependencies are installed by fetching them online, in case they cannot be detected on your local system. If the build process fails, check the error log given. Typically, building fails due to timeout. Simply run the `cmake --build .` command once more.

Build options
-------------

You can pass the following, general command-line options when running CMake:

  - `-DBUILD_MAIN[=ON (default)|OFF]`: build the main-function
  - `-DCMAKE_INSTALL_PREFIX[=$install_dir]`: set path prefix for install script (`make install`); if not set, defaults to usual locations
  - `-DBUILD_DOXYGEN_DOCS[=ON|OFF (default)]`: build the [Doxygen](http://www.doxygen.org "Doxygen homepage") documentation ([LaTeX](http://www.latex-project.org/) must be installed with `amsmath` package)
  - `-DBUILD_SHARED_LIBS[=ON|OFF (default)]`: build shared libraries instead of static
  - `-DBUILD_TESTS[=ON|OFF (default)]`: build tests (execute tests from build-directory using `ctest -V`)
  - `-DBUILD_DEPENDENCIES[=ON|OFF (default)]`: force local build of dependencies, instead of first searching system-wide using `find_package()`

The following command is conditional and can only be set if `BUILD_TESTS = ON`:

  - `-DBUILD_COVERAGE_ANALYSIS[=ON|OFF (default)]`: build code coverage using [Gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) and [LCOV](http://ltp.sourceforge.net/coverage/lcov.php) (both must be installed; requires [GCC](https://gcc.gnu.org/) compiler; execute coverage analysis from build-directory using `make coverage`)

Project structure
-------------

This project has been set up with a specific file/folder structure in mind. The following describes some important features of this setup:

  - `cmake/Modules` : Contains `CMake` modules
  - `docs`: Contains project-specific docs in [Markdown](https://help.github.com/articles/github-flavored-markdown/ "GitHub Flavored Markdown") that are also parsed by [Doxygen](http://www.doxygen.org "Doxygen homepage"). This sub-directory includes `global_todo.md`, which contains a global list of TODO items for project that appear on TODO list generated in [Doxygen](http://www.doxygen.org "Doxygen homepage") documentation
  - `doxydocs`: HTML output generated by building [Doxygen](http://www.doxygen.org "Doxygen homepage") documentation
  - `include/D2D`: Project header files (*.hpp)
  - `scripts`: Shell scripts used in [Travis CI](https://travis-ci.org/ "Travis CI homepage") build
  - `test`: Project test source files (*.cpp), including `testD2D.cpp`, which contains include for [Catch](https://www.github.com/philsquared/Catch "Catch Github repository")
  - `.travis.yml`: Configuration file for [Travis CI](https://travis-ci.org/ "Travis CI homepage") build, including static analysis using [Coverity Scan](https://scan.coverity.com/ "Coverity Scan homepage") and code coverage using [Coveralls](https://coveralls.io "Coveralls.io homepage")
  - `CMakeLists.txt`: main `CMakelists.txt` file for project (should not need to be modified for basic build)
  - `Dependencies.cmake`: list of dependencies and automated build, triggered if dependency cannot be found locally
  - `Doxyfile.in`: [Doxygen](http://www.doxygen.org "Doxygen homepage") configuration file, adapted for generic use within project build (should not need to be modified)
  - `LICENSE.md`: license file for project
  - `ProjectFiles.cmake`: list of project source files to build
  - `README.md`: project readme file, parsed as main page for [Doxygen](http://www.doxygen.org "Doxygen homepage") documentation

Contributing
------------

Once you've made your great commits:

1. [Fork](https://github.com/astropnp/d2d/fork) d2d
2. Create a topic branch - `git checkout -b my_branch`
3. Push to your branch - `git push origin my_branch`
4. Create a [Pull Request](http://help.github.com/pull-requests/) from your branch
5. That's it!

Disclaimer
------

The copyright holders are not liable for any damage(s) incurred due to improper use of D2D.
