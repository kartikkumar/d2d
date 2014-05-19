 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

// #include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <boost/format.hpp>

#include <Eigen/Core>

// #include <gsl/gsl_vector.h>

// #include <libsgp4/Eci.h>
// #include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/astrodynamicsFunctions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  
#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>
#include <TudatCore/Mathematics/BasicMathematics/mathematicalConstants.h>
  
namespace d2d
{

static double counter = 0;

struct CartesianToTwoLineElementsParameters
{ 
public:

    //! Constructor taking parameter values.
    CartesianToTwoLineElementsParameters( const double anEarthGravitationalParameter,
                                          const double aReferencePositionMagnitude,
                                          const double aReferenceVelocityMagnitude,
                                          const Tle someReferenceTwoLineElements )
        : earthGravitationalParameter( anEarthGravitationalParameter ),
          referencePositionMagnitude( aReferencePositionMagnitude ),
          referenceVelocityMagnitude( aReferenceVelocityMagnitude ),
          referenceTwoLineElements( someReferenceTwoLineElements )
    { }

    //! Earth gravitational parameter [kg m^3 s^-2].
    const double earthGravitationalParameter;

    //! Reference position magnitude.
    const double referencePositionMagnitude;

    //! Reference velocity magnitude.
    const double referenceVelocityMagnitude;

    //! Reference TLE.
    const Tle referenceTwoLineElements;

    // Eigen::VectorXd targetState;

protected:

private:
};

double cartesianToTwoLineElementsObjectiveFunction( 
    const std::vector< double >& state, std::vector< double >& gradient, void* parameters )

{    
    ++counter;
    
    ///////////////////////////////////////////////////////////////////////////

    // Declare using-statements.
    using std::string;

    using boost::format;

    using namespace tudat::basic_astrodynamics;
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace tudat::basic_astrodynamics::physical_constants;   
    using namespace tudat::basic_mathematics::mathematical_constants; 

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Store Earth gravitational parameter.
    const double earthGravitationalParameter 
        = static_cast< CartesianToTwoLineElementsParameters* >( 
            parameters )->earthGravitationalParameter;

    // Store reference position magnitude.
    const double referencePositionMagnitude 
        = static_cast< CartesianToTwoLineElementsParameters* >( 
            parameters )->referencePositionMagnitude;

    // Store reference velocity magnitude.
    const double referenceVelocityMagnitude 
        = static_cast< CartesianToTwoLineElementsParameters* >( 
            parameters )->referenceVelocityMagnitude;

    // Recover dimensional state.
    Eigen::Map< const Eigen::VectorXd > stateVector( &state.data( )[ 0 ], 6 );
    stateVector.segment( 0, 3 ).array( ) * referencePositionMagnitude;
    stateVector.segment( 3, 3 ).array( ) * referenceVelocityMagnitude;    

for ( unsigned int i = 0; i < 5; i++ )
    std::cout << stateVector( i ) << ", ";
std::cout << stateVector( 5 ) << std::endl;

    // Convert state to Keplerian elements.
    const Eigen::VectorXd stateInKeplerianElements
        = convertCartesianToKeplerianElements( stateVector, earthGravitationalParameter );

    // Compute eccentric anomaly from true anomaly [rad].
    const double eccentricAnomaly 
      = convertTrueAnomalyToEccentricAnomaly( 
          stateInKeplerianElements( trueAnomalyIndex ),
          stateInKeplerianElements( eccentricityIndex ) );

    // Compute mean anomaly from eccentric anomaly [rad].
    const double meanAnomaly
      = convertEccentricAnomalyToMeanAnomaly( 
          eccentricAnomaly, stateInKeplerianElements( eccentricityIndex ) );

    // Compute orbital mean motion [rad/s] from semi-major axis.
    const double meanMotion
      = computeKeplerMeanMotion( stateInKeplerianElements( semiMajorAxisIndex ),
                                 earthGravitationalParameter );

    // Compute revolutions/day from orbital mean motion [rad/s].
    const double revolutionsPerDay = meanMotion * JULIAN_DAY / ( 2.0 * PI );

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Generate new TLE based on osculating Keplerian elements.

    // Set reference TLE object.
    Tle referenceTwoLineElements( 
        static_cast< CartesianToTwoLineElementsParameters* >( 
                parameters )->referenceTwoLineElements );

std::cout << "old Line 2: " << referenceTwoLineElements.Line2( ) << std::endl;

    // Modify reference TLE based on Keplerian elements.
    string newTwoLineElementsLine1 = referenceTwoLineElements.Line1( );
    string newTwoLineElementsLine2 = referenceTwoLineElements.Line2( );
  
    // Convert new inclination to formatted string.
    const string newInclinationString
      = boost::str( format( "%08.4f" ) 
            % convertRadiansToDegrees( stateInKeplerianElements( inclinationIndex ) ) );

    // Replace new inclination [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 8, 8, newInclinationString );

    // Convert new right ascension of ascending node to formatted string.
    const string newRightAscensionOfAscendingNodeString
      = boost::str( format( "%08.4f" ) 
            % convertRadiansToDegrees( 
                stateInKeplerianElements( longitudeOfAscendingNodeIndex ) ) );

    // Replace new right ascension of ascending node [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 17, 8, newRightAscensionOfAscendingNodeString );

    // Convert new eccentricity to formatted string.
std::cout << "e: " <<  stateInKeplerianElements( eccentricityIndex ) << std::endl;
    const string newEccentricityString
      = boost::str( format( "%07.0f" ) 
                        % ( stateInKeplerianElements( eccentricityIndex ) * 1.0e7 ) );

    // Replace new eccentricity [-] in TLE line 2.
    newTwoLineElementsLine2.replace( 26, 7, newEccentricityString );

    // Convert new argument of periapsis to formatted string.
    const string newArgumentOfPeriapsisString
      = boost::str( format( "%08.4f" ) 
                        % convertRadiansToDegrees( 
                            stateInKeplerianElements( argumentOfPeriapsisIndex ) ) );

    // Replace new argument of periapsis [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 34, 8, newArgumentOfPeriapsisString );    

    // Convert new mean anomaly to formatted string.
    const string newMeanAnomalyString
      = boost::str( format( "%08.4f" ) % convertRadiansToDegrees( meanAnomaly ) );

    // Replace new mean anomaly [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 43, 8, newMeanAnomalyString ); 

    // Convert new revolutions/day to formatted string.
    const string newRevolutionsPerDayString = boost::str( format( "%11.8f" ) % revolutionsPerDay );

    // Replace new mean anomaly [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 52, 11, newRevolutionsPerDayString );     

std::cout << "new Line 2: " << newTwoLineElementsLine2 << std::endl;

    // Set new TLE object.
    const Tle newTwoLineElements( newTwoLineElementsLine1, newTwoLineElementsLine2 );

    ///////////////////////////////////////////////////////////////////////////   

    ///////////////////////////////////////////////////////////////////////////

//     // Compute non-linear function.

//     // Set up SGP4 propagator for new TLE.
//     SGP4 sgp4NewObject( newTwoLineElements );

//     // Convert new TLE to Cartesian state by propagating by 0.0.
//     Eci newState = sgp4NewObject.FindPosition( 0.0 ); 

//     // Set number of unknowns (size of Cartesian state).
//     const int numberOfUnknowns 
//         = static_cast< CartesianToTwoLineElementsParameters* >( 
//             parameters )->numberOfUnknowns;

//     // Set target Cartesian state.
//     const Eigen::VectorXd targetState
//         = static_cast< CartesianToTwoLineElementsParameters* >( 
//             parameters )->targetState;

//     // Set new Cartesian state, computed from new TLE.
//     const Eigen::VectorXd newStateVector
//         = ( Eigen::VectorXd( 6 ) << newState.Position( ).x * 1.0e3,
//                                     newState.Position( ).y * 1.0e3,
//                                     newState.Position( ).z * 1.0e3,
//                                     newState.Velocity( ).x * 1.0e3,
//                                     newState.Velocity( ).y * 1.0e3,
//                                     newState.Velocity( ).z * 1.0e3 ).finished( );

// // std::cout << "New: \n" << newStateVector << std::endl;

// // std::cout << "Diff: \n" << newStateVector - targetState << std::endl;

//     // Evaluate set of non-linear functions to solve.
//     computeVectorDifference( 
//         newStateVector, targetState, numberOfUnknowns, cartesianToTwoLineElementsFunction );

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Return value of objective function.
    return 0.0;

    ///////////////////////////////////////////////////////////////////////////
}

} // namespace d2d