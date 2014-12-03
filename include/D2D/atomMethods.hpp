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

//! Atom scanner input.
/*!
 * Data struct containing all valid Atom scanner input parameters. This struct is populated by
 * the checkAtomScannerInput() function and can be used to execute the Atom scanner.
 *
 * @sa checkAtomScannerInput, executeAtomScanner
 */
struct AtomScannerInput
{
public:

	//! Construct data struct.
	/*!
	 * Constructs data struct based on verified input parameters.
	 *
	 * @sa checkAtomScannerInput, executeAtomScanner
	 * @param aTransferList 		Path to list of Lambert transfers
	 * @param aDatabasePath         Path to SQLite database
	 */
	AtomScannerInput( const std::string aTransferList,
					  const std::string aDatabasePath )
		: transferList( aTransferList ),
		  databasePath( aDatabasePath ),
	{ }

	//! Path to Lambert transfer list.
	const std::string transferList;

	//! Path to SQLite database to store output.
	const std::string databasePath;

protected:

private:
};

//! Check Atom scanner input parameters.
/*!
 * Checks that all Atom scanner inputs are valid. If not, an error is thrown with a short
 * description of the problem. If all inputs are valid, a data struct containing all the inputs
 * is returned, which is subsequently used to execute the Atom scanner and related functions.
 *
 * @sa executeAtomScanner, AtomScannerInput
 * @param  jsonInput User-defined input options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute Atom scanner
 */
AtomScannerInput checkAtomScannerInput( const rapidjson::Document& jsonInput );

} // namespace d2d

/*!
 * Vallado (2006)
 * Kumar, K., et al. (2014)
 * Kumar, K., et al. (2014) https://github.com/kartikkumar.com/atom
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705. 
 */
