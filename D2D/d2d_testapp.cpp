 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <cstdlib>
#include <iostream>

#include <boost/timer/timer.hpp>

#include <Eigen/Core>  

#include <libsgp4/Globals.h>
  
#include <sgp4/cpp/sgp4unit.h>

#include <Tudat/Astrodynamics/Gravitation/centralGravityModel.h>
#include <Tudat/InputOutput/fieldType.h>
#include <Tudat/InputOutput/parser.h>


//! Execute D2D test app.
int main( const int numberOfInputs, const char* inputArguments[ ] )
{
    ///////////////////////////////////////////////////////////////////////////

    // Start timer. Timer automatically ends when this object goes out of scope.
    boost::timer::auto_cpu_timer timer;

    ///////////////////////////////////////////////////////////////////////////
     
    ///////////////////////////////////////////////////////////////////////////

    // Declare using-statements.
    using std::cerr;
    using std::cout;
    using std::endl;

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Testing area

    // sgp4( wgs84,  );

    cout << kXKMPER << endl;

    ///////////////////////////////////////////////////////////////////////////
                
    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 
}