 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <vector>

#include <Eigen/Core>
 
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

//! Compute objective function for converting from Cartesian state to TLE.
/*!
 * Computes objective function used by optimizer to find TLE corresponding with target Cartesian 
 * state. The objective function used is:
 *  \f[ 
 *      J = \sum_{i=1}^{6} x_{i}^{new} - x_{i}^{target}
 *  \f]
 * where \f$x_{i}^{new}\f$ is the \f$i^{th}\f$ Cartesian coordinate of the new Cartesian state 
 * computed by updating the TLE mean elements (decision variables) and propagating the TLE using 
 * the SGP4 propagator, and \f$x_{i}^{target}\f$ is the \f$i^{th}\f$ Cartesian coordinate of the 
 * target Cartesian state.
 * \param decisionVector Vector of decision variables used by the optimizer.
 * \param gradient Jacobian (set to NULL for derivative-free algorithms).
 * \param parameters Parameters required to compute the objective function.
 */
double computeCartesianToTwoLineElementsObjective( const std::vector< double >& decisionVector,
                                                   std::vector< double >& gradient, 
                                                   void* parameters );

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
                                                  const double optimizerTolerance = 1.0e-6 );

} // namespace d2d