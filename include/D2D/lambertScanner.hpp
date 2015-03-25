/*
 * Copyright (c) 2014-2015 Kartik Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <string>

#include <keplerian_toolbox.h>

#include <libsgp4/DateTime.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

//! Execute lambert_scanner.
/*!
 * Executes lambert_scanner application mode that performs a grid search to compute \f$\Delta V\f$
 * for debris-to-debris transfers. The transfers are modelled as conic sections. The Lambert
 * targeter employed is based on Izzo (2014), implemented in PyKEP (Izzo, 2012).
 *
 * The results obtained from the grid search are stored in a SQLite database, containing the
 * following table:
 *
 *	- "lambert_scanner_results": contains all Lambert transfers computed during grid search
 *
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeLambertScanner( const rapidjson::Document& config );

//! lambert_scanner input.
/*!
 * Data struct containing all valid lambert_scanner input parameters. This struct is populated by
 * the checkLambertScannerInput() function and can be used to execute the lambert_scanner
 * application mode.
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
     * @param[in] aCatalogPath          Path to TLE catalog
     * @param[in] aDatabasePath         Path to SQLite database
     * @param[in] aDepartureEpoch       Departure epoch for all transfers
     * @param[in] aTimeOfFlightMinimum  Minimum time-of-flight [s]
     * @param[in] aTimeOfFlightMaximum  Maximum time-of-flight [s]
     * @param[in] someTimeOfFlightSteps Number of steps to take in time-of-flight grid
     * @param[in] aTimeOfFlightStepSize Time-of-flight step size (derived parameter) [s]
     * @param[in] progradeFlag          Flag indicating if prograde transfer should be computed
     *                                  (false = retrograde)
     * @param[in] aRevolutionsMaximum   Maximum number of revolutions
     * @param[in] aShortlistLength      Number of transfers to include in shortlist
     * @param[in] aShortlistPath        Path to shortlist file
     */
    LambertScannerInput( const std::string& aCatalogPath,
                         const std::string& aDatabasePath,
                         const DateTime&    aDepartureEpoch,
                         const double       aTimeOfFlightMinimum,
                         const double       aTimeOfFlightMaximum,
                         const double       someTimeOfFlightSteps,
                         const double       aTimeOfFlightStepSize,
                         const bool         progradeFlag,
                         const int          aRevolutionsMaximum,
                         const int          aShortlistLength,
                         const std::string& aShortlistPath )
        : catalogPath( aCatalogPath ),
          databasePath( aDatabasePath ),
          departureEpoch( aDepartureEpoch ),
          timeOfFlightMinimum( aTimeOfFlightMinimum ),
          timeOfFlightMaximum( aTimeOfFlightMaximum ),
          timeOfFlightSteps( someTimeOfFlightSteps ),
          timeOfFlightStepSize( aTimeOfFlightStepSize ),
          isPrograde( progradeFlag ),
          revolutionsMaximum( aRevolutionsMaximum ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Path to TLE catalog.
    const std::string catalogPath;

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Departure epoch.
    const DateTime departureEpoch;

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
    const int shortlistLength;

    //! Path to shortlist file.
    const std::string shortlistPath;

protected:

private:
};

//! Check lambert_scanner input parameters.
/*!
 * Checks that all inputs for the lambert_scanner application mode are valid. If not, an error is
 * thrown with a short description of the problem. If all inputs are valid, a data struct
 * containing all the inputs is returned, which is subsequently used to execute lambert_scanner
 * and related functions.
 *
 * @sa executeLambertScanner, LambertScannerInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute lambert_scanner
 */
LambertScannerInput checkLambertScannerInput( const rapidjson::Document& config );

//! Create lambert_scanner table.
/*!
 * Creates lambert_scanner table in SQLite database used to store results obtaned from running
 * the lambert_scanner application mode.
 *
 * @sa executeLambertScanner
 * @param[in] database SQLite database handle
 */
void createLambertScannerTable( SQLite::Database& database );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of debris-to-debris Lambert transfers to file. The shortlist is based on the
 * requested number of transfers with the lowest transfer \f$\Delta V\f$, retrieved by sorting the
 * transfers in the SQLite database.
 *
 * @sa executeLambertScanner, createLambertScannerTable
 * @param[in] database        SQLite database handle
 * @param[in] shortlistNumber Number of entries to include in shortlist (if it exceeds number of
 *                            entries in database table, the whole table is written to file)
 * @param[in] shortlistPath   Path to shortlist file
 */
void writeTransferShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath );

} // namespace d2d

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in
 * 	astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth
 *  International Conf. Astrodynam. Tools and Techniques, ESA/ESTEC, The Netherlands.
 */
