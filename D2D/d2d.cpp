 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <boost/timer/timer.hpp>

#include <Eigen/Core>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>  
#include <libsgp4/Globals.h>      
#include <libsgp4/SGP4.h>  
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/astrodynamicsFunctions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>  
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  
#include <TudatCore/Mathematics/BasicMathematics/basicMathematicsFunctions.h>  
#include <TudatCore/Mathematics/BasicMathematics/mathematicalConstants.h>  

//! Convert Cartesian state to TLE (Two Line Elements).
/*!
 * Converts a given Cartesian state to an effective TLE.
 * \param targetState Target Cartesian state.
 * \param epoch Epoch associated with Cartesian state.
 * \param referenceTle Reference Two Line Elements.
 * \param earthGravitationalParameter Earth gravitational parameter [m^3 s^-2].
 * \return TLE object that generates target Cartesian state when propagated with SGP4 propagator to
 *           target epoch.
 */
const Tle convertCartesianStateToTwoLineElements( const Eigen::VectorXd targetState,
                                                  const DateTime epoch,
                                                  const Tle referenceTle,
                                                  const double earthGravitationalParameter );

//! Execute D2D application.
int main( const int numberOfInputs, const char* inputArguments[ ] )
{
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Declare using-statements.
    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    using boost::timer::auto_cpu_timer;

    using namespace tudat::basic_astrodynamics::unit_conversions;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Start timer. Timer automatically ends when this object goes out of scope.
    auto_cpu_timer timer;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Input deck.
    
    // Set TLE strings for 1st debris object.
    const string nameObject1 = "0 VANGUARD 1";
    const string line1Object1 
        = "1 00005U 58002B   14143.59770889 -.00000036  00000-0 -22819-4 0  2222";
    const string line2Object1 
        = "2 00005 034.2431 215.7533 1849786 098.8024 282.5368 10.84391811964621";

    ////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Set Earth gravitational parameter [m^3 s^-2].
    const double earthGravitationalParameter = kMU * 1.0e9;

    // Set up TLE object.
    const Tle tleObject1( nameObject1, line1Object1, line2Object1 );

    // Set up SGP4 propagator for TLE object.
    const SGP4 sgp4( tleObject1 );

    // Propagate TLE to epoch of TLE.
    Eci propagatedState = sgp4.FindPosition( 0.0 );

    // Store propagated state as target Cartesian state.
    const Eigen::VectorXd targetState 
        = ( Eigen::VectorXd( 6 ) 
                << convertKilometersToMeters( propagatedState.Position( ).x ),
                   convertKilometersToMeters( propagatedState.Position( ).y ),
                   convertKilometersToMeters( propagatedState.Position( ).z ),
                   convertKilometersToMeters( propagatedState.Velocity( ).x ),
                   convertKilometersToMeters( propagatedState.Velocity( ).y ),
                   convertKilometersToMeters( propagatedState.Velocity( ).z ) ).finished( );

    // Convert target Cartesian state to a new TLE object.
    convertCartesianStateToTwoLineElements( 
        targetState, tleObject1.Epoch( ), tleObject1, earthGravitationalParameter );

    ////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////
                
    cout << "Timing information: ";

    // If program is successfully completed, return 0.
    return EXIT_SUCCESS; 

    ////////////////////////////////////////////////////////////////////////////////////////////////    
}

const Tle convertCartesianStateToTwoLineElements( const Eigen::VectorXd targetState,
                                                  const DateTime epoch,
                                                  const Tle referenceTle,
                                                  const double earthGravitationalParameter )
{
    using namespace tudat::basic_astrodynamics;
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::physical_constants;     
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace tudat::basic_mathematics; 
    using namespace tudat::basic_mathematics::mathematical_constants;     

    // Store target Cartesian state as current state.
    Eigen::VectorXd currentState = targetState;

    // Store new TLE as reference TLE.
    Tle newTle = referenceTle;

    // Update TLE epoch.
    newTle.updateEpoch( epoch );

    //
    int numberOfIterations = 0;

    do
    {
        // Convert Cartesian state to Keplerian elements.
        const Eigen::VectorXd currentStateInKeplerianElements 
            = convertCartesianToKeplerianElements( currentState, earthGravitationalParameter );

        // Compute new mean inclination [deg].
        const double newInclination 
            = computeModulo( convertRadiansToDegrees( 
                                currentStateInKeplerianElements( inclinationIndex ) ), 360.0 ); 

        // Compute new mean right ascending node [deg].
        const double newRightAscendingNode 
            = computeModulo( 
                convertRadiansToDegrees( 
                    currentStateInKeplerianElements( longitudeOfAscendingNodeIndex ) ), 360.0 ); 

        // Compute new mean eccentricity [-].
        const double newEccentricity = currentStateInKeplerianElements( eccentricityIndex ); 

        // Compute new mean argument of perigee [deg].
        const double newArgumentPerigee 
            = computeModulo( 
                convertRadiansToDegrees( 
                    currentStateInKeplerianElements( argumentOfPeriapsisIndex ) ), 360.0 ); 

        // Compute new eccentric anomaly [rad].
        const double eccentricAnomaly
            = convertTrueAnomalyToEccentricAnomaly( 
                currentStateInKeplerianElements( trueAnomalyIndex ), 
                currentStateInKeplerianElements( eccentricityIndex ) );

        // Compute new mean mean anomaly [deg].
        const double newMeanAnomaly
            = computeModulo( convertRadiansToDegrees( 
                    convertEccentricAnomalyToMeanAnomaly( 
                        eccentricAnomaly, 
                        currentStateInKeplerianElements( eccentricityIndex ) ) ), 360.0 );

        // Compute new mean motion [rev/day].
        const double newMeanMotion 
            = computeKeplerMeanMotion( currentStateInKeplerianElements( semiMajorAxisIndex ),
                                       earthGravitationalParameter ) / ( 2.0 * PI ) * JULIAN_DAY;

        // Update mean elements in TLE with osculating elements.
        newTle.updateMeanElements( newInclination, 
                                   newRightAscendingNode,
                                   newEccentricity,
                                   newArgumentPerigee,
                                   newMeanAnomaly, 
                                   newMeanMotion );

        // Set up SGP4 propagator for new TLE.
        SGP4 sgp4( newTle );

        // Propagate TLE to current epoch.
        Eci propagatedState = sgp4.FindPosition( 0.0 );

        // Update current state to propagated state.
        currentState( 0 ) = convertKilometersToMeters( propagatedState.Position( ).x );
        currentState( 1 ) = convertKilometersToMeters( propagatedState.Position( ).y );
        currentState( 2 ) = convertKilometersToMeters( propagatedState.Position( ).z );
        currentState( 3 ) = convertKilometersToMeters( propagatedState.Velocity( ).x );
        currentState( 4 ) = convertKilometersToMeters( propagatedState.Velocity( ).y );
        currentState( 5 ) = convertKilometersToMeters( propagatedState.Velocity( ).z );

        // Increment iterator count.
        ++numberOfIterations;

std::cout << ( currentState - targetState ).squaredNorm( ) << std::endl;
    }
    while( ( currentState - targetState ).squaredNorm( ) < 1.0e-5 || numberOfIterations < 1 );

    // TEMP
    return referenceTle;
}