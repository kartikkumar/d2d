/*
 * Copyright (c) 2014-2015 Kartik Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_SGP4_Scanner_HPP
#define D2D_SGP4_Scanner_HPP

#include <string>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

//! Execute sgp4_Scanner.
/*!
 * Executes sgp4_Scanner application mode that propagates a transfer, computed with the Lambert
 * targeter (Izzo, 2014), using the SGP4/SDP4 models, to compute the mismatch in position due to
 * perturbations. The Lambert targeter employed is based on Izzo (2014), implemented in PyKEP
 * (Izzo, 2012).
 *
 * This requires that the "lambert_scanner" application mode has been executed, which generates a
 * SQLite database containing all transfers computed (stored in "lambert_scanner_results".
 *
 * This function is called when the user specifies the application mode to be "sgp4_Scanner".
 *
 * @sa executeLambertTransfer, executeLambertScanner
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeSGP4Scanner( const rapidjson::Document& config );

//! sgp4_Scanner input.
/*!
 * Data struct containing all valid input parameters for the sgp4_Scanner application mode.
 * This struct is populated by the checkSGP4ScannerInput() function and can be used to execute
 * the SGP4 Scanner.
 *
 * @sa checkSGP4ScannerInput, executeSGP4Scanner
 */
struct SGP4ScannerInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkSGP4ScannerInput, executeSGP4Scanner
     * @param[in] aCatalogPath          Path to TLE catalog
     * @param[in] aDatabasePath         Path to SQLite database
     * @param[in] aShortlistLength      Number of transfers to include in shortlist
     * @param[in] aShortlistPath        Path to shortlist file
     */
    SGP4ScannerInput( const std::string& aCatalogPath,
                         const std::string& aDatabasePath,
                         const int          aShortlistLength,
                         const std::string& aShortlistPath )
        : catalogPath( aCatalogPath ),
          databasePath( aDatabasePath ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Path to TLE catalog.
    const std::string catalogPath;

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Number of entries (small arrival position error \f$\Delta\bar{r}_{arrival}\f$) to
    //! include in shortlist.
    const int shortlistLength;

    //! Path to shortlist file.
    const std::string shortlistPath;

protected:

private:
};

//! Check sgp4_Scanner input parameters.
/*!
 * Checks that all sgp4_Scanner inputs are valid. If not, an error is thrown with a short
 * description of the problem. If all inputs are valid, a data struct containing all the inputs
 * is returned, which is subsequently used to execute the sgp4_Scanner application mode and
 * related functions.
 *
 * @sa executeSGP4Scanner, LambertScannerInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute SGP4 Scanner
 */
SGP4ScannerInput checkSGP4ScannerInput( const rapidjson::Document& config );

//! Create sgp4_Scanner table.
/*!
 * Creates sgp4_Scanner table in SQLite database used to store results obtaned from running the
 * "sgp4_propagate" application mode.
 *
 * @sa executeSGP4Scanner
 * @param[in] database SQLite database handle
 */
void createSGP4ScannerTable( SQLite::Database& database );

} // namespace d2d

#endif // D2D_SGP4_Scanner_HPP
