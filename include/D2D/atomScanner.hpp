/*    
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <rapidjson/document.h>

namespace d2d
{

//! Execute Atom scanner.
/*!
 * Executes Atom scanner that performs a grid search to compute \f$\Delta V\f$ for 
 * debris-to-debris transfers. The transfers are modelled using the SGP4/SDP4 propagator 
 * (Vallado, 2006). The Atom solver employed is based on Kumar, et al. (2014), implemented in 
 * Atom (Kumar, 2014).
 *
 * @param jsonInput User-defined input options (extracted from JSON input file)
 */
void executeLambertScanner( const rapidjson::Document& jsonInput );

//! Execute SGP4 forward propagation.
/*!
 * Executes forward propagation of transfer computed with Lambert targeter (Izzo, 2014) using the
 * SGP4/SDP4 models to compute the mismatch in position due to perturbations.
 *
 * @sa executeLambertScanner
 * @param jsonInput User-defined input options (extracted from JSON input file)
 */
void executeSGP4ForwardPropagation( const rapidjson::Document& jsonInput );

} // namespace d2d

/*!
 * Vallado (2006)
 * Kumar, K., et al. (2014)
 * Kumar, K., et al. (2014) https://github.com/kartikkumar.com/atom
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705. 
 */
