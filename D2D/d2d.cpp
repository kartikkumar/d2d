 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/timer/timer.hpp>

#include <Eigen/Core> 

#include <libsgp4/Eci.h>
#include <libsgp4/DateTime.h>
// #include <libsgp4/Globals.h>
// #include <libsgp4/OrbitalElements.h>  
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>

#include <Tudat/Astrodynamics/MissionSegments/lambertTargeterIzzo.h>

// #include <D2D/convertCartesianToTwoLineElements.h>
// #include <D2D/printFunctions.h>

//! Execute D2D test app.
int main( const int numberOfInputs, const char* inputArguments[ ] )
{
    ///////////////////////////////////////////////////////////////////////////

    // Start timer. Timer automatically ends when this object goes out of scope.
    boost::timer::auto_cpu_timer timer;

    ///////////////////////////////////////////////////////////////////////////
     
    ///////////////////////////////////////////////////////////////////////////

    // // Declare using-statements.
    // using std::cerr;
    using std::cout;
    using std::endl;
    using std::string;

    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace tudat::mission_segments;

    // using namespace d2d;

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Input deck.

    // Set Earth gravitational parameter [kg m^3 s^-2].
    const double earthGravitationalParameter = 398600.8 * 1.0e9;
    
    // Set initial epoch.
    const DateTime initialEpoch( 1979, 4, 15 );

    // Set time-of-flight [s].
    const double timeOfFlight = 1325.0;

    // Set TLE strings for 1st debris object.
    const string nameObject1 = "0 SCOUT D-1 R/B";
    const string line1Object1 
        = "1 06800U 72091  B 79104.50192675  .00554509 +00000-0 +00000-0 0 03968";
    const string line2Object1 
        = "2 06800 001.9018 086.9961 0020515 168.0311 195.7657 15.89181450264090";

    // Set TLE strings for 2nd debris object.
    const string nameObject2 = "0 SCOUT D-1 R/B";
    const string line1Object2 
        = "1 06800U 72091  B 79098.71793498  .00418782 +00000-0 +00000-0 0 03921";
    const string line2Object2 
        = "2 06800 001.9047 136.0594 0024458 068.9289 290.1247 15.83301112263179";

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Compute derived parameters.

    // Compute final epoch.
    const DateTime finalEpoch( initialEpoch );
    finalEpoch.AddSeconds( timeOfFlight );    

    // Create TLE objects from strings.
    Tle tleObject1( nameObject1, line1Object1, line2Object1 );
    Tle tleObject2( nameObject2, line1Object2, line2Object2 );

    // Set up SGP4 propagator objects.
    SGP4 sgp4Object1( tleObject1 );
    SGP4 sgp4Object2( tleObject2 );

    // Compute Cartesian states of objects at initial and final epochs.
    Eci initialState = sgp4Object1.FindPosition( initialEpoch );
    Eci finalState = sgp4Object2.FindPosition( finalEpoch );

    // Compute initial position [m] and velocity [ms^-1].
    const Eigen::Vector3d initialPosition( 
        convertKilometersToMeters( initialState.Position( ).x ), 
        convertKilometersToMeters( initialState.Position( ).y ),
        convertKilometersToMeters( initialState.Position( ).z ) );

    const Eigen::Vector3d initialVelocity( 
        convertKilometersToMeters( initialState.Velocity( ).x ), 
        convertKilometersToMeters( initialState.Velocity( ).y ),
        convertKilometersToMeters( initialState.Velocity( ).z ) );

    // // Compute final position [m] and velocity [ms^-1].
    const Eigen::Vector3d finalPosition( 
        convertKilometersToMeters( finalState.Position( ).x ), 
        convertKilometersToMeters( finalState.Position( ).y ),
        convertKilometersToMeters( finalState.Position( ).z ) );

    const Eigen::Vector3d finalVelocity( 
        convertKilometersToMeters( finalState.Velocity( ).x ), 
        convertKilometersToMeters( finalState.Velocity( ).y ),
        convertKilometersToMeters( finalState.Velocity( ).z ) );

    // Set up Lambert targeter. This automatically triggers the solver to execute.
    LambertTargeterIzzo lambertTargeter( 
        initialPosition, finalPosition, timeOfFlight, earthGravitationalParameter );

    // Compute Delta V at initial position [ms^-1].
    cout << "Delta V_i: " 
         << std::fabs( 
                lambertTargeter.getInertialVelocityAtDeparture( ).norm( ) 
                - initialVelocity.norm( ) )
         << " m/s" << endl;

    // Set guess for initial Cartesian state after departure.
    const Eigen::VectorXd departureState 
        = ( Eigen::VectorXd( 6 ) 
                << initialPosition,
                   // lambertTargeter.getInertialVelocityAtDeparture( ) ).finished( );
                   initialVelocity ).finished( );

    // Convert post-maneuver state in Cartesian elements to Keplerian elements.
    Eigen::VectorXd departureStateInKeplerianElements
        = convertCartesianToKeplerianElements( departureState, earthGravitationalParameter );  

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Compute modified TLE.

    // // Set up parameters for non-linear function.
    // CartesianToTwoLineElementsParameters parameters 
    //     = { 
    //         earthGravitationalParameter,
    //         tleObject1,
    //         departureStateInitialGuess,
    //         numberOfUnknowns
    //       };

    // // Set up non-linear function.
    // gsl_multiroot_function cartesianToTwoLineElementsFunction
    //     = {
    //          &convertCartesianToTwoLineElements, 
    //          numberOfUnknowns, 
    //          &parameters
    //       };

    // // Set initial guess.
    // gsl_vector* initialGuess = gsl_vector_alloc( numberOfUnknowns );
    // for ( unsigned int i = 0; i < numberOfUnknowns; i++ )
    // {
    //     gsl_vector_set( initialGuess, i, departureStateInKeplerianElements[ i ]);      
    // }

    // // Set up non-linear solver type (derivative free).
    // const gsl_multiroot_fsolver_type* solverType = gsl_multiroot_fsolver_hybrids;

    // // Allocate memory for solver.
    // gsl_multiroot_fsolver* solver = gsl_multiroot_fsolver_alloc( solverType, numberOfUnknowns );

    // // Set solver to use non-linear function with initial guess vector.
    // gsl_multiroot_fsolver_set( solver, &cartesianToTwoLineElementsFunction, initialGuess );

    // // Declare current solver status and iteration counter.
    // int solverStatus;
    // size_t iteration = 0;

    // // Print current state of solver.
    // printSolverStateTableHeader( );
    // printSolverState( iteration, solver );

    // if ( GSL_EBADFUNC == gsl_multiroot_fsolver_iterate( solver ) )
    //     std::cout << "Oops!" << std::endl;
    // // do
    // // {
    // //     iteration++;
    // //     solverStatus = gsl_multiroot_fsolver_iterate( solver );

    // //     printSolverState( iteration, solver );

    // //     // Check if solver is stuck; if it is stuck, break from loop.
    // //     if ( solverStatus )   
    // //     {
    // //         cout << "ERROR: Non-linear solver is stuck!" << endl;
    // //         break;
    // //     }

    // //     solverStatus = gsl_multiroot_test_residual( solver->f, solverTolerance );
    // // }
    // // while ( solverStatus == GSL_CONTINUE && iteration < 1 );

    // // // Print final status of non-linear solver.
    // // cout << endl;
    // // cout << "Status of non-linear solver: " << gsl_strerror( solverStatus ) << endl;
    // // cout << endl;

    // // Free up memory.
    // gsl_multiroot_fsolver_free( solver );
    // gsl_vector_free( initialGuess );

    // // ///////////////////////////////////////////////////////////////////////////

    // // // Set up non-linear solver to compute modified TLE.

    // // // Set number of unknowns (6 state variables).
    // // const int numberOfUnknowns = 6;

    // // // Set up parameters for non-linear function.
    // // CartesianToTwoLineElementsParameters parameters 
    // //     = { 
    // //         earthGravitationalParameter,
    // //         tleObject1,
    // //         departureStateInitialGuess,
    // //         numberOfUnknowns
    // //       };

    // // // Set up non-linear function.
    // // gsl_multiroot_function cartesianToTwoLineElementsFunction
    // //     = {
    // //          &convertCartesianToTwoLineElements, 
    // //          numberOfUnknowns, 
    // //          &parameters
    // //       };

    // // // Set initial guess.
    // // gsl_vector* initialGuess = gsl_vector_alloc( numberOfUnknowns );
    // // for ( unsigned int i = 0; i < numberOfUnknowns; i++ )
    // // {
    // //     gsl_vector_set( initialGuess, i, departureStateInKeplerianElements[ i ]);      
    // // }

    // // // Set up non-linear solver type (derivative free).
    // // const gsl_multiroot_fsolver_type* solverType = gsl_multiroot_fsolver_hybrids;

    // // // Allocate memory for solver.
    // // gsl_multiroot_fsolver* solver = gsl_multiroot_fsolver_alloc( solverType, numberOfUnknowns );

    // // // Set solver to use non-linear function with initial guess vector.
    // // gsl_multiroot_fsolver_set( solver, &cartesianToTwoLineElementsFunction, initialGuess );

    // // // Declare current solver status and iteration counter.
    // // int solverStatus;
    // // size_t iteration = 0;

    // // // Print current state of solver.
    // // printSolverStateTableHeader( );
    // // printSolverState( iteration, solver );

    // // do
    // // {
    // //     iteration++;
    // //     solverStatus = gsl_multiroot_fsolver_iterate( solver );

    // //     printSolverState( iteration, solver );

    // //     // Check if solver is stuck; if it is stuck, break from loop.
    // //     if ( solverStatus )   
    // //     {
    // //         cout << "ERROR: Non-linear solver is stuck!" << endl;
    // //         break;
    // //     }

    // //     solverStatus = gsl_multiroot_test_residual( solver->f, solverTolerance );
    // // }
    // // while ( solverStatus == GSL_CONTINUE && iteration < 1 );

    // // // Print final status of non-linear solver.
    // // cout << endl;
    // // cout << "Status of non-linear solver: " << gsl_strerror( solverStatus ) << endl;
    // // cout << endl;

    // // // Free up memory.
    // // gsl_multiroot_fsolver_free( solver );
    // // gsl_vector_free( initialGuess );

    /////////////////////////////////////////////////////////////////////////
                
    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 
}