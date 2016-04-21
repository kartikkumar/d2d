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
#include <limits>
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

    // Set gravitational parameter used by Atom solver.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s^-2" << std::endl;

    // Set mean radius used [km].
    const double earthMeanRadius = kXKMPER;
    std::cout << "Earth mean radius               " << earthMeanRadius << " km" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Open database in read/write mode.
    // N.B.: Database must already exist and contain two populated table called
    //       "lambert_scanner_results" and "sgp4_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create table called atom_scanner_results in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createAtomScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Fetch number of rows in spg4_scanner_results table.
    std::ostringstream sgp4ScannerTableSizeSelect;
    sgp4ScannerTableSizeSelect << "SELECT COUNT(*) "
                               << "FROM sgp4_scanner_results "
                               << "WHERE success=1;";
    const int atomScannerTableSize
        = database.execAndGet( sgp4ScannerTableSizeSelect.str( ) );
    std::cout << "Cases to process: " << atomScannerTableSize << std::endl;

    // Set up select query to fetch data from lambert and sgp4 scanner tables.
    std::ostringstream lambertSGP4ScannerTableSelect;
    lambertSGP4ScannerTableSelect << "SELECT        * "
                                  << "FROM          sgp4_scanner_results "
                                  << "INNER JOIN    lambert_scanner_results "
                                  << "ON            lambert_scanner_results.transfer_id "
                                  << "              = sgp4_scanner_results.lambert_transfer_id "
                                  << "WHERE         sgp4_scanner_results.success=1;";

    SQLite::Statement lambertSGP4Query( database, lambertSGP4ScannerTableSelect.str( ) );

    // Setup insert query.
    std::ostringstream atomScannerTableInsert;
    atomScannerTableInsert << "INSERT INTO atom_scanner_results VALUES ("
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
    boost::progress_display showProgress( atomScannerTableSize );

    int failCounter = 0;

    while ( lambertSGP4Query.executeStep( ) )
    {
        const int      lambertTransferId                    = lambertSGP4Query.getColumn( 1 );

        const double   departureEpochJulian                 = lambertSGP4Query.getColumn( 20 );
        const double   timeOfFlight                         = lambertSGP4Query.getColumn( 21 );

        const double   departurePositionX                   = lambertSGP4Query.getColumn( 24 );
        const double   departurePositionY                   = lambertSGP4Query.getColumn( 25 );
        const double   departurePositionZ                   = lambertSGP4Query.getColumn( 26 );
        const double   departureVelocityX                   = lambertSGP4Query.getColumn( 27 );
        const double   departureVelocityY                   = lambertSGP4Query.getColumn( 28 );
        const double   departureVelocityZ                   = lambertSGP4Query.getColumn( 29 );

        const double   arrivalPositionX                     = lambertSGP4Query.getColumn( 36 );
        const double   arrivalPositionY                     = lambertSGP4Query.getColumn( 37 );
        const double   arrivalPositionZ                     = lambertSGP4Query.getColumn( 38 );
        const double   arrivalVelocityX                     = lambertSGP4Query.getColumn( 39 );
        const double   arrivalVelocityY                     = lambertSGP4Query.getColumn( 40 );
        const double   arrivalVelocityZ                     = lambertSGP4Query.getColumn( 41 );

        const double   departureDeltaVX                     = lambertSGP4Query.getColumn( 54 );
        const double   departureDeltaVY                     = lambertSGP4Query.getColumn( 55 );
        const double   departureDeltaVZ                     = lambertSGP4Query.getColumn( 56 );

        // Set up DateTime object for departure epoch using Julian date.
        // Note: The transformation given in the following statement is based on how the DateTime
        //       class internally handles date transformations.
        const DateTime departureEpoch( ( departureEpochJulian
                                   - astro::ASTRO_GREGORIAN_EPOCH_IN_JULIAN_DAYS ) * TicksPerDay );

        Vector3 departurePosition;
        departurePosition[ astro::xPositionIndex ] = departurePositionX;
        departurePosition[ astro::yPositionIndex ] = departurePositionY;
        departurePosition[ astro::zPositionIndex ] = departurePositionZ;

        Vector3 departureVelocity;
        departureVelocity[ 0 ] = departureVelocityX;
        departureVelocity[ 1 ] = departureVelocityY;
        departureVelocity[ 2 ] = departureVelocityZ;

        Vector3 arrivalPosition;
        arrivalPosition[ astro::xPositionIndex ] = arrivalPositionX;
        arrivalPosition[ astro::yPositionIndex ] = arrivalPositionY;
        arrivalPosition[ astro::zPositionIndex ] = arrivalPositionZ;

        Vector3 arrivalVelocity;
        arrivalVelocity[ 0 ] = arrivalVelocityX;
        arrivalVelocity[ 1 ] = arrivalVelocityY;
        arrivalVelocity[ 2 ] = arrivalVelocityZ;

        Vector3 departureVelocityGuess;
        departureVelocityGuess[ 0 ] = departureDeltaVX + departureVelocityX;
        departureVelocityGuess[ 1 ] = departureDeltaVY + departureVelocityY;
        departureVelocityGuess[ 2 ] = departureDeltaVZ + departureVelocityZ;

        std::string solverStatusSummary;
        int numberOfIterations = 0;
        Tle referenceTle = Tle( );

        try
        {
            const Velocities velocities = atom::executeAtomSolver( departurePosition,
                                                                   departureEpoch,
                                                                   arrivalPosition,
                                                                   timeOfFlight,
                                                                   departureVelocityGuess,
                                                                   solverStatusSummary,
                                                                   numberOfIterations,
                                                                   referenceTle,
                                                                   earthGravitationalParameter,
                                                                   earthMeanRadius,
                                                                   input.absoluteTolerance,
                                                                   input.relativeTolerance,
                                                                   input.maxIterations );

            const Vector3 atomDepartureDeltaV =
                            sml::add( velocities.first, sml::multiply( departureVelocity, -1.0 ) );
            const Vector3 atomArrivalDeltaV =
                            sml::add( arrivalVelocity, sml::multiply( velocities.second, -1.0 ) );

            const double atomTransferDeltaV = sml::norm< double > ( atomDepartureDeltaV )
                                                + sml::norm< double > ( atomArrivalDeltaV );

            atomQuery.bind( ":lambert_transfer_id",              lambertTransferId );
            atomQuery.bind( ":atom_departure_delta_v_x",         atomDepartureDeltaV[ 0 ] );
            atomQuery.bind( ":atom_departure_delta_v_y",         atomDepartureDeltaV[ 1 ] );
            atomQuery.bind( ":atom_departure_delta_v_z",         atomDepartureDeltaV[ 2 ] );
            atomQuery.bind( ":atom_arrival_delta_v_x",           atomArrivalDeltaV[ 0 ] );
            atomQuery.bind( ":atom_arrival_delta_v_y",           atomArrivalDeltaV[ 1 ] );
            atomQuery.bind( ":atom_arrival_delta_v_z",           atomArrivalDeltaV[ 2 ] );
            atomQuery.bind( ":atom_transfer_delta_v",            atomTransferDeltaV );

            atomQuery.executeStep( );
            atomQuery.reset( );
        }
        catch( std::exception& atomSolverError )
        {
            failCounter = failCounter + 1;
        }
        ++showProgress;
    }

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Total cases: " << atomScannerTableSize << std::endl;
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
    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                        " << databasePath << std::endl;

    const double relativeTolerance = find( config, "relative_tolerance" )->value.GetDouble( );
    std::cout << "Relative tolerance              " << relativeTolerance << std::endl;

    const double absoluteTolerance = find( config, "absolute_tolerance" )->value.GetDouble( );
    std::cout << "Absolute tolerance              " << absoluteTolerance << std::endl;

    const int maxIterations = find( config, "maximum_iterations" )->value.GetInt( );
    std::cout << "Maximum iterations Atom solver  " << maxIterations << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers        " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                       " << shortlistPath << std::endl;
    }

    return AtomScannerInput( relativeTolerance,
                             absoluteTolerance,
                             databasePath,
                             maxIterations,
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
        << "\"transfer_id\"                                  INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"                          INT,"
        << "\"atom_departure_delta_v_x\"                     REAL,"
        << "\"atom_departure_delta_v_y\"                     REAL,"
        << "\"atom_departure_delta_v_z\"                     REAL,"
        << "\"atom_arrival_delta_v_x\"                       REAL,"
        << "\"atom_arrival_delta_v_y\"                       REAL,"
        << "\"atom_arrival_delta_v_z\"                       REAL,"
        << "\"atom_transfer_delta_v\"                        REAL"
        <<                                                   ");";

    // Execute command to create table.
    database.exec( atomScannerTableCreate.str( ).c_str( ) );

    if ( !database.tableExists( "atom_scanner_results" ) )
    {
        throw std::runtime_error( "ERROR: 'atom_scanner_results' table could not be created!" );
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
    shortlistSelect << "SELECT      * "
                    << "FROM        atom_scanner_results "
                    << "ORDER BY    atom_transfer_delta_v "
                    << "ASC "
                    << "LIMIT "
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
        const int    atomTransferId                     = query.getColumn( 0 );
        const int    lambertTransferId                  = query.getColumn( 1 );
        const double atomDepartureDeltaVX               = query.getColumn( 2 );
        const double atomDepartureDeltaVY               = query.getColumn( 3 );
        const double atomDepartureDeltaVZ               = query.getColumn( 4 );
        const double atomArrivalDeltaVX                 = query.getColumn( 5 );
        const double atomArrivalDeltaVY                 = query.getColumn( 6 );
        const double atomArrivalDeltaVZ                 = query.getColumn( 7 );
        const double atomTransferDeltaV                 = query.getColumn( 8 );

        shortlistFile << atomTransferId             << ","
                      << lambertTransferId          << ",";

        shortlistFile << std::setprecision( std::numeric_limits< double >::digits10 )
                      << atomDepartureDeltaVX       << ","
                      << atomDepartureDeltaVY       << ","
                      << atomDepartureDeltaVZ       << ","
                      << atomArrivalDeltaVX         << ","
                      << atomArrivalDeltaVY         << ","
                      << atomArrivalDeltaVZ         << ","
                      << atomTransferDeltaV
                      << std::endl;
    }

    shortlistFile.close( );
}

} // namespace d2d
