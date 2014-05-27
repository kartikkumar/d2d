 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iostream>
#include <string>
#include <vector>

#include <boost/format.hpp>

#include <Eigen/Core>

#include <libsgp4/Eci.h>
#include <libsgp4/DateTime.h>  
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/astrodynamicsFunctions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  
#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>
#include <TudatCore/Mathematics/BasicMathematics/basicMathematicsFunctions.h> 
  
namespace d2d
{

//! Number of iterations taken by optimizer to convert Cartesian to TLE elements.
static double optimizerIterations = 0;

//! Set enum for TLE mean elements indices.
enum tleMeanElementIndices
{
  tleInclinationIndex,
  tleRightAscensingNodeIndex,
  tlEccentricityIndex,
  tleArgumentPerigeeIndex,
  tleMeanAnomalyIndex,
  tleMeanMotion
};

//! Container of parameters used by objective function to convert Cartesian state to TLE elements.
struct ObjectiveFunctionParameters
{ 
public:

    //! Constructor taking parameter values.
    ObjectiveFunctionParameters( const double anEarthGravitationalParameter,
                                 const Tle someReferenceTle,
                                 const Eigen::VectorXd aTargetState,
                                 const DateTime aTargetEpoch )
        : earthGravitationalParameter( anEarthGravitationalParameter ),
          referenceTle( someReferenceTle ),
          targetState( aTargetState ),
          targetEpoch( aTargetEpoch )
    { }

    //! Earth gravitational parameter [kg m^3 s^-2].
    const double earthGravitationalParameter;

    //! Reference TLE.
    const Tle referenceTle;

    //! Target state in Cartesian elements.
    const Eigen::VectorXd targetState;

    //! Target epoch.
    const DateTime targetEpoch;

protected:

private:
};

double cartesianToTwoLineElementsObjectiveFunction( 
    const std::vector< double >& decisionVector, 
    std::vector< double >& gradient, void* parameters )

{    
    // Increment iterations counter.
    ++optimizerIterations;

    ///////////////////////////////////////////////////////////////////////////

    // Declare using-statements.
    using std::string;

    using boost::format;

    using namespace tudat::basic_astrodynamics;
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace tudat::basic_astrodynamics::physical_constants;   
    using namespace tudat::basic_mathematics; 

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Generate new TLE and compute Cartesian elements at target epoch.

    // Map decision vector to TLE mean elements.
    Eigen::Map< const Eigen::VectorXd > newTleMeanElements( &decisionVector[ 0 ], 6, 1 );

    // Set reference TLE object.
    const Tle referenceTle( 
      static_cast< ObjectiveFunctionParameters* >( parameters )->referenceTle );

std::cout << "old Line 2: " << referenceTle.Line2( ) << std::endl;

    // Modify reference TLE based on Keplerian elements.
    string newTleLine1 = referenceTle.Line1( );
    string newTleLine2 = referenceTle.Line2( );
  
    // Convert new TLE inclination to formatted string.
    const string newTleInclinationString
      = boost::str( format( "%08.4f" ) % newTleMeanElements( tleInclinationIndex ) );

    // Replace new inclination [deg] in TLE line 2.
    newTleLine2.replace( 8, 8, newTleInclinationString );

    // Convert new right ascension of ascending node to formatted string.
    const string newRightAscensionOfAscendingNodeString
      = boost::str( format( "%08.4f" ) 
          % computeModulo( newTleMeanElements( tleRightAscensingNodeIndex ), 360.0 ) );

    // Replace new right ascension of ascending node [deg] in TLE line 2.
    newTleLine2.replace( 17, 8, newRightAscensionOfAscendingNodeString );

    // Convert new eccentricity to formatted string.
    const string newEccentricityString
      = boost::str( format( "%07.0f" ) % ( newTleMeanElements( tlEccentricityIndex ) * 1.0e7 ) );

    // Replace new eccentricity [-] in TLE line 2.
    newTleLine2.replace( 26, 7, newEccentricityString );

    // Convert new argument of periapsis to formatted string.
    const string newArgumentOfPeriapsisString
      = boost::str( format( "%08.4f" ) 
          % computeModulo( newTleMeanElements( tleArgumentPerigeeIndex ), 360.0 ) );

    // Replace new argument of periapsis [deg] in TLE line 2.
    newTleLine2.replace( 34, 8, newArgumentOfPeriapsisString );    

    // Convert new mean anomaly to formatted string.
    const string newMeanAnomalyString
      = boost::str( format( "%08.4f" ) 
          % computeModulo( newTleMeanElements( tleMeanAnomalyIndex ), 360.0 ) );

    // Replace new mean anomaly [deg] in TLE line 2.
    newTleLine2.replace( 43, 8, newMeanAnomalyString ); 

    // Convert new revolutions/day to formatted string.
    const string newRevolutionsPerDayString 
      = boost::str( format( "%11.8f" ) % newTleMeanElements( tleMeanMotion ) );

    // Replace new mean anomaly [deg] in TLE line 2.
    newTleLine2.replace( 52, 11, newRevolutionsPerDayString );     

std::cout << "new Line 2: " << newTleLine2 << std::endl;

    // Set new TLE object.
    const Tle newTwoLineElements( newTleLine1, newTleLine2 );

    // Set up SGP4 propagator for new TLE.
    const SGP4 sgp4NewObject( newTwoLineElements );

    // Convert new TLE to Cartesian state by propagating to target epoch.
    const Eci newState 
      = sgp4NewObject.FindPosition( 
          static_cast< ObjectiveFunctionParameters* >( parameters )->targetEpoch ); 

    // Set target Cartesian state.
    const Eigen::VectorXd targetState
        = static_cast< ObjectiveFunctionParameters* >( parameters )->targetState;

    // Set new Cartesian state, computed from new TLE.
    const Eigen::VectorXd newStateVector
        = ( Eigen::VectorXd( 6 ) 
              << convertKilometersToMeters( newState.Position( ).x ),
                 convertKilometersToMeters( newState.Position( ).y ),
                 convertKilometersToMeters( newState.Position( ).z ),
                 convertKilometersToMeters( newState.Velocity( ).x ),
                 convertKilometersToMeters( newState.Velocity( ).y ),
                 convertKilometersToMeters( newState.Velocity( ).z ) ).finished( );

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Return value of objective function.
    std::cout << "Obj: " << ( newStateVector - targetState ).norm( ) << std::endl;
    std::cout << "new: " << newStateVector[ 0 ] << " "
                         << newStateVector[ 1 ] << " "
                         << newStateVector[ 2 ] << " "
                         << newStateVector[ 3 ] << " "
                         << newStateVector[ 4 ] << " "
                         << newStateVector[ 5 ] << std::endl;
    std::cout << "tar: " << targetState[ 0 ] << " "
                         << targetState[ 1 ] << " "
                         << targetState[ 2 ] << " "
                         << targetState[ 3 ] << " "
                         << targetState[ 4 ] << " "
                         << targetState[ 5 ] << std::endl;   

    return ( newStateVector - targetState ).squaredNorm( );

    ///////////////////////////////////////////////////////////////////////////
}

} // namespace d2d