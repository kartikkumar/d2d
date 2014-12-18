Debris-2-Debris
===================

[![MIT license](http://img.shields.io/badge/license-MIT-brightgreen.svg)](http://opensource.org/licenses/MIT) [![Build Status](https://travis-ci.org/kartikkumar/d2d.svg?branch=master)](https://travis-ci.org/kartikkumar/d2d)[![Coverity Scan Build Status](https://scan.coverity.com/projects/3700/badge.svg)](https://scan.coverity.com/projects/3700) [![Coverage Status](https://coveralls.io/repos/kartikkumar/d2d/badge.png)](https://coveralls.io/r/kartikkumar/d2d)

D2D (Debris-2-Debris) is a toolbox that can be used to rapidly search through a catalog of space debris objects to find candidate optimal debris-to-debris transfers. These candidate pairs can be fed into a full-scale optimization problem in the context of mission analysis for an Active Debris Removal (ADR) space mission.

For an example use of this toolbox, take a look at [Kumar, et al. (2015)](#temp).

Features
------

  - Search-space pruning tool to reduce candidates based on static constraints (e.g., minimum and maximum semi-major axis, eccentricity, inclination)
  - Lambert targeter to assess high-thrust transfer cost (DeltaV) for comparison
  - Fully-parameterized characterization of transfer cost (DeltaV) using [Atom](https://github.com/kartikkumar/atom) solver
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

The [Boost](http://www.boost.org/) libraries cannot be automatically downloaded and installed, as is possible for the libraries listed below. This choice has been made because compilation time can be very long. It is recommended that pre-build binaries are installed using e.g., [Homebrew](http://brewformulas.org/Boost) on Mac OS X or [apt-get](https://launchpad.net/ubuntu/+source/boost) on Ubuntu.

 In addition, D2D depends on the following libraries:

  - [SML](https://www.github.com/kartikkumar/sml) (math library)
  - [Astro](https://www.github.com/kartikkumar/astro) (astrodynamics library)
  - [Atom](https://www.github.com/kartikkumar/sam) (modified-Lambert solver using SGP4/SDP4)
  - [GSL](http://www.gnu.org/software/gsl) (GNU scientific library that includes non-linear root-finders used)
  - [SGP4](https://www.github.com/kartikkumar/sgp4deorbit) (SGP4/SDP4 library)
  - [PyKep](https://www.github.com/esa/pykep) (astrodynamics library including [Izzo Lambert targeter](http://arxiv.org/abs/1403.2705); depends on [Boost](http://www.boost.org/))
  - [RapidJSON](https://github.com/miloyip/rapidjson) (JSON library)
  - [SQLite](http://www.sqlite.org/) (C interface for SQLite database engine)
  - [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) (C++ interface to C library for SQLite)
  - [CATCH](https://www.github.com/philsquared/Catch) (unit testing library necessary for `BUILD_TESTS` option)
  - [Eigen](http://eigen.tuxfamily.org/) (linear algebra library necessary for `BUILD_TESTS_WITH_EIGEN` option)

These dependencies will be downloaded and configured automagically if not already present locally (requires an internet connection).

It takes a while to install [GSL](http://www.gnu.org/software/gsl) automagically, so it is recommended to pre-install if possible using e.g., [Homebrew](http://brewformulas.org/Gsl) on Mac OS X, [apt-get](http://askubuntu.com/questions/490465/install-gnu-scientific-library-gsl-on-ubuntu-14-04-via-terminal) on Ubuntu, [Gsl for Windows](http://gnuwin32.sourceforge.net/packages/gsl.htm)).

Additionally, although [SQLite](http://www.sqlite.org/) will build automagically, the Github repository is manually updated. To ensure that you have the latest version with bug fixes, updates etc., it is also recommended to pre-install if possible using e.g., [Homebrew](http://brewformulas.org/Gsl) on Mac OS X, [apt-get](https://launchpad.net/ubuntu/+source/sqlite) on Ubuntu, [SQLite pre-compilted binaries](http://www.sqlite.org/download.html) for all platforms).

------

Run the following commands to download, build, and install this project.

    git clone https://www.github.com/kartikkumar/sml
    cd sml
    git submodule init && git submodule update
    mkdir build && cd build
    cmake .. && cmake --build .

To install the header files, run the following from within the `build` directory:

    make install

Note that dependencies are installed by fetching them online, in case they cannot be detected on your local system. If the build process fails, check the error log given. Typically, building fails due to timeout. Simply run the `cmake --build .` command once more.

Build options
-------------

You can pass the following, general command-line options when running CMake:

  - `-DBUILD_MAIN[=ON (default)|OFF]`: build the main-function
  - `-DCMAKE_INSTALL_PREFIX[=$install_dir]`: set path prefix for install script (`make install`); if not set, defaults to usual locations
  - `-DBUILD_SHARED_LIBS[=ON|OFF (default)]`: build shared libraries instead of static
  - `-DBUILD_DOCS[=ON|OFF (default)]`: build the [Doxygen](http://www.doxygen.org "Doxygen homepage") documentation ([LaTeX](http://www.latex-project.org/) must be installed with `amsmath` package)
  - `-DBUILD_TESTS[=ON|OFF (default)]`: build tests (execute tests from build-directory using `ctest -V`)
  - `-DBUILD_DEPENDENCIES[=ON|OFF (default)]`: force local build of dependencies, instead of first searching system-wide using `find_package()`

The following command is conditional and can only be set if `BUILD_TESTS = ON`:

  - `-DBUILD_COVERAGE_ANALYSIS[=ON|OFF (default)]`: build code coverage using [Gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) and [LCOV](http://ltp.sourceforge.net/coverage/lcov.php) (both must be installed; requires [GCC](https://gcc.gnu.org/) compiler; execute coverage analysis from build-directory using `make coverage`)

Contributing
------------

Once you've made your great commits:

1. [Fork](https://github.com/kartikkumar/d2d/fork) D2D
2. Create a topic branch - `git checkout -b my_branch`
3. Push to your branch - `git push origin my_branch`
4. Create a [Pull Request](http://help.github.com/pull-requests/) from your branch
5. That's it!

Disclaimer
------

The copyright holders are not liable for any damage(s) incurred due to improper use of D2D.

TODO
------

  - Find a way to provide an option to clean installation.
  - Parallelize lambert_scanner and atom_scanner using OpenMP if present
  - Add atom_single mode to compute single transfers
  - Add option to automatically download TLE catalog
  - Add status indicator in console for scanning modes
  - Build GUI interface
  - Add option to compute transfers in 2D (by setting z-component to zero)
