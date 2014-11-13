Debris-2-Debris
===================

[![MIT license](http://img.shields.io/badge/license-MIT-brightgreen.svg)](http://opensource.org/licenses/MIT) [![Build Status](https://travis-ci.org/kartikkumar/d2d.svg?branch=master)](https://travis-ci.org/kartikkumar/d2d)

`Debris-2-Debris` (`D2D`) is a toolbox that can be used to rapidly search through a catalog of space debris objects to find candidate optimal debris-to-debris transfers. These candidate pairs can be fed into a full-scale optimization problem in the context of mission analysis for an Active Debris Removal (ADR) space mission.

For an example use of this toolbox, take a look at [Kumar, et al. (2014)](#temp). 

Features
------

  - Search-space pruning tool to reduce candidates based on static constraints (e.g., minimum and maximum inclination)
  - Lambert targeter to assess high-thrust transfer cost (DeltaV) for comparison
  - Fully-parameterized characterization of transfer cost (DeltaV) using [Atom](https://github.com/kartikkumar/atom) solver
  - Structure I/O using JSON (input) and SQLite (output)

Requirements
------

To install this project, please ensure that you have installed the following (install guides are provided on the respective websites):

  - [Git](http://git-scm.com)
  - A C++ compiler, e.g., [GCC](https://gcc.gnu.org/), [clang](http://clang.llvm.org/), [MinGW](http://www.mingw.org/)
  - [CMake](http://www.cmake.org)
  - [Doxygen](http://www.doxygen.org "Doxygen homepage") (optional)

 In addition, `D2D` depends on the following libraries:

  - [SML](https://www.github.com/kartikkumar/sml)
  - [SAM](https://www.github.com/kartikkumar/sam)
  - [Atom](https://www.github.com/kartikkumar/sam)  
  - [GSL](http://www.gnu.org/software/gsl)
  - [SGP4](https://www.github.com/kartikkumar/sgp4deorbit)
  - [PyKep](https://www.github.com/esa/pykep)        
  - [CATCH](https://www.github.com/philsquared/Catch)
  - [Eigen](http://eigen.tuxfamily.org/) (optional)

These dependencies will be downloaded and configured automatically if they are not already present locally (requires an internet connection).

Installation
------

Run the following commands to download, build, and install this `D2D`.

    git clone https://www.github.com/kartikkumar/d2d
    cd d2d
    git submodule init && git submodule update
    mkdir build && cd build
    cmake ..
    cmake --build .

To install the executable, run the following from within the `build` directory:

    make install

Note that dependencies are installed by fetching them online, in case they cannot be detected on your local system. If the build process fails, check the error log given. Typically, building fails due to timeout. Simply run the `cmake --build .` command once more.

Build options
-------------

You can pass the follow command-line options when running `CMake`:

  - `-DBUILD_MAIN=[on|off (default)]`: build the main-function
  - `-DBUILD_DOCS=[on|off (default)]`: build the [Doxygen](http://www.doxygen.org "Doxygen homepage") documentation ([LaTeX](http://www.latex-project.org/) must be installed with `amsmath` package)
  - `-DBUILD_TESTS`=[on|off (default)]: build tests (execute tests from build-directory using `make test`)
  - `-DBUILD_SHARED_LIBS=[on|off (default)]`: build shared libraries instead of static
  - `-DCMAKE_INSTALL_PREFIX`: set path prefix for install script (`make install`); if not set, defaults to usual locations
  - `-DBUILD_DEPENDENCIES=[on|off (default)]`: force local build of dependencies, instead of first searching system-wide using `find_package()`
  - `-DMYLIB_PATH[=build_dir/lib (default]`: set library path
  - `-DMYBIN_PATH[=build_dir/bin (default]`: set binary path
  - `-DMYTEST_PATH[=build_dir/tests (default]`: set tests path

Contributing
------------

Once you've made your great commits:

1. [Fork](https://github.com/kartikkumar/d2d/fork) `D2D`
2. Create a topic branch - `git checkout -b my_branch`
3. Push to your branch - `git push origin my_branch`
4. Create a [Pull Request](http://help.github.com/pull-requests/) from your branch
5. That's it!

License
------

See `LICENSE.md`.

Disclaimer
------

The copyright holders are not liable for any damage(s) incurred due to improper use of `D2D`.

Contact
------

Shoot an [email](mailto:me@kartikkumar.com?subject=D2D) if you have any questions.

TODO
------

  - Find a way to provide an option to clean installation. 
