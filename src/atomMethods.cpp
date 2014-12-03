/*    
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */


#include "D2D/atomScanner.hpp"
 
namespace d2d
{

//! Execute Atom scanner.
void executeAtomScanner( const rapidjson::Document& jsonInput )
{
	// Verify input parameters.
	const AtomScannerInput input = checkAtomScannerInput( jsonInput );
}

//! Execute SGP4 forward propagation.
void executeSGP4ForwardPropagation( const rapidjson::Document& jsonInput )
{

}

//! Check Atom scanner input parameters.
AtomScannerInput checkAtomScannerInput( const rapidjson::Document& jsonInput )
{

}

} // namespace d2d