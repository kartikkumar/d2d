 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/timer/timer.hpp>

#include <Eigen/Core>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>  
#include <libsgp4/Globals.h>      
#include <libsgp4/SGP4.h>  
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  

#include "D2D/cartesianToTwoLineElements.h"

//! Execute D2D application.
int main( const int numberOfInputs, const char* inputArguments[ ] )
{
    //////////////////////////////////////////////////////////////////////////////////////////////

    // Declare using-statements.
    using std::cout;
    using std::endl;
    using std::string;

    using boost::timer::auto_cpu_timer;

    using namespace tudat::basic_astrodynamics::unit_conversions;

    using namespace d2d;

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Start timer. Timer automatically ends when this object goes out of scope.
    auto_cpu_timer timer;

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Input deck.

    // Target epoch.
    DateTime targetEpoch( 2014, 5, 29 );
    
    // Set TLE strings for 1st debris object.
    const string nameObject1 = "0 VANGUARD 1";
    const string line1Object1 
        = "1 00005U 58002B   14143.59770889 -.00000036  00000-0 -22819-4 0  2222";
    const string line2Object1 
        = "2 00005 034.2431 215.7533 1849786 098.8024 282.5368 10.84391811964621";

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Set Earth gravitational parameter [m^3 s^-2].
    const double earthGravitationalParameter = kMU * 1.0e9;

    // Set up TLE object.
    const Tle tleObject1( nameObject1, line1Object1, line2Object1 );

    // Set up SGP4 propagator for TLE object.
    const SGP4 sgp4( tleObject1 );

    // Propagate TLE to epoch of TLE.
    targetEpoch = tleObject1.Epoch( );
    Eci propagatedState = sgp4.FindPosition( targetEpoch );

    // Store propagated state as target Cartesian state.
    const Eigen::VectorXd targetState 
        = ( Eigen::VectorXd( 6 ) 
                << convertKilometersToMeters( propagatedState.Position( ).x ),
                   convertKilometersToMeters( propagatedState.Position( ).y ),
                   convertKilometersToMeters( propagatedState.Position( ).z ),
                   convertKilometersToMeters( propagatedState.Velocity( ).x ) + 100.0,
                   convertKilometersToMeters( propagatedState.Velocity( ).y ) - 25.0,
                   convertKilometersToMeters( propagatedState.Velocity( ).z ) + 125.0 ).finished( );

    // Convert target Cartesian state to a new TLE object.
    convertCartesianStateToTwoLineElements( 
        targetState, targetEpoch, tleObject1, earthGravitationalParameter );

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////
                
    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 

    ////////////////////////////////////////////////////////////////////////////////////////////// 
}
