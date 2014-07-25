 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iomanip>
#include <iostream>
#include <limits>

#include <libsgp4/Eci.h>  
#include <libsgp4/SGP4.h> 

#include <gsl/gsl_multiroots.h>

#include <TudatCore/Astrodynamics/BasicAstrodynamics/astrodynamicsFunctions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/orbitalElementConversions.h>
#include <TudatCore/Astrodynamics/BasicAstrodynamics/physicalConstants.h>  
#include <TudatCore/Astrodynamics/BasicAstrodynamics/unitConversions.h>  
#include <TudatCore/Mathematics/BasicMathematics/basicMathematicsFunctions.h>  
#include <TudatCore/Mathematics/BasicMathematics/mathematicalConstants.h> 

#include "D2D/cartesianToTwoLineElements.h"
#include "D2D/printFunctions.h"

namespace d2d
{

//! Update TLE mean elements.
const Tle updateTleMeanElements( 
    const Eigen::VectorXd newKeplerianElements, 
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
        = computeModulo( 
        	convertRadiansToDegrees( newKeplerianElements( inclinationIndex ) ), 360.0 ); 

    // Compute new mean right ascending node [deg].
    const double newRightAscendingNode 
        = computeModulo( 
            convertRadiansToDegrees( 
            	newKeplerianElements( longitudeOfAscendingNodeIndex ) ), 360.0 ); 

    // Compute new mean eccentricity [-].
    const double newEccentricity = newKeplerianElements( eccentricityIndex ); 

    // Compute new mean argument of perigee [deg].
    const double newArgumentPerigee 
        = computeModulo( 
            convertRadiansToDegrees( newKeplerianElements( argumentOfPeriapsisIndex ) ), 360.0 ); 

    // Compute new eccentric anomaly [rad].
    const double eccentricAnomaly
        = convertTrueAnomalyToEccentricAnomaly( 
            newKeplerianElements( trueAnomalyIndex ), newEccentricity );

    // Compute new mean mean anomaly [deg].
    const double newMeanAnomaly
        = computeModulo( 
        	convertRadiansToDegrees( 
                convertEccentricAnomalyToMeanAnomaly( eccentricAnomaly, newEccentricity ) ), 360.0 );

    // Compute new mean motion [rev/day].
    const double newMeanMotion 
        = computeKeplerMeanMotion( newKeplerianElements( semiMajorAxisIndex ),
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

//! Evaluate system of non-linear equations for converting from Cartesian state to TLE.
int evaluateCartesianToTwoLineElementsSystem( const gsl_vector* independentVariables, 
                                              void* parameters, gsl_vector* functionValues )
{
    // Declare using-statements.
    using namespace tudat::basic_astrodynamics::unit_conversions;

    // Store Keplerian elements in vector of independent variables.
    Eigen::VectorXd currentStateInKeplerianElements( 6 );
    for ( unsigned int i = 0; i < 6; i++ )
    {
        currentStateInKeplerianElements[ i ] = gsl_vector_get( independentVariables, i );
    }

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

    // Evaluate system of non-linear equations and store function values.
    for ( unsigned int i = 0; i < 6; i++ )
    {
        // gsl_vector_set( functionValues, i, ( newState[ i ] - targetState[ i ] ) 
        //                                    * ( newState[ i ] - targetState[ i ] ) );
        gsl_vector_set( functionValues, i, ( newState[ i ] - targetState[ i ] ) );        
    }

    // Return success.
    return GSL_SUCCESS;
}

//! Convert Cartesian state to TLE (Two Line Elements).
const Tle convertCartesianStateToTwoLineElements( const Eigen::VectorXd targetState,
                                                  const DateTime targetEpoch,
                                                  const Tle referenceTle,
                                                  const double earthGravitationalParameter,
                                                  const double tolerance,
                                                  const bool isPrintProgress )
{
    // Declare using-statements.
    using namespace tudat::basic_astrodynamics::orbital_element_conversions;
    using namespace tudat::basic_astrodynamics::unit_conversions;

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

    // Set up function.
    gsl_multiroot_function cartesianToTwoLineElementsFunction
        = {
             &evaluateCartesianToTwoLineElementsSystem, 
             6, 
             &parameters
          };

    // Set initial guess.
    gsl_vector* initialGuess = gsl_vector_alloc( 6 );
    for ( unsigned int i = 0; i < 6; i++ )
    {
        gsl_vector_set( initialGuess, i, currentStateInKeplerianElements[ i ] );      
    }

    // Set up solver type (derivative free).
    const gsl_multiroot_fsolver_type* solverType = gsl_multiroot_fsolver_hybrids;

    // Allocate memory for solver.
    gsl_multiroot_fsolver* solver = gsl_multiroot_fsolver_alloc( solverType, 6 );

    // Set solver to use function with initial guess.
    gsl_multiroot_fsolver_set( solver, &cartesianToTwoLineElementsFunction, initialGuess );

    // Declare current solver status and iteration counter.
    int solverStatus;
    int iterationCounter = 0;

    // Print progress table header if flag is set to true.
    if ( isPrintProgress )
    {
        printSolverStateTableHeader( );        
    }

    do
    {
        // Print current state of solver if flag is set to true.
        if ( isPrintProgress )
        {
            printSolverState( iterationCounter, solver );        
        }

        // Increment iteration counter.
        ++iterationCounter;

        // Execute solver iteration.
        solverStatus = gsl_multiroot_fsolver_iterate( solver );

        // Check if solver is stuck; if it is stuck, break from loop.
        if ( solverStatus )   
        {
            std::cerr << "ERROR: Non-linear solver is stuck!" << std::endl;
            break;
        }

        // Check if root has been found (within tolerance).
        solverStatus = gsl_multiroot_test_residual( solver->f, tolerance );

    } while ( solverStatus == GSL_CONTINUE && iterationCounter < 100 );

    // Print final status of solver if flag is set to true.
    if ( isPrintProgress )
    {
        std::cout << std::endl;
        std::cout << "Status of non-linear solver: " << gsl_strerror( solverStatus ) << std::endl;
        std::cout << std::endl;
    }

    // Store final Keplerian elements.
    Eigen::VectorXd finalKeplerianElements( 6 );
    for ( unsigned int i = 0; i < 6; i++ )
    {
        finalKeplerianElements( i ) = gsl_vector_get( solver->x, i );
    }

    // Free up memory.
    gsl_multiroot_fsolver_free( solver );
    gsl_vector_free( initialGuess );

    // Update and return new TLE object.
    return updateTleMeanElements( finalKeplerianElements, newTle, earthGravitationalParameter );
}

} // namespace d2d