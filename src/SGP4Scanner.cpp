/*
 * Copyright (c) 2014-2015 Kartik Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <boost/progress.hpp>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/TimeSpan.h>
#include <libsgp4/Tle.h>

#include <Atom/atom.hpp>

#include "D2D/SGP4Scanner.hpp"
#include "D2D/SGP4Tools.hpp"
#include "D2D/tools.hpp"

#include <keplerian_toolbox.h>

namespace d2d
{

//! Execute sgp4_scanner.
void executeSGP4Scanner( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const SGP4ScannerInput input = checkSGP4ScannerInput( config );

    // Set gravitational parameter used [km^3 s^-2].
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " km^3 s^-2" << std::endl;

    // Set mean radius used [km].
    const double earthMeanRadius = kXKMPER;
    std::cout << "Earth mean radius             " << earthMeanRadius << " km" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Open database in read/write mode.
    // N.B.: Database must already exist and contain a populated table called
    //       "lambert_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create sgp4_Scanner_results table in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createSGP4ScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Fetch number of rows in lambert_scanner_results table.
    std::ostringstream lambertScannerTableSizeSelect;
    lambertScannerTableSizeSelect << "SELECT COUNT(*) FROM lambert_scanner_results;";
    const int lambertScannertTableSize
        = database.execAndGet( lambertScannerTableSizeSelect.str( ) );

    // Setup select query to fetch data from lambert_scanner_results table.
    std::ostringstream lambertScannerTableSelect;
    lambertScannerTableSelect << "SELECT * FROM lambert_scanner_results;";

    // Setup insert query to insert data into sgp4_Scanner_results table.
    std::ostringstream sgp4ScannerTableInsert;
    sgp4ScannerTableInsert << "INSERT INTO sgp4_Scanner_results VALUES ("
        << "NULL,"
        << ":lambert_transfer_id,"
        << ":arrival_position_x_error,"
        << ":arrival_position_y_error,"
        << ":arrival_position_z_error,"
        << "arrival_position_error"
        << ");";

    SQLite::Statement lambertScannerTableQuery( database, lambertScannerTableSelect.str( ) );

    std::cout << "Propagating Lambert transfers using SGP4 and populating database ... "
              << std::endl;

    // Loop over rows in lambert_scanner_results table and propagate Lambert transfers using SGP4.
    boost::progress_display showProgress( lambertScannertTableSize );

    while ( lambertScannerTableQuery.executeStep( ) )
    {
        const int      lambertTransferId    = lambertScannerTableQuery.getColumn( 0 );

        const double   departureEpochJulian = lambertScannerTableQuery.getColumn( 3 );
        const double   timeOfFlight         = lambertScannerTableQuery.getColumn( 4 );

        const double   departurePositionX   = lambertScannerTableQuery.getColumn( 7 );
        const double   departurePositionY   = lambertScannerTableQuery.getColumn( 8 );
        const double   departurePositionZ   = lambertScannerTableQuery.getColumn( 9 );
        const double   departureVelocityX   = lambertScannerTableQuery.getColumn( 10 );
        const double   departureVelocityY   = lambertScannerTableQuery.getColumn( 11 );
        const double   departureVelocityZ   = lambertScannerTableQuery.getColumn( 12 );
        const double   departureDeltaVX     = lambertScannerTableQuery.getColumn( 37 );
        const double   departureDeltaVY     = lambertScannerTableQuery.getColumn( 38 );
        const double   departureDeltaVZ     = lambertScannerTableQuery.getColumn( 39 );

        const double   arrivalPositionX     = lambertScannerTableQuery.getColumn( 19 );
        const double   arrivalPositionY     = lambertScannerTableQuery.getColumn( 20 );
        const double   arrivalPositionZ     = lambertScannerTableQuery.getColumn( 21 );
        const double   arrivalVelocityX     = lambertScannerTableQuery.getColumn( 22 );
        const double   arrivalVelocityY     = lambertScannerTableQuery.getColumn( 23 );
        const double   arrivalVelocityZ     = lambertScannerTableQuery.getColumn( 24 );
        const double   arrivalDeltaVX       = lambertScannerTableQuery.getColumn( 40 );
        const double   arrivalDeltaVY       = lambertScannerTableQuery.getColumn( 41 );
        const double   arrivalDeltaVZ       = lambertScannerTableQuery.getColumn( 42 );

        // std::cout << lambertTransferId << ", "
        //           << departureEpochJulian << ", "
        //           << timeOfFlight << ", "
        //           << departurePositionX << ", "
        //           << departurePositionY << ", "
        //           << departurePositionZ << ", "
        //           << departureVelocityX << ", "
        //           << departureVelocityY << ", "
        //           << departureVelocityZ << ", "
        //           << departureDeltaVX << ", "
        //           << departureDeltaVY << ", "
        //           << departureDeltaVZ << ", "
        //           << arrivalPositionX << ", "
        //           << arrivalPositionY << ", "
        //           << arrivalPositionZ << ", "
        //           << arrivalVelocityX << ", "
        //           << arrivalVelocityY << ", "
        //           << arrivalVelocityZ << ", "
        //           << arrivalDeltaVX << ", "
        //           << arrivalDeltaVY << ", "
        //           << arrivalDeltaVZ
        //           << std::endl;

        // // Set up DateTime object for departure epoch using Julian date.
        // // NB: 1721425.5 corresponds to the Gregorian epoch: 0001 Jan 01 00:00:00.0
        // const DateTime departureEpoch( ( departureEpochJulian - 1721425.5 ) * TicksPerDay );

        // Vector6 transferDepartureState;
        // transferDepartureState[ 0 ] = departurePositionX;
        // transferDepartureState[ 1 ] = departurePositionY;
        // transferDepartureState[ 2 ] = departurePositionZ;
        // transferDepartureState[ 3 ] = departureVelocityX + departureDeltaVX;
        // transferDepartureState[ 4 ] = departureVelocityY + departureDeltaVY;
        // transferDepartureState[ 5 ] = departureVelocityZ + departureDeltaVZ;

        // std::string solverStatusSummary = "";
        // int numberOfIterations = 0;
        // const Tle transferDepartureTle
        //     = atom::convertCartesianStateToTwoLineElements< double, Vector6 >(
        //         transferDepartureState,
        //         departureEpoch,
        //         solverStatusSummary,
        //         numberOfIterations,
        //         Tle( ),
        //         earthGravitationalParameter,
        //         earthMeanRadius,
        //         1.0e-1,
        //         1.0e-1,
        //         100 );

// std::cout << solverStatusSummary << std::endl;
    Vector6 cartesianState;
    cartesianState[ 0 ] = -11448.8;
    cartesianState[ 1 ] = -40574.5;
    // cartesianState[ 2 ] = -20546.4;
    cartesianState[ 2 ] = -9.37564;
    cartesianState[ 3 ] = 2.95981;
    cartesianState[ 4 ] = -0.834249;
    // cartesianState[ 5 ] = -1.4634;
    cartesianState[ 5 ] = -0.000399845;

atom::convertCartesianStateToTwoLineElements< double, Vector6 >( cartesianState, DateTime( ) );
exit( 0 );

    //     const SGP4 sgp4( transferDepartureTle );
    //     const Eci tleTransferArrivalState = sgp4.FindPosition( timeOfFlight );
    //     const Vector6 transferArrivalState = getStateVector( tleTransferArrivalState );
    }

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;
}

//! Check sgp4_Scanner input parameters.
SGP4ScannerInput checkSGP4ScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                     " << shortlistPath << std::endl;
    }

    return SGP4ScannerInput( catalogPath,
                                databasePath,
                                shortlistLength,
                                shortlistPath );
}

//! Create sgp4_Scanner table.
void createSGP4ScannerTable( SQLite::Database& database )
{
    // Check that lambert_scanner_results table exists.
    if ( !database.tableExists( "lambert_scanner_results" ) )
    {
        throw std::runtime_error(
            "ERROR: \"lambert_scanner_results\" must exist and be populated!" );
    }

    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS sgp4_Scanner_results;" );

    // Set up SQL command to create table to store SGP4 Scanner results.
    std::ostringstream sgp4ScannerTableCreate;
    sgp4ScannerTableCreate
        << "CREATE TABLE sgp4_Scanner_results ("
        << "\"transfer_id\"                 INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"         INTEGER,"
        << "\"arrival_position_x\"          REAL,"
        << "\"arrival_position_y\"          REAL,"
        << "\"arrival_position_z\"          REAL,"
        << "\"arrival_velocity_x\"          REAL,"
        << "\"arrival_velocity_y\"          REAL,"
        << "\"arrival_velocity_z\"          REAL,"
        << "\"arrival_position_x_error\"    REAL,"
        << "\"arrival_position_y_error\"    REAL,"
        << "\"arrival_position_z_error\"    REAL,"
        << "\"arrival_position_error\"      REAL,"
        << "\"arrival_velocity_x_error\"    REAL,"
        << "\"arrival_velocity_y_error\"    REAL,"
        << "\"arrival_velocity_z_error\"    REAL,"
        << "\"arrival_velocity_error\"      REAL,"
        <<                                  ");";

    // Execute command to create table.
    database.exec( sgp4ScannerTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_position_error column.
    std::ostringstream arrivalPositionErrorIndexCreate;
    arrivalPositionErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_position_error\" on "
                                    << "sgp4_Scanner_results (arrival_position_error ASC);";
    database.exec( arrivalPositionErrorIndexCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_velocity_error column.
    std::ostringstream arrivalVelocityErrorIndexCreate;
    arrivalVelocityErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_velocity_error\" on "
                                    << "sgp4_Scanner_results (arrival_velocity_error ASC);";
    database.exec( arrivalVelocityErrorIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "sgp4_Scanner_results" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'sgp4_Scanner_results' failed! in routine SGP4Scanner.cpp" );
    }
}

} // namespace d2d
