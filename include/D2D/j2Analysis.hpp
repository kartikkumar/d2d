/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_J2_ANALYSIS_HPP
#define D2D_J2_ANALYSIS_HPP

#include <string>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

//! Execute j2_analysis.
/*!
 * Executes j2_analysis application mode. This mode computes the change in the orbital elements of
 * the transfer orbit due to the J2 perturbation for a given time of flight. The osculating elements
 * are then converted back to cartesian state, for the true anomaly of the arrival point in the
 * transfer orbit. Finally, the difference between the converted cartesian state and the arrival
 * state in the transfer orbit computed by the lambert_scanner application mode is computed and
 * stored in a database table by the name j2_analysis_results.
 *
 * This requires that the "lambert_scanner" and "sgp4_scanner" application mode has been executed,
 * which generates a SQLite database containing all transfers computed (stored in
 * "lambert_scanner_results" and "sgp4_scanner_results").
 *
 * This function is called when the user specifies the application mode to be "j2_analysis".
 *
 * @sa executeLambertTransfer, executeSGP4Scanner, executeJ2Analysis
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeJ2Analysis( const rapidjson::Document& config );

//! Input for j2_analysis application mode.
/*!
 * Data struct containing all valid input parameters for the j2_analysis application mode.
 * This struct is populated by the checkJ2AnalysisInput() function and can be used to execute
 * the J2 Analysis.
 *
 * @sa checkJ2AnalysisInput, executeJ2Analysis
 */
struct j2AnalysisInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkJ2AnalysisInput, executeJ2Analysis
     * @param[in] aDatabasePath           Path to SQLite database
     * @param[in] aShortlistLength        Number of transfers to include in shortlist
     * @param[in] aShortlistPath          Path to shortlist file
     */
    j2AnalysisInput( const std::string& aDatabasePath,
                     const int          aShortlistLength,
                     const std::string& aShortlistPath )
        : databasePath( aDatabasePath ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Number of entries
    const int shortlistLength;

    //! Path to shortlist file.
    const std::string shortlistPath;

protected:

private:
};

//! Check j2_analysis input parameters.
/*!
 * Checks that all j2_analysis inputs are valid. If not, an error is thrown with a short
 * description of the problem. If all inputs are valid, a data struct containing all the inputs
 * is returned, which is subsequently used to execute the j2_analysis application mode and
 * related functions.
 *
 * @sa executeJ2Analysis, j2AnalysisInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute J2 Analysis mode
 */
j2AnalysisInput checkJ2AnalysisInput( const rapidjson::Document& config );

//! Create j2_analysis_results table.
/*!
 * Creates j2_analysis_results table in SQLite database used to store results obtained from running
 * the "j2_analysis" application mode.
 *
 * @sa executeJ2Analysis
 * @param[in] database SQLite database handle
 */
void createJ2AnalysisTable( SQLite::Database& database );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of the J2 perturbation analysis on lambert transfer orbits to file. The
 * shortlist is based on the requested number of transfers with the lowest
 * Lambert transfer \f$\Delta V\f$, retrieved by sorting the transfers in the
 * lambert_Scanner_results table in the SQLite database.
 *
 * @sa executeJ2Analysis, createJ2AnalysisTable
 * @param[in] database        SQLite database handle
 * @param[in] shortlistNumber Number of entries to include in shortlist (if it exceeds number of
 *                            entries in database table, the whole table is written to file)
 * @param[in] shortlistPath   Path to shortlist file
 */
void writeJ2TransferShortlist( SQLite::Database& database,
                               const int shortlistNumber,
                               const std::string& shortlistPath );

} // namespace d2d

#endif // D2D_J2_ANALYSIS_HPP
