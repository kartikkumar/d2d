 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

#include <boost/timer/timer.hpp>

#include <Eigen/Core>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>  
#include <libsgp4/Globals.h>      
#include <libsgp4/SGP4.h>  
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  

#include <Tudat/Astrodynamics/MissionSegments/lambertTargeterIzzo.h>
  
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
    using namespace tudat::mission_segments;

    using namespace d2d;

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Start timer. Timer automatically ends when this object goes out of scope.
    auto_cpu_timer timer;

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Input deck.

    // Departure epoch.
    DateTime departureEpoch( 2014, 5, 23, 15, 0, 0 );

    // Time of flight [s].
    const double timeOfFlight = 1200.0;

    // Set TLE strings for departure debris object.
    // const string nameDepartureObject = "0 VANGUARD 1";
    // const string line1DepartureObject
    //     = "1 00005U 58002B   14155.10921814  .00000060  00000-0  78635-4 0  2264";
    // const string line2DepartureObject
    //     = "2 00005 034.2432 180.3034 1849607 150.5906 221.2890 10.84391253965858";

    const string nameDepartureObject = "TEST";
    const string line1DepartureObject
        = "1 00341U 62029B   12295.78875316 -.00000035  00000-0  68936-4 0  4542";
    const string line2DepartureObject
        = "2 00341 044.7940 159.4482 2422241 038.6572 336.4786 09.14200419679435";

    // Set TLE strings for arrival debris object.
    const string nameArrivalObject = "0 EXPLORER 7";
    const string line1ArrivalObject 
        = "1 00022U 59009A   14155.52295729  .00002637  00000-0  24497-3 0  9272";
    const string line2ArrivalObject 
        = "2 00022 050.2855 342.3526 0155059 135.2836 226.0732 14.89200936895768";

    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Compute derived parameters.

    // Set Earth gravitational parameter [m^3 s^-2].
    const double earthGravitationalParameter = kMU * 1.0e9;

    // Set arrival epoch as departure epoch + time-of-flight.
    const DateTime arrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );

    // Set up TLE objects.
    const Tle tleDepartureObject( nameDepartureObject, line1DepartureObject, line2DepartureObject );
    const Tle tleArrivalObject( nameArrivalObject, line1ArrivalObject, line2ArrivalObject );

    // Set up SGP4 propagator for TLE objects.
    const SGP4 sgp4DepartureObject( tleDepartureObject );
    const SGP4 sgp4ArrivalObject( tleArrivalObject );

    // Propagate TLE objects to departure and arrival epochs.
    Eci sgp4DepartureState = sgp4DepartureObject.FindPosition( 0.0 );
    Eci sgp4ArrivalState = sgp4ArrivalObject.FindPosition( arrivalEpoch );

    // Store propagated states as target departure and arrival Cartesian states.
    const Eigen::VectorXd departureState 
        = ( Eigen::VectorXd( 6 ) 
                << convertKilometersToMeters( sgp4DepartureState.Position( ).x ),
                   convertKilometersToMeters( sgp4DepartureState.Position( ).y ),
                   convertKilometersToMeters( sgp4DepartureState.Position( ).z ),
                   convertKilometersToMeters( sgp4DepartureState.Velocity( ).x ),
                   convertKilometersToMeters( sgp4DepartureState.Velocity( ).y ),
                   convertKilometersToMeters( sgp4DepartureState.Velocity( ).z ) ).finished( );

    const Eigen::VectorXd arrivalState 
        = ( Eigen::VectorXd( 6 ) 
                << convertKilometersToMeters( sgp4ArrivalState.Position( ).x ),
                   convertKilometersToMeters( sgp4ArrivalState.Position( ).y ),
                   convertKilometersToMeters( sgp4ArrivalState.Position( ).z ),
                   convertKilometersToMeters( sgp4ArrivalState.Velocity( ).x ),
                   convertKilometersToMeters( sgp4ArrivalState.Velocity( ).y ),
                   convertKilometersToMeters( sgp4ArrivalState.Velocity( ).z ) ).finished( );

    // Compute Lambert solution to transfer from departure to arrival state.
    LambertTargeterIzzo lambertTargeter( departureState.segment( 0, 3 ),
                                         arrivalState.segment( 0, 3 ),
                                         timeOfFlight,
                                         earthGravitationalParameter );   

    // Compute Lambert departure Delta-V [m/s].
    const Eigen::Vector3d lambertDepartureDeltaV 
      = lambertTargeter.getInertialVelocityAtDeparture( ) - departureState.segment( 3, 3 );

    // Compute post-maneuver departure state.
    const Eigen::VectorXd postManeuverDepartureState 
      // = departureState  + ( Eigen::VectorXd( 6 ) << Eigen::Vector3d( ), 
      //                                               lambertDepartureDeltaV ).finished( );
      = departureState  + ( Eigen::VectorXd( 6 ) << Eigen::Vector3d( ), 
                                                    Eigen::Vector3d( 100.0, 100.0, 100.0 ) ).finished( );
    // Convert target Cartesian departure state to a new TLE object.
    const Tle newDepartureTle = convertCartesianStateToTwoLineElements( 
        postManeuverDepartureState, tleDepartureObject.Epoch( ), tleDepartureObject, earthGravitationalParameter, 1.0e-8, true );

std::cout << newDepartureTle << std::endl;
    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////

    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 

    ////////////////////////////////////////////////////////////////////////////////////////////// 
}
