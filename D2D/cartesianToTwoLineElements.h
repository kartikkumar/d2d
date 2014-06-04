 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <vector>

#include <Eigen/Core>
 
#include <gsl/gsl_vector.h>

#include <libsgp4/Tle.h>

namespace d2d
{

//! Container of parameters used by Cartesian-to-TLE objective function.
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
          targetState( aTargetState )
    { }

    //! Reference TLE.
    const Tle referenceTle;

    //! Earth gravitational parameter [kg m^3 s^-2].
    const double earthGravitationalParameter;

    //! Target state in Cartesian elements.
    const Eigen::VectorXd targetState;

protected:

private:
};

//! Update TLE mean elements.
/*!
 * Updates mean elements stored in TLE based on current osculating elements. This function 
 * uses the osculating elements to replace mean elements (converts units and computes mean anomaly
 * and mean motion).
 * \param newKeplerianElements New Keplerian elements.
 * \param oldTle TLE in which the mean elements are to be replaced.
 * \param earthGravitationalParameter Earth gravitational parameter [m^3 s^-2].
 * \return New TLE with mean elements updated.
 */
const Tle updateTleMeanElements( 
    const Eigen::VectorXd newKeplerianElements, 
    const Tle oldTle, const double earthGravitationalParameter );

//! Evaluate system of non-linear equations for converting from Cartesian state to TLE.
/*!
 * Evaluates system of non-linear equations to find TLE corresponding with target Cartesian 
 * state. The system of non-linear equations used is:
 *  \f[ 
 *      F = 0 = \bar{x}^{new} - \bar{x}^{target}
 *  \f]
 * where \f$\bar{x}^{new}\f$ is the new Cartesian state computed by updating the TLE mean elements 
 * and propagating the TLE using the SGP4 propagator, and \f$\bar{x}^{target}\f$ is the target
 * Cartesian state.
 * \param indepedentVariables Vector of independent variables used by the root-finder.
 * \param parameters Parameters required to compute the objective function.
 * \param functionValues Vector of computed function values. 
 */
int evaluateCartesianToTwoLineElementsSystem( const gsl_vector* independentVariables, 
                                              void* parameters, 
                                              gsl_vector* functionValues );

//! Convert Cartesian state to TLE (Two Line Elements).
/*!
 * Converts a given Cartesian state to an effective TLE.
 * \param targetState Target Cartesian state.
 * \param targetEpoch Epoch associated with target Cartesian state.
 * \param referenceTle Reference Two Line Elements.
 * \param earthGravitationalParameter Earth gravitational parameter [m^3 s^-2].
 * \param tolerance Tolerance.
 * \return TLE object that generates target Cartesian state when propagated with SGP4 propagator to
 *           target epoch.
 */
const Tle convertCartesianStateToTwoLineElements( const Eigen::VectorXd targetState,
                                                  const DateTime targetEpoch,
                                                  const Tle referenceTle,
                                                  const double earthGravitationalParameter,
                                                  const double tolerance = 1.0e-6 );

} // namespace d2d