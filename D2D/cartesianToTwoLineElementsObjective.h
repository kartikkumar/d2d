 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iostream>
#include <vector>

#include <Eigen/Core>

#include <libsgp4/Eci.h>  
#include <libsgp4/SGP4.h>  
#include <libsgp4/Tle.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/astrodynamicsFunctions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>  
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  
#include <TudatCore/Mathematics/BasicMathematics/basicMathematicsFunctions.h>  
#include <TudatCore/Mathematics/BasicMathematics/mathematicalConstants.h> 

namespace d2d
{

//! Container of parameters used by Cartesian state to TLE elements objective function.
struct CartesianToTwoLineElementsObjectiveParameters
{ 
public:

    //! Constructor taking parameter values.
    CartesianToTwoLineElementsObjectiveParameters( 
        const Tle someReferenceTle,
        const double anEarthGravitationalParameter,
        const Eigen::VectorXd aTargetState )
        : referenceTle( someReferenceTle ),
          earthGravitationalParameter( anEarthGravitationalParameter ),
          targetState( aTargetState ),
          iterationCounter( 0 )
    { }

    //! Reference TLE.
    const Tle referenceTle;

    //! Earth gravitational parameter [kg m^3 s^-2].
    const double earthGravitationalParameter;

    //! Target state in Cartesian elements.
    const Eigen::VectorXd targetState;

    //! Iteration counter.
    int iterationCounter;

protected:

private:
};

//! Update TLE mean elements.
const Tle updateTleMeanElements( 
    const Eigen::VectorXd currentStateInKeplerianElements, 
    const Tle oldTle, const double earthGravitationalParameter )
{
    using namespace tudat::basic_astrodynamics;
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;    
    using namespace tudat::basic_astrodynamics::physical_constants;     
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace tudat::basic_mathematics;
    using namespace tudat::basic_mathematics::mathematical_constants;    

    // Copy old TLE to new object.
    Tle newTle( oldTle );

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

    // Return new TLE object.
    return newTle;
}

//! Compute objective function for converting from Cartesian state to TLE.
double computeCartesianToTwoLineElementsObjective( const std::vector< double >& decisionVector,
                                                   std::vector< double >& gradient, 
                                                   void* parameters )
{
    // Declare using-statements.
    using namespace tudat::basic_astrodynamics::unit_conversions;

    // Store Keplerian elements in decision vector.
    Eigen::Map< const Eigen::VectorXd > currentStateInKeplerianElements(
        &decisionVector[ 0 ], 6, 1 );

    // Store old TLE.
    const Tle oldTle = static_cast< CartesianToTwoLineElementsObjectiveParameters* >( 
        parameters )->referenceTle;

    // Store gravitational parameter.
    const double earthGravitationalParameter 
        = static_cast< CartesianToTwoLineElementsObjectiveParameters* >( 
            parameters )->earthGravitationalParameter;
 
    // Update TLE mean elements and store as new TLE.
    Tle newTle = updateTleMeanElements( 
        currentStateInKeplerianElements, oldTle, earthGravitationalParameter );

    // Propagate new TLE to epoch of TLE.
    SGP4 sgp4( newTle );
    Eci propagatedState = sgp4.FindPosition( 0.0 );

    // Store new state.
    const Eigen::VectorXd newState 
        = ( Eigen::VectorXd( 6 ) 
            << convertKilometersToMeters( propagatedState.Position( ).x ),
               convertKilometersToMeters( propagatedState.Position( ).y ),
               convertKilometersToMeters( propagatedState.Position( ).z ),
               convertKilometersToMeters( propagatedState.Velocity( ).x ),
               convertKilometersToMeters( propagatedState.Velocity( ).y ),
               convertKilometersToMeters( propagatedState.Velocity( ).z ) ).finished( );

    // Store target state.
    const Eigen::VectorXd targetState
        = static_cast< CartesianToTwoLineElementsObjectiveParameters* >( parameters )->targetState;

    // Increment iteration counter.
    static_cast< CartesianToTwoLineElementsObjectiveParameters* >( 
        parameters )->iterationCounter++;

for ( unsigned int i = 0; i < 5; i ++ )
    std::cout << newState[i ] << " ";
std::cout << newState[ 5 ] << std::endl;
for ( unsigned int i = 0; i < 5; i ++ )
    std::cout << targetState[i ] << " ";
std::cout << targetState[ 5 ] << std::endl;
std::cout << ( newState - targetState ).squaredNorm( ) << std::endl;
std::cout << std::endl;

    // Return objective function.
    return ( newState - targetState ).squaredNorm( );
}

//! Convert Cartesian state to TLE (Two Line Elements).
/*!
 * Converts a given Cartesian state to an effective TLE.
 * \param targetState Target Cartesian state.
 * \param targetEpoch Epoch associated with target Cartesian state.
 * \param referenceTle Reference Two Line Elements.
 * \param earthGravitationalParameter Earth gravitational parameter [m^3 s^-2].
 * \param optimizerTolerance Optimizer tolerance.
 * \return TLE object that generates target Cartesian state when propagated with SGP4 propagator to
 *           target epoch.
 */
const Tle convertCartesianStateToTwoLineElements( const Eigen::VectorXd targetState,
                                                  const DateTime targetEpoch,
                                                  const Tle referenceTle,
                                                  const double earthGravitationalParameter,
                                                  const double optimizerTolerance = 1.0e-6 )
{
    // Declare using-statements.
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::unit_conversions;
    using namespace nlopt;

    // Store reference TLE as new TLE.
    Tle newTle = referenceTle;

    // Set up SGP4 propagator.
    SGP4 sgp4( newTle );

    // Propagate new TLE to target epoch.
    Eci propagatedState = sgp4.FindPosition( targetEpoch );

    // Store propagated state as current state.
    const Eigen::VectorXd currentState
        = ( Eigen::VectorXd( 6 )
                << convertKilometersToMeters( propagatedState.Position( ).x ),
                   convertKilometersToMeters( propagatedState.Position( ).y ),
                   convertKilometersToMeters( propagatedState.Position( ).z ),
                   convertKilometersToMeters( propagatedState.Velocity( ).x ),
                   convertKilometersToMeters( propagatedState.Velocity( ).y ),
                   convertKilometersToMeters( propagatedState.Velocity( ).z ) ).finished( );

    // Compute current state in Keplerian elements.
    const Eigen::VectorXd currentStateInKeplerianElements
        = convertCartesianToKeplerianElements( currentState, earthGravitationalParameter );

    // Update TLE epoch.
    newTle.updateEpoch( targetEpoch );

    // Set up parameters for objective function.
    CartesianToTwoLineElementsObjectiveParameters parameters( 
        newTle, earthGravitationalParameter, targetState );

    // Set up derivative-free optimizer.
    opt optimizer( LN_COBYLA, 6 );

    // Set objective function.
    optimizer.set_min_objective( computeCartesianToTwoLineElementsObjective, &parameters );

    // Set tolerance.
    optimizer.set_xtol_rel( optimizerTolerance );

    // Set initial guess for decision vector.
    std::vector< double > decisionVector( 6 );
    Eigen::Map< Eigen::VectorXd >( decisionVector.data( ), 6, 1 ) 
        = currentStateInKeplerianElements;

    // Set initial step size.
    optimizer.set_initial_step( 1.0e-4 );

    // Execute optimizer.
    double minimumFunctionValue;
    result result = optimizer.optimize( decisionVector, minimumFunctionValue );

    // Print output statements.
    if ( result < 0 ) 
    {
        std::cout << "NLOPT failed!" << std::endl;
    }
    
    else 
    {
        std::cout << "Minimum = " << minimumFunctionValue << std::endl;
        std::cout << "# iterations = " << parameters.iterationCounter << std::endl;
    }

    // TEMP
    return referenceTle;
}

} // namespace d2d