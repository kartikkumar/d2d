 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iomanip>
#include <limits>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <boost/format.hpp>
#include <boost/timer/timer.hpp>

#include <Eigen/Core> 

#include <nlopt.hpp>

#include <libsgp4/Eci.h>
#include <libsgp4/DateTime.h>  
#include <libsgp4/Globals.h>    
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>  
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>
#include <TudatCore/Mathematics/BasicMathematics/basicMathematicsFunctions.h>  
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

    // Declare using-statements.
    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace tudat::basic_astrodynamics::physical_constants; 
    using namespace tudat::basic_mathematics; 
    using namespace tudat::basic_mathematics::mathematical_constants; 
    using namespace tudat::mission_segments;

    using namespace d2d;

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Input deck.
    
    // Set depature epoch.
    const DateTime departureEpoch( 1979, 4, 15 );
    
    // Set time-of-flight [s].
//    const double timeOfFlight = 1325.0;

    // Set TLE strings for 1st debris object.
    const string nameObject1 = "0 SCOUT D-1 R/B";
    const string line1Object1 
        = "1 06800U 72091  B 79104.50192675  .00554509 +00000-0 +00000-0 0 03968";
    const string line2Object1 
        = "2 06800 001.9018 086.9961 0020515 168.0311 195.7657 15.89181450264090";

    // Set TLE strings for 2nd debris object.
//    const string nameObject2 = "0 SCOUT D-1 R/B";
//    const string line1Object2 
//        = "1 06800U 72091  B 79098.71793498  .00418782 +00000-0 +00000-0 0 03921";
//    const string line2Object2 
//        = "2 06800 001.9047 136.0594 0024458 068.9289 290.1247 15.83301112263179";

    // Set minimization tolerance.
   const double minimizationTolerance = 1.0e-8;

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
         
    // Compute derived parameters.

    // Set Earth gravitational parameter [m^3 s^-2].
    const double earthGravitationalParameter = kMU * 1.0e9;

    // Compute arrival epoch.
//    const DateTime arrivalEpoch( departureEpoch );
//    arrivalEpoch.AddSeconds( timeOfFlight );    

    // Create TLE objects from strings.
    const Tle tleObject1( nameObject1, line1Object1, line2Object1 );
//    const Tle tleObject2( nameObject2, line1Object2, line2Object2 );

    // Set up SGP4 propagator objects.
    const SGP4 sgp4Object1( tleObject1 );
//    const SGP4 sgp4Object2( tleObject2 );

    // Compute Cartesian states of objects at departure and arrival epochs.
    // const Eci departureState = sgp4Object1.FindPosition( departureEpoch );
    const Eci departureState = sgp4Object1.FindPosition( 0.0 );
    // const Eci arrivalState = sgp4Object2.FindPosition( arrivalEpoch );

    // Compute departure position [m] and velocity [ms^-1].
    const Eigen::Vector3d depaturePosition( 
        convertKilometersToMeters( departureState.Position( ).x ), 
        convertKilometersToMeters( departureState.Position( ).y ),
        convertKilometersToMeters( departureState.Position( ).z ) );

    const Eigen::Vector3d departureVelocity( 
        convertKilometersToMeters( departureState.Velocity( ).x ), 
        convertKilometersToMeters( departureState.Velocity( ).y ),
        convertKilometersToMeters( departureState.Velocity( ).z ) );

    // Compute arrival position [m] and velocity [ms^-1].
//    const Eigen::Vector3d arrivalPosition( 
//        convertKilometersToMeters( arrivalState.Position( ).x ), 
//        convertKilometersToMeters( arrivalState.Position( ).y ),
//        convertKilometersToMeters( arrivalState.Position( ).z ) );

//    const Eigen::Vector3d arrivalVelocity( 
//        convertKilometersToMeters( arrivalState.Velocity( ).x ), 
//        convertKilometersToMeters( arrivalState.Velocity( ).y ),
//        convertKilometersToMeters( arrivalState.Velocity( ).z ) );

    // Set up Lambert targeter. This automatically triggers the solver to execute.
//    LambertTargeterIzzo lambertTargeter( 
//        depaturePosition, arrivalPosition, timeOfFlight, earthGravitationalParameter );

    // // Compute Delta V at departure position [ms^-1].
    // cout << "Delta V_dep: " 
    //      << std::fabs( 
    //             lambertTargeter.getInertialVelocityAtDeparture( ).norm( ) 
    //             - departureVelocity.norm( ) )
    //      << " m/s" << endl;

    // Set guess for Cartesian state at departure after executing maneuver.
    const Eigen::VectorXd departureStateAfterManeuver 
        = ( Eigen::VectorXd( 6 ) 
                << depaturePosition,
                   // lambertTargeter.getInertialVelocityAtDeparture( ) ).finished( );
                   departureVelocity ).finished( );

    // Compute true anomaly at departure epoch [rad].
    const double departureTrueAnomaly 
        = convertCartesianToKeplerianElements( 
                departureStateAfterManeuver, earthGravitationalParameter )[ trueAnomalyIndex ];

    // Compute eccentric anomaly from true anomaly [rad].
    const double departureEccentricAnomaly 
        = convertTrueAnomalyToEccentricAnomaly( departureTrueAnomaly, tleObject1.Eccentricity( ) );

    // Compute mean anomaly from eccentric anomaly [deg].
    const double departureMeanAnomaly
        = computeModulo( 
            convertRadiansToDegrees( 
                convertEccentricAnomalyToMeanAnomaly( 
                    departureEccentricAnomaly, tleObject1.Eccentricity( ) ) ), 360.0 );

    // Set initial guess for TLE mean elements at departure.
    const Eigen::VectorXd departureTLEMeanElementsGuess
        = ( Eigen::VectorXd( 6 ) << tleObject1.Inclination( true ),
                                    tleObject1.RightAscendingNode( true ),
                                    tleObject1.Eccentricity( ),
                                    tleObject1.ArgumentPerigee( true ),
                                    departureMeanAnomaly,
                                    tleObject1.MeanMotion( ) ).finished( );

    // Set up reference TLE at departure epoch.

    // Update Line 1 for object 1 at departure epoch.
    const string departureEpochString 
        = boost::str( 
            boost::format( "%02i" ) % ( computeModulo( departureEpoch.Year( ), 100.0 ) ) )
            + boost::str( boost::format( "%012.8f" ) 
                % ( departureEpoch.DayOfYear( departureEpoch.Year( ), 
                                              departureEpoch.Month( ), 
                                              departureEpoch.Day( ) ) 
                  + ( departureEpoch.Hour( ) * 3600.0 
                      + departureEpoch.Minute( ) * 60.0 
                      + departureEpoch.Second( ) ) / JULIAN_DAY ) );
    string departureLine1Object1 = line1Object1;
    departureLine1Object1.replace( 18, 14, departureEpochString );

    // Update Line 2 for object 1 at departure epoch.
    const string departureMeanAnomalyString
        = boost::str( boost::format( "%08.4f" ) % departureMeanAnomaly );
    string departureLine2Object1 = line2Object1;
    departureLine2Object1.replace( 43, 8, departureMeanAnomalyString );

    // Create reference TLE for object 1 at departure.
    const Tle departureReferenceTleObject1( 
        nameObject1, departureLine1Object1, departureLine2Object1 );

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Compute TLE for object 1 at departure epoch.

    // Set up derivative-free optimizer.
    nlopt::opt optimizer( nlopt::LN_COBYLA, 6 );

    // Set up parameters for objective function.
    ObjectiveFunctionParameters parameters( 
        earthGravitationalParameter, departureReferenceTleObject1, departureStateAfterManeuver );

    // Set objective function.
    optimizer.set_min_objective( cartesianToTwoLineElementsObjectiveFunction, &parameters );

    // Set tolerance.
    optimizer.set_xtol_rel( minimizationTolerance );

    // Set lower bounds.
    std::vector< double > lowerBounds( 6, -HUGE_VAL );
    // lowerBounds.at( semiMajorAxisIndex ) =  6.0e6;
    lowerBounds.at( eccentricityIndex ) =  0.0;
    // lowerBounds.at( inclinationIndex ) =  0.0;
    // lowerBounds.at( argumentOfPeriapsisIndex ) =  0.0;
    // lowerBounds.at( longitudeOfAscendingNodeIndex ) =  0.0;
    // lowerBounds.at( trueAnomalyIndex ) =  0.0;

    optimizer.set_lower_bounds( lowerBounds );

    // Set upper bounds.
    std::vector< double > upperBounds( 6, HUGE_VAL );
    // lowerBounds.at( semiMajorAxisIndex ) =  5.0e7;    
    upperBounds.at( eccentricityIndex ) =  1.0;
    // upperBounds.at( inclinationIndex ) =  PI;
    // upperBounds.at( argumentOfPeriapsisIndex ) =  2.0 * PI;
    // upperBounds.at( longitudeOfAscendingNodeIndex ) = 2.0 * PI;
    // upperBounds.at( trueAnomalyIndex ) = 2.0 * PI;

    optimizer.set_upper_bounds( upperBounds );

    // Set initial guess for decision vector to the TLE mean elements at departure.
    vector< double > decisionVector( 6 );
    Eigen::Map< Eigen::VectorXd >( decisionVector.data( ), 6, 1 ) = departureTLEMeanElementsGuess;

    // // Set initial step size.
    // optimizer.set_initial_step( 0.01 );

    // Execute optimizer.
    double minimumFunctionValue;
    nlopt::result result = optimizer.optimize( decisionVector, minimumFunctionValue );

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
    cout << "# of iterations: " << optimizerIterations << endl;
    cout << endl;

    /////////////////////////////////////////////////////////////////////////
  
    /////////////////////////////////////////////////////////////////////////
                
    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 

    /////////////////////////////////////////////////////////////////////////
}