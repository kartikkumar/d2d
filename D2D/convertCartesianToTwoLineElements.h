 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iomanip>
#include <string>

#include <boost/format.hpp>

#include <Eigen/Core>

#include <gsl/gsl_vector.h>

#include <libsgp4/Eci.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>
#include <libsgp4/Util.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/astrodynamicsFunctions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>
#include <TudatCore/Mathematics/BasicMathematics/mathematicalConstants.h>
  
namespace d2d
{

struct CartesianToTwoLineElementsParameters
{ 
    double earthGravitationalParameter;
    Tle referenceTwoLineElements;
    Eigen::VectorXd targetState;
    int numberOfUnknowns;
};

int convertCartesianToTwoLineElements( 
    const gsl_vector* keplerianElements, 
    void* parameters, gsl_vector* cartesianToTwoLineElementsFunction )
{
    ///////////////////////////////////////////////////////////////////////////

    // Declare using-statements.
    using std::string;

    using boost::format;

    using namespace Util;

    using namespace tudat::basic_astrodynamics;
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::physical_constants;   
    using namespace tudat::basic_mathematics::mathematical_constants; 

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Store Earth gravitational parameter.
    const double earthGravitationalParameter 
        = static_cast< CartesianToTwoLineElementsParameters* >( 
            parameters )->earthGravitationalParameter;

    // Set Keplerian elements.
    const Eigen::VectorXd keplerianElementsVector 
        = ( Eigen::VectorXd( 6 ) 
                << gsl_vector_get( keplerianElements, 0 ),
                   gsl_vector_get( keplerianElements, 1 ),
                   gsl_vector_get( keplerianElements, 2 ),
                   gsl_vector_get( keplerianElements, 3 ),
                   gsl_vector_get( keplerianElements, 4 ),
                   gsl_vector_get( keplerianElements, 5 ) ).finished( );

    // Compute eccentric anomaly from true anomaly [rad].
    const double eccentricAnomaly 
      = convertTrueAnomalyToEccentricAnomaly( 
          keplerianElementsVector( trueAnomalyIndex ),
          keplerianElementsVector( eccentricityIndex ) );

    // Compute mean anomaly from eccentric anomaly [rad].
    const double meanAnomaly
      = convertEccentricAnomalyToMeanAnomaly( 
          eccentricAnomaly, keplerianElementsVector( eccentricityIndex ) );

    // Compute orbital mean motion [rad/s] from semi-major axis.
    const double meanMotion
      = computeKeplerMeanMotion( keplerianElementsVector( semiMajorAxisIndex ),
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

    // Modify reference TLE based on Keplerian elements.
    string newTwoLineElementsLine1 = referenceTwoLineElements.Line1( );
    string newTwoLineElementsLine2 = referenceTwoLineElements.Line2( );
  
    // Convert new inclination to formatted string.
    const string newInclinationString
      = boost::str( format( "%08.4f" ) 
            % RadiansToDegrees( keplerianElementsVector( inclinationIndex ) ) );

    // Replace new inclination [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 8, 8, newInclinationString );

    // Convert new right ascension of ascending node to formatted string.
    const string newRightAscensionOfAscendingNodeString
      = boost::str( format( "%08.4f" ) 
            % RadiansToDegrees( keplerianElementsVector( longitudeOfAscendingNodeIndex ) ) );

    // Replace new right ascension of ascending node [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 17, 8, newRightAscensionOfAscendingNodeString );

    // Convert new eccentricity to formatted string.
    const string newEccentricityString
      = boost::str( format( "%7.0f" ) % ( keplerianElementsVector( eccentricityIndex ) * 1.0e7 ) );

    // Replace new eccentricity [-] in TLE line 2.
    newTwoLineElementsLine2.replace( 26, 7, newEccentricityString );

    // Convert new argument of periapsis to formatted string.
    const string newArgumentOfPeriapsisString
      = boost::str( format( "%08.4f" ) 
            % RadiansToDegrees( keplerianElementsVector( argumentOfPeriapsisIndex ) ) );

    // Replace new argument of periapsis [deg] in TLE line 2.
    newTwoLineElementsLine2.replace( 34, 8, newArgumentOfPeriapsisString );    

    // Convert new mean anomaly to formatted string.
    const string newMeanAnomalyString
      = boost::str( format( "%08.4f" ) % RadiansToDegrees( meanAnomaly ) );

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

    // Compute non-linear function.

    // Set up SGP4 propagator for new TLE.
    SGP4 sgp4( newTwoLineElements );

    // Convert new TLE to Cartesian state by propagating by 0.0.
    Eci newState = sgp4.FindPosition( 0.0 ); 

    // Set number of unknowns (size of Cartesian state).
    const int numberOfUnknowns 
        = static_cast< CartesianToTwoLineElementsParameters* >( 
            parameters )->numberOfUnknowns;

    // Set target Cartesian state.
    const Eigen::VectorXd targetState
        = static_cast< CartesianToTwoLineElementsParameters* >( 
            parameters )->targetState;

    // Set new Cartesian state, computed from new TLE.
    const Eigen::VectorXd newStateVector
        = ( Eigen::VectorXd( 6 ) << newState.Position( ).x * 1.0e3,
                                    newState.Position( ).y * 1.0e3,
                                    newState.Position( ).z * 1.0e3,
                                    newState.Velocity( ).x * 1.0e3,
                                    newState.Velocity( ).y * 1.0e3,
                                    newState.Velocity( ).z * 1.0e3 ).finished( );

std::cout << "Diff: \n" << newStateVector - targetState << std::endl;

    // Evaluate set of non-linear functions to solve.
    for ( int i = 0; i < numberOfUnknowns; i++ )
    {
        gsl_vector_set( cartesianToTwoLineElementsFunction, 
                        i,  newStateVector( i ) - targetState( i ) );    
    }

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Return success if this point is reached.
    return GSL_SUCCESS;

    ///////////////////////////////////////////////////////////////////////////
}

} // namespace d2d