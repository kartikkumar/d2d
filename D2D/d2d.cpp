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
#include <vector>

#include <boost/timer/timer.hpp>

#include <Eigen/Core> 

#include <nlopt.hpp>

#include <libsgp4/Eci.h>
#include <libsgp4/DateTime.h>  
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>
#include <TudatCore/Mathematics/BasicMathematics/mathematicalConstants.h>

#include <Tudat/Astrodynamics/MissionSegments/lambertTargeterIzzo.h>

#include <D2D/cartesianToTwoLineElementsObjectiveFunction.h>

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
    using namespace tudat::basic_mathematics::mathematical_constants; 
    using namespace tudat::mission_segments;

    using namespace d2d;

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

    // Set minimization tolerance.
    const double minimizationTolerance = 1.0e-8;

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

    // // Compute Delta V at initial position [ms^-1].
    // cout << "Delta V_i: " 
    //      << std::fabs( 
    //             lambertTargeter.getInertialVelocityAtDeparture( ).norm( ) 
    //             - initialVelocity.norm( ) )
    //      << " m/s" << endl;

    // Set guess for initial Cartesian state after departure.
    const Eigen::VectorXd departureState 
        = ( Eigen::VectorXd( 6 ) 
                << initialPosition,
                   // lambertTargeter.getInertialVelocityAtDeparture( ) ).finished( );
                   initialVelocity ).finished( );

    // Convert state to Keplerian elements.
    const Eigen::VectorXd departureStateInKeplerianElements
        = convertCartesianToKeplerianElements( 
            departureState, earthGravitationalParameter );

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Compute modified TLE.

    // Set up derivative-free optimizer.
    nlopt::opt optimizer( nlopt::LN_COBYLA, 6 );

    // Set up parameters for non-linear function.
    CartesianToTwoLineElementsParameters parameters( 
        earthGravitationalParameter, tleObject1, departureState );

    // Set objective function.
    optimizer.set_min_objective( cartesianToTwoLineElementsObjectiveFunction, &parameters );

    // Set tolerance.
    optimizer.set_xtol_rel( minimizationTolerance );

    // Set lower bounds.
    std::vector< double > lowerBounds( 6, -HUGE_VAL );
    lowerBounds.at( semiMajorAxisIndex ) =  0.0;
    lowerBounds.at( eccentricityIndex ) =  0.0;
    lowerBounds.at( inclinationIndex ) =  0.0;
    lowerBounds.at( argumentOfPeriapsisIndex ) =  0.0;
    lowerBounds.at( longitudeOfAscendingNodeIndex ) =  0.0;
    lowerBounds.at( trueAnomalyIndex ) =  0.0;

    optimizer.set_lower_bounds( lowerBounds );

    // Set upper bounds.
    std::vector< double > upperBounds( 6, HUGE_VAL );
    lowerBounds.at( semiMajorAxisIndex ) =  5.0e7;    
    upperBounds.at( eccentricityIndex ) =  1.0;
    upperBounds.at( inclinationIndex ) =  PI;
    upperBounds.at( argumentOfPeriapsisIndex ) =  2.0 * PI;
    upperBounds.at( longitudeOfAscendingNodeIndex ) = 2.0 * PI;
    upperBounds.at( trueAnomalyIndex ) = 2.0 * PI;

    optimizer.set_upper_bounds( upperBounds );

    // Set initial guess.
    std::vector< double > stateInKeplerianElements( 6 );
    Eigen::Map< Eigen::VectorXd >( 
        stateInKeplerianElements.data( ), 6, 1 ) = departureStateInKeplerianElements;

    // Set initial step size.
    // optimizer.set_initial_step( 0.01 );
    
    // Execute optimizer.
    double minimumFunctionValue;
    nlopt::result result = optimizer.optimize( stateInKeplerianElements, minimumFunctionValue );

    // Print output statements.
    if ( result < 0 ) 
    {
        cout << "NLOPT failed!" << endl;
    }
    else 
    {
        cout << "found minimum = " << minimumFunctionValue << endl;
    }

    // Print number of iterations taken by optimizer.
    cout << endl;
    cout << "# of iterations: " << cartesianToTwoLineElementsOptmizerIterations << endl;
    cout << endl;

    /////////////////////////////////////////////////////////////////////////
  
    /////////////////////////////////////////////////////////////////////////
                
    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 

    /////////////////////////////////////////////////////////////////////////
}