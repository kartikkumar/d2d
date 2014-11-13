/*    
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <keplerian_toolbox.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

//! Execute Lambert scanner.
/*!
 * Executes Lambert scanner that performs a grid search to compute \f$\Delta V\f$ for 
 * debris-to-debris transfers. The transfers are modelled as conic sections. The Lambert targeter
 * employed is based on Izzo (2014), implemented in PyKEP (Izzo, 2012).
 *
 * @param configuration User-defined configuration options (extracted from JSON input file)
 */
void executeLambertScanner( const rapidjson::Document& configuration );

//! Create Lambert scanner table.
/*!
 * Creates Lambert scanner table in SQLite database used to store results obtaned from running 
 * Lambert scanner. If the database already exists, it is inspected to make sure that the table 
 * schema is correct.
 *
 * @sa executeLambertScanner
 * @param database 			SQLite database handle
 */
void createLambertScannerTable( SQLite::Database& database );

} // namespace d2d

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in 
 * 	astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth 
 *  International Conf. Astrodynam. Tools and Techniques, ICATT. 2012.
 */
