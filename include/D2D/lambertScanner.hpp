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
 * @param jsonInput User-defined input options (extracted from JSON input file)
 */
void executeLambertScanner( const rapidjson::Document& jsonInput );

//! Create Lambert scanner table.
/*!
 * Creates Lambert scanner table in SQLite database used to store results obtaned from running 
 * Lambert scanner. If the database already exists, it is inspected to make sure that the table 
 * schema is correct.
 *
 * @sa executeLambertScanner
 * @param database SQLite database handle
 */
void createLambertScannerTable( SQLite::Database& database );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of debris-to-debris Lambert transfers computed to file. The shortlist is based
 * on the requested number of transfers with the lowest transfer \f$\Delta V\f$, retrieved by 
 * sorting the transfers in the SQLite database.
 *
 * @sa executeLambertScanner, createLambertScannerTable
 * @param database 		  SQLite database handle
 * @param shortlistNumber Number of entries to include in shortlist (if it exceeds number of 
 *						  entries in database table, the whole table is written to file)
 * @param shortlistPath   Path to shortlist file
 */
void writeTransferShortlist( 
	SQLite::Database& database, const int shortlistNumber, const std::string shortlistPath );

//! Lambert scanner input.
/*!
 * Data struct containing all valid Lambert scanner input parameters. This struct is populated by
 * the checkLambertScannerInput() function and can be used to execute the Lambert scanner.
 * 
 * @sa checkLambertScannerInput, executeLambertScanner
 */
struct LambertScannerInput
{
public:

	//! Construct data struct.
	/*!
	 * Constructs data struct based on verified input parameters.
	 *
 	 * @sa checkLambertScannerInput, executeLambertScanner
	 * @param aCatalogPath 			Path to TLE catalog
	 * @param someTleLines 			Number of lines per TLE entry in catalog (2 or 3)
	 * @param aDatabasePath         Path to SQLite database
	 * @param aTimeOfFlightMinimum  Minimum time-of-flight [s]
	 * @param aTimeOfFlightMaximum  Maximum time-of-flight [s]
	 * @param someTimeOfFlightSteps Number of steps to take in time-of-flight grid
	 * @param aTimeOfFlightStepSize Time-of-flight step size (derived parameter) [s]
	 * @param flagPrograde 			Flag indicating if transfer are prograde. False = retrograde
	 * @param aRevolutionsMaximum   Maximum number of revolutions
	 * @param aShortlistNumber      Number of transfers to include in shortlist
	 * @param aShortlistPath        Path to shortlist file
	 */
	LambertScannerInput( const std::string aCatalogPath,
						 const int someTleLines,
						 const std::string aDatabasePath,
						 const double aTimeOfFlightMinimum,
						 const double aTimeOfFlightMaximum,
						 const double someTimeOfFlightSteps,
						 const double aTimeOfFlightStepSize,
						 const bool flagPrograde,
						 const int aRevolutionsMaximum,
						 const int aShortlistNumber,
						 const std::string aShortlistPath )
		: catalogPath( aCatalogPath ),
		  tleLines( someTleLines ),
		  databasePath( aDatabasePath ),
		  timeOfFlightMinimum( aTimeOfFlightMinimum ),
		  timeOfFlightMaximum( aTimeOfFlightMaximum ),
		  timeOfFlightSteps( someTimeOfFlightSteps ),
		  timeOfFlightStepSize( aTimeOfFlightStepSize ),
		  isPrograde( flagPrograde ),
		  revolutionsMaximum( aRevolutionsMaximum ),
		  shortlistNumber( aShortlistNumber ),
		  shortlistPath( aShortlistPath )
	{ }

	//! Path to TLE catalog.
	const std::string catalogPath;

	//! Number of lines per TLE entry in catalog.
	const int tleLines;

	//! Path to SQLite database to store output.
	const std::string databasePath;

	//! Minimum time-of-flight [s].
	const double timeOfFlightMinimum;

	//! Maximum time-of-flight [s].
	const double timeOfFlightMaximum;

	//! Number of time-of-flight steps.
	const double timeOfFlightSteps;

	//! Time-of-flight step size [s].
	const double timeOfFlightStepSize;

	//! Flag indicating if transfers are prograde. False indicates retrograde.
	const bool isPrograde;

	//! Maximum number of revolutions (N) for transfer. Number of revolutions is 2*N+1.
	const int revolutionsMaximum;

	//! Number of entries (lowest transfer \f$\Delta V\f$) to include in shortlist. 
	const int shortlistNumber;

	//! Path to shortlist file.
	const std::string shortlistPath;

protected:

private:
};

//! Check Lambert scanner input parameters.
/*!
 * Checks that all Lambert scanner inputs are valid. If not, an error is thrown with a short 
 * description of the problem. If all inputs are valid, a data struct containing all the inputs
 * is returned, which is subsequently used to execute the Lambert scanner and related functions.
 *
 * @sa executeLambertScanner, LambertScannerInput
 * @param  jsonInput User-defined input options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute Lambert scanner
 */
LambertScannerInput checkLambertScannerInput( const rapidjson::Document& jsonInput );

//! Fetch details of specific debris-to-debris Lambert transfer.
/*!
 * Fetches all data relating to a specific Lambert transfer stored in an the specified SQLite
 * database. The transfer to fetch is specified by its ID in the database. The data retrieved from
 * the database is used to propagate the transfers and a time series of the Cartesian elements of 
 * the departure orbit, arrival orbit, and transfer trajectory is written to an output file.
 *
 * @sa executeLambertScanner, createLambertScannerTable
 * @param input User-defined input options (extracted from JSON input file)
 */
void fetchLambertTransfer( const rapidjson::Document& input );

} // namespace d2d

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in 
 * 	astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth 
 *  International Conf. Astrodynam. Tools and Techniques, ICATT. 2012.
 */
