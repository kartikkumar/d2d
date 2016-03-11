/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_LAMBERT_FETCH_HPP
#define D2D_LAMBERT_FETCH_HPP

#include <string>

#include <rapidjson/document.h>

namespace d2d
{

//! Fetch details of specific debris-to-debris Lambert transfer.
/*!
 * Fetches all data relating to a specific Lambert transfer stored in the specified SQLite
 * database. The transfer to fetch is specified by its ID in the "lambert_scanner" table.
 *
 * The data retrieved from the database is used to propagate the transfers and a time series of the
 * Cartesian elements of the departure orbit, arrival orbit, and transfer trajectory is written to
 * an output file.
 *
 * This function is executed if the user provides "lambert_fetch" as the application mode.
 *
 * @sa executeLambertScanner, createLambertScannerTable
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void fetchLambertTransfer( const rapidjson::Document& config );

//! Input for lambert_fetch application mode.
/*!
 * Data struct containing all valid input parameters to fetch a Lambert transfer from an SQLite
 * database, execute the transfer, and write the output to file. This struct is populated by the
 * checkLambertFetchInput() function.
 *
 * @sa checkLambertFetchInput, fetchLambertTransfer
 */
struct LambertFetchInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkLambertFetchInput, fetchLambertTransfer
     * @param[in] aDatabasePath                 Path to SQLite database
     * @param[in] aTransferId                   Transfer ID that identifies transfer to fetch
     * @param[in] numberOfOutputSteps           Number of time steps to generate for output files
     * @param[in] anOutputDirectory             Path to output directory to write output files to
     *                                          (relative or absolute)
     * @param[in] aMetadataFilename             Output filename for simulation metadata
     * @param[in] aDepartureOrbitFilename       Output filename for sampled departure orbit
     * @param[in] aDeparturePathFilename        Output filename for sampled path of departure
     *                                          object
     * @param[in] anArrivalOrbitFilename        Output filename for sampled arrival orbit
     * @param[in] anArrivalPathFilename         Output filename for sampled path of arrival object
     * @param[in] aTransferOrbitFilename        Output filename for sampled transfer orbit
     * @param[in] aTransferPathFilename         Output filename for sampled transfer path
     */
    LambertFetchInput( const std::string& aDatabasePath,
                       const int          aTransferId,
                       const int          numberOfOutputSteps,
                       const std::string& anOutputDirectory,
                       const std::string& aMetadataFilename,
                       const std::string& aDepartureOrbitFilename,
                       const std::string& aDeparturePathFilename,
                       const std::string& anArrivalOrbitFilename,
                       const std::string& anArrivalPathFilename,
                       const std::string& aTransferOrbitFilename,
                       const std::string& aTransferPathFilename )
        : databasePath( aDatabasePath ),
          transferId( aTransferId ),
          outputSteps( numberOfOutputSteps ),
          outputDirectory( anOutputDirectory ),
          metadataFilename( aMetadataFilename ),
          departureOrbitFilename( aDepartureOrbitFilename ),
          departurePathFilename( aDeparturePathFilename ),
          arrivalOrbitFilename( anArrivalOrbitFilename ),
          arrivalPathFilename( anArrivalPathFilename ),
          transferOrbitFilename( aTransferOrbitFilename ),
          transferPathFilename( aTransferPathFilename )
    { }

    //! Path to SQLite database with transfer data.
    const std::string databasePath;

    //! Transfer ID.
    const int transferId;

    //! Number of time steps to generate for output.
    const int outputSteps;

    //! Path to output directory (relative or absolute).
    const std::string outputDirectory;

    //! Simulation metadata output filename.
    const std::string metadataFilename;

    // Output filename for sampled departure orbit.
    const std::string departureOrbitFilename;

    // Output filename for sampled path of departure object.
    const std::string departurePathFilename;

    // Output filename for sampled arrival orbit.
    const std::string arrivalOrbitFilename;

    // Output filename for sampled path of arrival object.
    const std::string arrivalPathFilename;

    // Output filename for sampled transfer orbit.
    const std::string transferOrbitFilename;

    // Output filename for sampled transfer path.
    const std::string transferPathFilename;

protected:

private:
};

//! Check input parameters for lambert_fetch.
/*!
 * Checks that all inputs to fetch a Lambert transfer from the specified SQLite database is valid.
 * If not, an error is thrown with a short description of the problem. If all inputs are valid, a
 * data struct containing all the inputs is returned.
 *
 * @sa fetchLambertTransfer, LambertTransferFetch
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input for lambert_fetch application mode
 */
LambertFetchInput checkLambertFetchInput( const rapidjson::Document& config );

} // namespace d2d

#endif // D2D_LAMBERT_FETCH_HPP
