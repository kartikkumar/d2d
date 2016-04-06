/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2016 Enne Hekma (ennehekma@gmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/progress.hpp>

#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <SML/sml.hpp>

#include <Atom/atom.hpp>

#include <Astro/astro.hpp>

#include "D2D/atomScanner.hpp"
#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

namespace d2d
{

// typedef double Real;
typedef std::pair< Vector3, Vector3 > Velocities;


//! Execute atom_scanner.
void executeAtomScanner( const rapidjson::Document& config )
{
    std::cout.precision( 15 );

    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const AtomScannerInput input = checkAtomScannerInput( config );

    // Set gravitational parameter used by Atom targeter.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s^-2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Open database in read/write mode.
    // N.B.: Database must already exist and contain a populated table called
    //       "lambert_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create table for atom_scanner_results in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createAtomScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Fetch number of rows in lambert_scanner_results table.
    std::ostringstream lambertScannerTableSizeSelect;
    lambertScannerTableSizeSelect << "SELECT COUNT(*) FROM sgp4_scanner_results INNER JOIN lambert_scanner_results ON lambert_scanner_results.transfer_id = sgp4_scanner_results.lambert_transfer_id WHERE sgp4_scanner_results.success=1;";
    const int atomScannertTableSize
        = database.execAndGet( lambertScannerTableSizeSelect.str( ) );
    std::cout << "Cases to process: " << atomScannertTableSize << std::endl;


    // Set up select query to fetch data from lambert_scanner_results table.
    std::ostringstream lambertScannerTableSelect;
    // lambertScannerTableSelect << "SELECT * FROM lambert_scanner_results;";
    lambertScannerTableSelect << "SELECT * FROM sgp4_scanner_results INNER JOIN lambert_scanner_results ON lambert_scanner_results.transfer_id = sgp4_scanner_results.lambert_transfer_id WHERE sgp4_scanner_results.success=1;";

    SQLite::Statement lambertQuery( database, lambertScannerTableSelect.str( ) );

    // Setup insert query.
    std::ostringstream atomScannerTableInsert;
    atomScannerTableInsert
        << "INSERT INTO atom_scanner_results VALUES ("
        << "NULL,"
        << ":lambert_transfer_id,"
        << ":atom_departure_delta_v_x,"
        << ":atom_departure_delta_v_y,"
        << ":atom_departure_delta_v_z,"
        << ":atom_arrival_delta_v_x,"
        << ":atom_arrival_delta_v_y,"
        << ":atom_arrival_delta_v_z,"
        << ":atom_transfer_delta_v"
        << ");";


    SQLite::Statement atomQuery( database, atomScannerTableInsert.str( ) );

    std::cout << "Computing Atom transfers and populating database ... " << std::endl;

    boost::progress_display showProgress( atomScannertTableSize );

    int failCounter = 0;


    while ( lambertQuery.executeStep( ) )
    {
        const int      lambertTransferId                    = lambertQuery.getColumn( 1 );
        // const int      departureObjectId                    = lambertQuery.getColumn( 1 );
        // const int      arrivalObjectId                      = lambertQuery.getColumn( 2 );

        const double   departureEpochJulian                 = lambertQuery.getColumn( 20 );
        const double   timeOfFlight                         = lambertQuery.getColumn( 21 );

        const double   departurePositionX                   = lambertQuery.getColumn( 24 );
        const double   departurePositionY                   = lambertQuery.getColumn( 25 );
        const double   departurePositionZ                   = lambertQuery.getColumn( 26 );
        const double   departureVelocityX                   = lambertQuery.getColumn( 27 );
        const double   departureVelocityY                   = lambertQuery.getColumn( 28 );
        const double   departureVelocityZ                   = lambertQuery.getColumn( 29 );

        const double   arrivalPositionX                     = lambertQuery.getColumn( 36 );
        const double   arrivalPositionY                     = lambertQuery.getColumn( 37 );
        const double   arrivalPositionZ                     = lambertQuery.getColumn( 38 );
        const double   arrivalVelocityX                     = lambertQuery.getColumn( 39 );
        const double   arrivalVelocityY                     = lambertQuery.getColumn( 40 );
        const double   arrivalVelocityZ                     = lambertQuery.getColumn( 41 );

        const double   departureDeltaVX                     = lambertQuery.getColumn( 54 );
        const double   departureDeltaVY                     = lambertQuery.getColumn( 55 );
        const double   departureDeltaVZ                     = lambertQuery.getColumn( 56 );

        const double   lambertTotalDeltaV                   = lambertQuery.getColumn( 60 );

        // Set up DateTime object for departure epoch using Julian date.
        // Note: The transformation given in the following statement is based on how the DateTime
        //       class internally handles date transformations.
        DateTime departureEpoch( ( departureEpochJulian
                                   - astro::ASTRO_GREGORIAN_EPOCH_IN_JULIAN_DAYS ) * TicksPerDay );

        // Get departure state for the transfer object.
        Vector3 departurePosition;
        departurePosition[ astro::xPositionIndex ] = departurePositionX;
        departurePosition[ astro::yPositionIndex ] = departurePositionY;
        departurePosition[ astro::zPositionIndex ] = departurePositionZ;

        Vector3 departureVelocity;
        departureVelocity[ astro::xPositionIndex ] = departureVelocityX;
        departureVelocity[ astro::yPositionIndex ] = departureVelocityY;
        departureVelocity[ astro::zPositionIndex ] = departureVelocityZ;

        Vector3 arrivalPosition;
        arrivalPosition[ astro::xPositionIndex ] = arrivalPositionX;
        arrivalPosition[ astro::yPositionIndex ] = arrivalPositionY;
        arrivalPosition[ astro::zPositionIndex ] = arrivalPositionZ;

        Vector3 arrivalVelocity;
        arrivalVelocity[ astro::xPositionIndex ] = arrivalVelocityX;
        arrivalVelocity[ astro::yPositionIndex ] = arrivalVelocityY;
        arrivalVelocity[ astro::zPositionIndex ] = arrivalVelocityZ;

        Vector3 departureVelocityGuess;
        departureVelocityGuess[ 0 ] = departureDeltaVX + departureVelocityX;
        departureVelocityGuess[ 1 ] = departureDeltaVY + departureVelocityY;
        departureVelocityGuess[ 2 ] = departureDeltaVZ + departureVelocityZ;
        // std::cout << "departureVelocityGuess" << departureVelocityGuess << std::endl;

        std::string dummyString = "";
        int numberOfIterations = 0;
        try
        {

            const Velocities velocities = atom::executeAtomSolver( departurePosition,
                                                                   departureEpoch,
                                                                   arrivalPosition,
                                                                   timeOfFlight,
                                                                   departureVelocityGuess,
                                                                   dummyString,
                                                                   numberOfIterations );

            Vector3 atom_departure_delta_v = sml::add( velocities.first,
                                                          sml::multiply( departureVelocity, -1.0 ) );
            Vector3 atom_arrival_delta_v = sml::add( arrivalVelocity,
                                                          sml::multiply( velocities.second, -1.0 ) );

            double atom_transfer_delta = sml::norm< double > (atom_departure_delta_v)
                                                + sml::norm< double > (atom_arrival_delta_v);



            // std::cout << "Velocities: " << velocities.first << std::endl;
            atomQuery.bind( ":lambert_transfer_id" ,              lambertTransferId );
            atomQuery.bind( ":atom_departure_delta_v_x" ,         atom_departure_delta_v[0] );
            atomQuery.bind( ":atom_departure_delta_v_y" ,         atom_departure_delta_v[1] );
            atomQuery.bind( ":atom_departure_delta_v_z" ,         atom_departure_delta_v[2] );
            atomQuery.bind( ":atom_arrival_delta_v_x" ,           atom_arrival_delta_v[0] );
            atomQuery.bind( ":atom_arrival_delta_v_y" ,           atom_arrival_delta_v[1] );
            atomQuery.bind( ":atom_arrival_delta_v_z" ,           atom_arrival_delta_v[2] );
            atomQuery.bind( ":atom_transfer_delta_v" ,            atom_transfer_delta );

            atomQuery.executeStep( );
            atomQuery.reset( );

        }
        catch( std::exception& virtualTleError )
        {
            // std::cout << "lambertTransferId: " << lambertTransferId << std::endl;
            // std::cout << "dummyString: " << dummyString << std::endl;
            // std::cout << virtualTleError.what() << std::endl;
            failCounter = failCounter +1;

        }

        ++showProgress;
    }

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Total cases: " << atomScannertTableSize << std::endl;
    std::cout << "Failed cases: " << failCounter << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;

    // Check if shortlist file should be created; call function to write output.
    if ( input.shortlistLength > 0 )
    {
        std::cout << "Writing shortlist to file ... " << std::endl;
        writeAtomTransferShortlist( database, input.shortlistLength, input.shortlistPath );
        std::cout << "Shortlist file created successfully!" << std::endl;
    }
}

//! Check atom_scanner input parameters.
AtomScannerInput checkAtomScannerInput( const rapidjson::Document& config )
{
    const double transferDeltaVCutoff
        = find( config, "transfer_deltav_cutoff" )->value.GetDouble( );
    std::cout << "Transfer deltaV cut-off         " << transferDeltaVCutoff << std::endl;

    const double relativeTolerance = find( config, "relative_tolerance" )->value.GetDouble( );
    std::cout << "Relative tolerance              " << relativeTolerance << std::endl;

    const double absoluteTolerance = find( config, "absolute_tolerance" )->value.GetDouble( );
    std::cout << "Absolute tolerance              " << absoluteTolerance << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                        " << databasePath << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers        " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                       " << shortlistPath << std::endl;
    }

    return AtomScannerInput(  transferDeltaVCutoff,
                             relativeTolerance,
                             absoluteTolerance,
                             databasePath,
                             shortlistLength,
                             shortlistPath );
}

//! Create atom_scanner table.
void createAtomScannerTable( SQLite::Database& database )
{
    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS atom_scanner_results;" );

    // Set up SQL command to create table to store atom_scanner results.
    std::ostringstream atomScannerTableCreate;
    atomScannerTableCreate
        << "CREATE TABLE atom_scanner_results ("
        << "\"atom_transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"                          INT,"
        << "\"atom_departure_delta_v_x\"                     REAL,"
        << "\"atom_departure_delta_v_y\"                     REAL,"
        << "\"atom_departure_delta_v_z\"                     REAL,"
        << "\"atom_arrival_delta_v_x\"                       REAL,"
        << "\"atom_arrival_delta_v_y\"                       REAL,"
        << "\"atom_arrival_delta_v_z\"                       REAL,"
        << "\"atom_transfer_delta_v\"                        REAL"
        <<                                              ");";

    // Execute command to create table.
    database.exec( atomScannerTableCreate.str( ).c_str( ) );

    if ( !database.tableExists( "atom_scanner_results" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'atom_scanner_results' failed!" );
    }
}

//! Write transfer shortlist to file.
void writeAtomTransferShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath )
{
    // Fetch transfers to include in shortlist.
    // Database table is sorted by transfer_delta_v, in ascending order.
    std::ostringstream shortlistSelect;
    shortlistSelect << "SELECT * FROM atom_scanner_results ORDER BY atom_transfer_delta_v ASC LIMIT "
                    << shortlistNumber << ";";
    SQLite::Statement query( database, shortlistSelect.str( ) );

    // Write fetch data to file.
    std::ofstream shortlistFile( shortlistPath.c_str( ) );

    // Print file header.
    shortlistFile << "transfer_id,"
                  << "lambert_transfer_id,"
                  << "atom_departure_delta_v_x,"
                  << "atom_departure_delta_v_y,"
                  << "atom_departure_delta_v_z,"
                  << "atom_arrival_delta_v_x,"
                  << "atom_arrival_delta_v_y,"
                  << "atom_arrival_delta_v_z,"
                  << "atom_transfer_delta_v"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int    atomTransferId                   = query.getColumn( 0 );
        const int    lambertTransferId                = query.getColumn( 1 );
        const double atomDepartureDeltaVX               = query.getColumn( 2 );
        const double atomDepartureDeltaVY               = query.getColumn( 3 );
        const double atomDepartureDeltaVZ               = query.getColumn( 4 );
        const double atomArrivalDeltaVX                 = query.getColumn( 5 );
        const double atomArrivalDeltaVY                 = query.getColumn( 6 );
        const double atomArrivalDeltaVZ                 = query.getColumn( 7 );
        const double atomTransferDeltaV                 = query.getColumn( 8 );

        shortlistFile << atomTransferId           << ","
                      << lambertTransferId        << ","
                      << atomDepartureDeltaVX       << ","
                      << atomDepartureDeltaVY       << ","
                      << atomDepartureDeltaVZ       << ","
                      << atomArrivalDeltaVX         << ","
                      << atomArrivalDeltaVY         << ","
                      << atomArrivalDeltaVZ         << ","
                      << atomTransferDeltaV         << ","
    }

    shortlistFile.close( );
}

//! Bind zeroes into sgp4_scanner_results table.
std::string bindZeroesAtomScannerTable( const int lambertTransferId )
{
    std::ostringstream zeroEntry;
    zeroEntry << "INSERT INTO atom_scanner_results VALUES ("
              << "NULL"                          << ","
              << lambertTransferId               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0                               << ","
              << 0
              << ");";

    return zeroEntry.str( );
}

} // namespace d2d
