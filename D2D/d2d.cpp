/*    
 * Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
 * All rights reserved.
 */

// #define INTEGER int
// #define REAL double

// #include <cstdlib>
// #include <iostream>
// #include <string>
// #include <utility>
// #include <vector>

// #include <Eigen/Core>

// #include <libsgp4/Eci.h>
// #include <libsgp4/DateTime.h> 
// #include <libsgp4/SGP4.h>
// #include <libsgp4/Tle.h>

// #include <Atom/atom.hpp>

// typedef std::vector< REAL > D2D_Vector;
// typedef std::pair< D2D_Vector, D2D_Vector > D2D_Velocities;

//! Execute D2D application.
int main( const int numberOfInputs, const char* inputArguments[ ] )
{
    //////////////////////////////////////////////////////////////////////////////////////////////

    // // Input deck

    // // Set up TLE strings.
    // std::string tleDepartureObjectName = "0 VANGUARD 1";
    // std::string tleDepartureObjectLine1 
    //     = "1 00005U 58002B   14286.73583013  .00000101  00000-0  14924-3 0  2753";
    // std::string tleDepartureObjectLine2 
    //     = "2 00005 034.2416 135.1185 1846262 022.4344 344.7441 10.84392947980145";

    // std::string tleArrivalObjectName = "0 VANGUARD 2";
    // std::string tleArrivalObjectLine1 
    //     = "1 00011U 59001A   14287.11582107  .00000370  00000-0  20422-3 0  2912";
    // std::string tleArrivalObjectLine2
    //     = "2 00011 032.8671 275.5194 1472436 073.1262 302.5194 11.84637428370639";

    // // Set up TLE objects.
    // Tle tleDepartureObject( 
    //     tleDepartureObjectName, tleDepartureObjectLine1, tleDepartureObjectLine2 );
    // Tle tleArrivalObject( 
    //     tleArrivalObjectName, tleArrivalObjectLine1, tleArrivalObjectLine2 );

    // // Set up Time-of-Flight (ToF).
    // const double timeOfFlight = 1000.0;

    // // Set up departure and arrival positions and epochs.
    // SGP4 sgp4Departure( tleDepartureObject );
    // SGP4 sgp4Arrival( tleArrivalObject );

    // DateTime departureEpoch = tleDepartureObject.Epoch( );
    // DateTime arrivalEpoch = tleArrivalObject.Epoch( );
    // arrivalEpoch.AddSeconds( timeOfFlight );

    // Eci departureState = sgp4Departure.FindPosition( 0.0 );
    // Eci arrivalState = sgp4Arrival.FindPosition( arrivalEpoch );

    // D2D_Vector departurePosition( 3 );
    // departurePosition[ 0 ] = departureState.Position( ).x;
    // departurePosition[ 1 ] = departureState.Position( ).y;
    // departurePosition[ 2 ] = departureState.Position( ).z;

    // D2D_Vector arrivalPosition( 3 );
    // arrivalPosition[ 0 ] = arrivalState.Position( ).x;
    // arrivalPosition[ 1 ] = arrivalState.Position( ).y;
    // arrivalPosition[ 2 ] = arrivalState.Position( ).z;

    // D2D_Vector departureVelocity( 3 );
    // departureVelocity[ 0 ] = departureState.Velocity( ).x;
    // departureVelocity[ 1 ] = departureState.Velocity( ).y;
    // departureVelocity[ 2 ] = departureState.Velocity( ).z;

    // D2D_Vector arrivalVelocity( 3 );
    // arrivalVelocity[ 0 ] = arrivalState.Velocity( ).x;
    // arrivalVelocity[ 1 ] = arrivalState.Velocity( ).y;
    // arrivalVelocity[ 2 ] = arrivalState.Velocity( ).z;

    // // Execute Atom solver.
    // std::string solverStatus = "";
    // int numberOfIterations = 0;

    // D2D_Vector departureVelocityGuess( 3 );
    // departureVelocityGuess[ 0 ] = departureVelocity[ 0 ] + 0.01;
    // departureVelocityGuess[ 1 ] = departureVelocity[ 1 ] - 0.02;
    // departureVelocityGuess[ 2 ] = departureVelocity[ 2 ] - 0.03;

    // const D2D_Velocities velocities = atom::executeAtomSolver( 
    //     departurePosition, 
    //     departureEpoch, 
    //     arrivalPosition, 
    //     timeOfFlight, 
    //     departureVelocityGuess,
    //     solverStatus,
    //     numberOfIterations ); 

    // std::cout << solverStatus << std::endl;

    //////////////////////////////////////////////////////////////////////////////////////////////

    // return EXIT_SUCCESS;

    //////////////////////////////////////////////////////////////////////////////////////////////
}
