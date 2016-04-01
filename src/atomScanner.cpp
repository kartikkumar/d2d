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
    lambertScannerTableSizeSelect << "SELECT COUNT(*) FROM lambert_scanner_results;";
    const int lambertScannertTableSize
        = database.execAndGet( lambertScannerTableSizeSelect.str( ) );

    // Set up select query to fetch data from lambert_scanner_results table.
    std::ostringstream lambertScannerTableSelect;
    lambertScannerTableSelect << "SELECT * FROM lambert_scanner_results;";

    SQLite::Statement lambertQuery( database, lambertScannerTableSelect.str( ) );

    // Setup insert query.
    std::ostringstream atomScannerTableInsert;
    atomScannerTableInsert
        << "INSERT INTO atom_scanner_results VALUES ("
        << "NULL,"
        << ":departure_object_id,"
        << ":arrival_object_id,"
        << ":departure_epoch,"
        << ":time_of_flight,"
        << ":revolutions,"
        << ":prograde,"
        << ":departure_position_x,"
        << ":departure_position_y,"
        << ":departure_position_z,"
        << ":departure_velocity_x,"
        << ":departure_velocity_y,"
        << ":departure_velocity_z,"
        << ":departure_semi_major_axis,"
        << ":departure_eccentricity,"
        << ":departure_inclination,"
        << ":departure_argument_of_periapsis,"
        << ":departure_longitude_of_ascending_node,"
        << ":departure_true_anomaly,"
        << ":arrival_position_x,"
        << ":arrival_position_y,"
        << ":arrival_position_z,"
        << ":arrival_velocity_x,"
        << ":arrival_velocity_y,"
        << ":arrival_velocity_z,"
        << ":arrival_semi_major_axis,"
        << ":arrival_eccentricity,"
        << ":arrival_inclination,"
        << ":arrival_argument_of_periapsis,"
        << ":arrival_longitude_of_ascending_node,"
        << ":arrival_true_anomaly,"
        << ":transfer_semi_major_axis,"
        << ":transfer_eccentricity,"
        << ":transfer_inclination,"
        << ":transfer_argument_of_periapsis,"
        << ":transfer_longitude_of_ascending_node,"
        << ":transfer_true_anomaly,"
        << ":departure_delta_v_x,"
        << ":departure_delta_v_y,"
        << ":departure_delta_v_z,"
        << ":arrival_delta_v_x,"
        << ":arrival_delta_v_y,"
        << ":arrival_delta_v_z,"
        << ":transfer_delta_v"
        << ");";

    SQLite::Statement atomQuery( database, atomScannerTableInsert.str( ) );

    std::cout << "Computing Atom transfers and populating database ... " << std::endl;

    boost::progress_display showProgress( lambertScannertTableSize );


    while ( lambertQuery.executeStep( ) )
    {
        const int      lambertTransferId                    = lambertQuery.getColumn( 0 );
        const int      departureObjectId                    = lambertQuery.getColumn( 1 );
        const int      arrivalObjectId                      = lambertQuery.getColumn( 2 );

        const double   departureEpochJulian                 = lambertQuery.getColumn( 3 );
        const double   timeOfFlight                         = lambertQuery.getColumn( 4 );

        const double   departurePositionX                   = lambertQuery.getColumn( 7 );
        const double   departurePositionY                   = lambertQuery.getColumn( 8 );
        const double   departurePositionZ                   = lambertQuery.getColumn( 9 );
        // const double   departureVelocityX                   = lambertQuery.getColumn( 10 );
        // const double   departureVelocityY                   = lambertQuery.getColumn( 11 );
        // const double   departureVelocityZ                   = lambertQuery.getColumn( 12 );

        const double   arrivalPositionX                     = lambertQuery.getColumn( 19 );
        const double   arrivalPositionY                     = lambertQuery.getColumn( 20 );
        const double   arrivalPositionZ                     = lambertQuery.getColumn( 21 );
        // const double   arrivalVelocityX                     = lambertQuery.getColumn( 22 );
        // const double   arrivalVelocityY                     = lambertQuery.getColumn( 23 );
        // const double   arrivalVelocityZ                     = lambertQuery.getColumn( 24 );

        const double   departureDeltaVX                     = lambertQuery.getColumn( 37 );
        const double   departureDeltaVY                     = lambertQuery.getColumn( 38 );
        const double   departureDeltaVZ                     = lambertQuery.getColumn( 39 );
        // const double   lambertArrivalDeltaVX                = lambertQuery.getColumn( 40 );
        // const double   lambertArrivalDeltaVY                = lambertQuery.getColumn( 41 );
        // const double   lambertArrivalDeltaVZ                = lambertQuery.getColumn( 42 );

        const double   lambertTotalDeltaV                   = lambertQuery.getColumn( 43 );
        // std::cout << "enne is baas" << std::endl;

        // Filter out cases using transfer deltaV cut off given through input file.
        if ( lambertTotalDeltaV > input.transferDeltaVCutoff )
        {
            // Bind zeroes to atomQuery.
            std::string bindZeroes = bindZeroesAtomScannerTable( lambertTransferId );
            SQLite::Statement zeroQuery( database, bindZeroes );
            zeroQuery.executeStep( );
            zeroQuery.reset( );

            ++showProgress;
            continue;
        }


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

        Vector3 arrivalPosition;
        arrivalPosition[ astro::xPositionIndex ] = arrivalPositionX;
        arrivalPosition[ astro::yPositionIndex ] = arrivalPositionY;
        arrivalPosition[ astro::zPositionIndex ] = arrivalPositionZ;

        Vector3 departureVelocityGuess;
        departureVelocityGuess[ 0 ] = departureDeltaVX;
        departureVelocityGuess[ 1 ] = departureDeltaVY;
        departureVelocityGuess[ 2 ] = departureDeltaVZ;


        // // Create virtual TLE for the transfer object's orbit from its departure state.
        // // This TLE will be propagated using the SGP4 transfer.
        // Tle transferTle;
        // std::string solverStatusSummary;
        // int numberOfIterations;
        // const Tle referenceTle = Tle( );
        // const int maximumIterations = 100;

        // try
        // {
        //     transferTle = atom::convertCartesianStateToTwoLineElements< double, Vector6 >(
        //         transferDepartureState,
        //         departureEpoch,
        //         solverStatusSummary,
        //         numberOfIterations,
        //         referenceTle,
        //         earthGravitationalParameter,
        //         earthMeanRadius,
        //         input.absoluteTolerance,
        //         input.relativeTolerance,
        //         maximumIterations );
        // }
        // catch( std::exception& virtualTleError )
        // {
        //     // At the moment we just catch the exceptions that are thrown internally and proceed.
        //     // @todo: Figure out how to handle and register these exceptions.
        // }

        // // Check if transferTle is correct.
        // const SGP4 sgp4Check( transferTle );
        // bool testPassed = false;

        // Eci propagatedStateEci = sgp4Check.FindPosition( 0.0 );
        // Vector6 propagatedState;
        // propagatedState[ astro::xPositionIndex ] = propagatedStateEci.Position( ).x;
        // propagatedState[ astro::yPositionIndex ] = propagatedStateEci.Position( ).y;
        // propagatedState[ astro::zPositionIndex ] = propagatedStateEci.Position( ).z;
        // propagatedState[ astro::xVelocityIndex ] = propagatedStateEci.Velocity( ).x;
        // propagatedState[ astro::yVelocityIndex ] = propagatedStateEci.Velocity( ).y;
        // propagatedState[ astro::zVelocityIndex ] = propagatedStateEci.Velocity( ).z;

        // Vector6 trueDepartureState;
        // for ( int i = 0; i < 6; i++ )
        // {
        //     trueDepartureState[ i ] = transferDepartureState[ i ];
        // }

        // testPassed = executeVirtualTleConvergenceTest( propagatedState,
        //                                                trueDepartureState,
        //                                                input.relativeTolerance,
        //                                                input.absoluteTolerance );

        // if ( testPassed == false )
        // {
        //     // Bind zeroes to atomQuery.
        //     std::string bindZeroes = bindZeroesSGP4ScannerTable( lambertTransferId );
        //     SQLite::Statement zeroQuery( database, bindZeroes );
        //     zeroQuery.executeStep( );
        //     zeroQuery.reset( );

        //     ++virtualTleFailCounter;
        //     ++showProgress;
        //     continue;
        // }

        // // Propagate transfer object using the SGP4 propagator.
        // const SGP4 sgp4( transferTle );
        // DateTime sgp4ArrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
        // Vector eciPosition = Vector( );
        // Vector eciVelocity = Vector( );
        // Eci sgp4ArrivalStateEci( sgp4ArrivalEpoch, eciPosition, eciVelocity );
        // try
        // {
        //     sgp4ArrivalStateEci = sgp4.FindPosition( sgp4ArrivalEpoch );
        // }
        // catch( std::exception& sgp4PropagationError )
        // {
        //     // Bind zeroes to atomQuery.
        //     std::string bindZeroes = bindZeroesSGP4ScannerTable( lambertTransferId );
        //     SQLite::Statement zeroQuery( database, bindZeroes );
        //     zeroQuery.executeStep( );
        //     zeroQuery.reset( );

        //     ++arrivalEpochPropagationFailCounter;
        //     ++showProgress;
        //     continue;
        // }

        // const Vector6 sgp4ArrivalState = getStateVector( sgp4ArrivalStateEci );

        // // Compute the required results.
        // const double sgp4ArrivalPositionX = sgp4ArrivalState[ astro::xPositionIndex ];
        // const double sgp4ArrivalPositionY = sgp4ArrivalState[ astro::yPositionIndex ];
        // const double sgp4ArrivalPositionZ = sgp4ArrivalState[ astro::zPositionIndex ];

        // const double sgp4ArrivalVelocityX = sgp4ArrivalState[ astro::xVelocityIndex ];
        // const double sgp4ArrivalVelocityY = sgp4ArrivalState[ astro::yVelocityIndex ];
        // const double sgp4ArrivalVelocityZ = sgp4ArrivalState[ astro::zVelocityIndex ];

        // const double arrivalPositionErrorX = sgp4ArrivalPositionX - lambertArrivalPositionX;
        // const double arrivalPositionErrorY = sgp4ArrivalPositionY - lambertArrivalPositionY;
        // const double arrivalPositionErrorZ = sgp4ArrivalPositionZ - lambertArrivalPositionZ;

        // Vector3 positionError;
        // positionError[ astro::xPositionIndex ] = arrivalPositionErrorX;
        // positionError[ astro::yPositionIndex ] = arrivalPositionErrorY;
        // positionError[ astro::zPositionIndex ] = arrivalPositionErrorZ;
        // const double arrivalPositionErrorNorm = sml::norm< double >( positionError );

        // const double arrivalVelocityErrorX = sgp4ArrivalVelocityX -
        //                                     ( lambertArrivalVelocityX - lambertArrivalDeltaVX );
        // const double arrivalVelocityErrorY = sgp4ArrivalVelocityY -
        //                                     ( lambertArrivalVelocityY - lambertArrivalDeltaVY );
        // const double arrivalVelocityErrorZ = sgp4ArrivalVelocityZ -
        //                                     ( lambertArrivalVelocityZ - lambertArrivalDeltaVZ );

        // Vector3 velocityError;
        // velocityError[ 0 ] = arrivalVelocityErrorX;
        // velocityError[ 1 ] = arrivalVelocityErrorY;
        // velocityError[ 2 ] = arrivalVelocityErrorZ;
        // double arrivalVelocityErrorNorm = sml::norm< double >( velocityError );
        // int lambertTransferId = 37;
        // Bind computed values to atomQuery.
        atomQuery.bind( ":departure_object_id",                   lambertTransferId );
        // atomQuery.bind( ":arrival_position_x",                    sgp4ArrivalPositionX );
        // atomQuery.bind( ":arrival_position_y",                    sgp4ArrivalPositionY );
        // atomQuery.bind( ":arrival_position_z",                    sgp4ArrivalPositionZ );
        // atomQuery.bind( ":arrival_velocity_x",                    sgp4ArrivalVelocityX );
        // atomQuery.bind( ":arrival_velocity_y",                    sgp4ArrivalVelocityY );
        // atomQuery.bind( ":arrival_velocity_z",                    sgp4ArrivalVelocityZ );
        // atomQuery.bind( ":arrival_position_x_error",              arrivalPositionErrorX );
        // atomQuery.bind( ":arrival_position_y_error",              arrivalPositionErrorY );
        // atomQuery.bind( ":arrival_position_z_error",              arrivalPositionErrorZ );
        // atomQuery.bind( ":arrival_position_error",                arrivalPositionErrorNorm );
        // atomQuery.bind( ":arrival_velocity_x_error",              arrivalVelocityErrorX );
        // atomQuery.bind( ":arrival_velocity_y_error",              arrivalVelocityErrorY );
        // atomQuery.bind( ":arrival_velocity_z_error",              arrivalVelocityErrorZ );
        // atomQuery.bind( ":arrival_velocity_error",                arrivalVelocityErrorNorm );
        // atomQuery.bind( ":success",                               1 );

        atomQuery.executeStep( );
        atomQuery.reset( );

        ++showProgress;
    }

    // Fetch number of rows in sgp4_scanner_results table.
    // std::ostringstream sgp4ScannerTableSizeSelect;
    // sgp4ScannerTableSizeSelect << "SELECT COUNT(*) FROM sgp4_scanner_results;";
    // const int sgp4ScannertTableSize
    //     = database.execAndGet( sgp4ScannerTableSizeSelect.str( ) );

    // std::ostringstream totalLambertCasesConsideredSelect;
    // totalLambertCasesConsideredSelect
    //     << "SELECT COUNT(*) FROM lambert_scanner_results WHERE transfer_delta_v <= "
    //     << input.transferDeltaVCutoff << ";";
    // const int totalLambertCasesConsidered
    //     = database.execAndGet( totalLambertCasesConsideredSelect.str( ) );

    // std::cout << std::endl;
    // std::cout << "Total Lambert cases = " << lambertScannertTableSize << std::endl;
    // std::cout << "Total SGP4 cases = " << sgp4ScannertTableSize << std::endl;
    // std::cout << std::endl;
    // std::cout << "Number of Lambert cases considered with the transfer deltaV cut-off = "
    //           << totalLambertCasesConsidered << std::endl;
    // std::cout << "Number of virtual TLE convergence fail cases = "
    //           << virtualTleFailCounter << std::endl;
    // std::cout << "Number of arrival epoch propagation fail cases = "
    //           << arrivalEpochPropagationFailCounter << std::endl;

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
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
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"departure_object_id\"                     TEXT,"
        << "\"arrival_object_id\"                       TEXT,"
        << "\"departure_epoch\"                         REAL,"
        << "\"time_of_flight\"                          REAL,"
        << "\"revolutions\"                             INTEGER,"
        // N.B.: SQLite doesn't support booleans so 0 = false, 1 = true for 'prograde'
        << "\"prograde\"                                INTEGER,"
        << "\"departure_position_x\"                    REAL,"
        << "\"departure_position_y\"                    REAL,"
        << "\"departure_position_z\"                    REAL,"
        << "\"departure_velocity_x\"                    REAL,"
        << "\"departure_velocity_y\"                    REAL,"
        << "\"departure_velocity_z\"                    REAL,"
        << "\"departure_semi_major_axis\"               REAL,"
        << "\"departure_eccentricity\"                  REAL,"
        << "\"departure_inclination\"                   REAL,"
        << "\"departure_argument_of_periapsis\"         REAL,"
        << "\"departure_longitude_of_ascending_node\"   REAL,"
        << "\"departure_true_anomaly\"                  REAL,"
        << "\"arrival_position_x\"                      REAL,"
        << "\"arrival_position_y\"                      REAL,"
        << "\"arrival_position_z\"                      REAL,"
        << "\"arrival_velocity_x\"                      REAL,"
        << "\"arrival_velocity_y\"                      REAL,"
        << "\"arrival_velocity_z\"                      REAL,"
        << "\"arrival_semi_major_axis\"                 REAL,"
        << "\"arrival_eccentricity\"                    REAL,"
        << "\"arrival_inclination\"                     REAL,"
        << "\"arrival_argument_of_periapsis\"           REAL,"
        << "\"arrival_longitude_of_ascending_node\"     REAL,"
        << "\"arrival_true_anomaly\"                    REAL,"
        << "\"transfer_semi_major_axis\"                REAL,"
        << "\"transfer_eccentricity\"                   REAL,"
        << "\"transfer_inclination\"                    REAL,"
        << "\"transfer_argument_of_periapsis\"          REAL,"
        << "\"transfer_longitude_of_ascending_node\"    REAL,"
        << "\"transfer_true_anomaly\"                   REAL,"
        << "\"departure_delta_v_x\"                     REAL,"
        << "\"departure_delta_v_y\"                     REAL,"
        << "\"departure_delta_v_z\"                     REAL,"
        << "\"arrival_delta_v_x\"                       REAL,"
        << "\"arrival_delta_v_y\"                       REAL,"
        << "\"arrival_delta_v_z\"                       REAL,"
        << "\"transfer_delta_v\"                        REAL"
        <<                                              ");";

    // Execute command to create table.
    database.exec( atomScannerTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
                              << "atom_scanner_results (transfer_delta_v ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "atom_scanner_results" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'atom_scanner_results' failed!" );
    }
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

//! Write transfer shortlist to file.
void writeAtomTransferShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath )
{
    // Fetch transfers to include in shortlist.
    // Database table is sorted by transfer_delta_v, in ascending order.
    std::ostringstream shortlistSelect;
    shortlistSelect << "SELECT * FROM atom_scanner_results ORDER BY transfer_delta_v ASC LIMIT "
                    << shortlistNumber << ";";
    SQLite::Statement query( database, shortlistSelect.str( ) );

    // Write fetch data to file.
    std::ofstream shortlistFile( shortlistPath.c_str( ) );

    // Print file header.
    shortlistFile << "transfer_id,"
                  << "departure_object_id,"
                  << "arrival_object_id,"
                  << "departure_epoch,"
                  << "time_of_flight,"
                  << "revolutions,"
                  << "prograde,"
                  << "departure_position_x,"
                  << "departure_position_y,"
                  << "departure_position_z,"
                  << "departure_velocity_x,"
                  << "departure_velocity_y,"
                  << "departure_velocity_z,"
                  << "departure_semi_major_axis,"
                  << "departure_eccentricity,"
                  << "departure_inclination,"
                  << "departure_argument_of_periapsis,"
                  << "departure_longitude_of_ascending_node,"
                  << "departure_true_anomaly,"
                  << "arrival_position_x,"
                  << "arrival_position_y,"
                  << "arrival_position_z,"
                  << "arrival_velocity_x,"
                  << "arrival_velocity_y,"
                  << "arrival_velocity_z,"
                  << "arrival_semi_major_axis,"
                  << "arrival_eccentricity,"
                  << "arrival_inclination,"
                  << "arrival_argument_of_periapsis,"
                  << "arrival_longitude_of_ascending_node,"
                  << "arrival_true_anomaly,"
                  << "transfer_semi_major_axis,"
                  << "transfer_eccentricity,"
                  << "transfer_inclination,"
                  << "transfer_argument_of_periapsis,"
                  << "transfer_longitude_of_ascending_node,"
                  << "transfer_true_anomaly,"
                  << "departure_delta_v_x,"
                  << "departure_delta_v_y,"
                  << "departure_delta_v_z,"
                  << "arrival_delta_v_x,"
                  << "arrival_delta_v_y,"
                  << "arrival_delta_v_z,"
                  << "transfer_delta_v"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int    transferId                         = query.getColumn( 0 );
        const int    departureObjectId                  = query.getColumn( 1 );
        const int    arrivalObjectId                    = query.getColumn( 2 );
        const double departureEpoch                     = query.getColumn( 3 );
        const double timeOfFlight                       = query.getColumn( 4 );
        const int    revolutions                        = query.getColumn( 5 );
        const int    prograde                           = query.getColumn( 6 );
        const double departurePositionX                 = query.getColumn( 7 );
        const double departurePositionY                 = query.getColumn( 8 );
        const double departurePositionZ                 = query.getColumn( 9 );
        const double departureVelocityX                 = query.getColumn( 10 );
        const double departureVelocityY                 = query.getColumn( 11 );
        const double departureVelocityZ                 = query.getColumn( 12 );
        const double departureSemiMajorAxis             = query.getColumn( 13 );
        const double departureEccentricity              = query.getColumn( 14 );
        const double departureInclination               = query.getColumn( 15 );
        const double departureArgumentOfPeriapsis       = query.getColumn( 16 );
        const double departureLongitudeOfAscendingNode  = query.getColumn( 17 );
        const double departureTrueAnomaly               = query.getColumn( 18 );
        const double arrivalPositionX                   = query.getColumn( 19 );
        const double arrivalPositionY                   = query.getColumn( 20 );
        const double arrivalPositionZ                   = query.getColumn( 21 );
        const double arrivalVelocityX                   = query.getColumn( 22 );
        const double arrivalVelocityY                   = query.getColumn( 23 );
        const double arrivalVelocityZ                   = query.getColumn( 24 );
        const double arrivalSemiMajorAxis               = query.getColumn( 25 );
        const double arrivalEccentricity                = query.getColumn( 26 );
        const double arrivalInclination                 = query.getColumn( 27 );
        const double arrivalArgumentOfPeriapsis         = query.getColumn( 28 );
        const double arrivalLongitudeOfAscendingNode    = query.getColumn( 29 );
        const double arrivalTrueAnomaly                 = query.getColumn( 30 );
        const double transferSemiMajorAxis              = query.getColumn( 31 );
        const double transferEccentricity               = query.getColumn( 32 );
        const double transferInclination                = query.getColumn( 33 );
        const double transferArgumentOfPeriapsis        = query.getColumn( 34 );
        const double transferLongitudeOfAscendingNode   = query.getColumn( 35 );
        const double transferTrueAnomaly                = query.getColumn( 36 );
        const double departureDeltaVX                   = query.getColumn( 37 );
        const double departureDeltaVY                   = query.getColumn( 38 );
        const double departureDeltaVZ                   = query.getColumn( 39 );
        const double arrivalDeltaVX                     = query.getColumn( 40 );
        const double arrivalDeltaVY                     = query.getColumn( 41 );
        const double arrivalDeltaVZ                     = query.getColumn( 42 );
        const double transferDeltaV                     = query.getColumn( 43 );

        shortlistFile << transferId                         << ","
                      << departureObjectId                  << ","
                      << arrivalObjectId                    << ","
                      << departureEpoch                     << ","
                      << timeOfFlight                       << ","
                      << revolutions                        << ","
                      << prograde                           << ","
                      << departurePositionX                 << ","
                      << departurePositionY                 << ","
                      << departurePositionZ                 << ","
                      << departureVelocityX                 << ","
                      << departureVelocityY                 << ","
                      << departureVelocityZ                 << ","
                      << departureSemiMajorAxis             << ","
                      << departureEccentricity              << ","
                      << departureInclination               << ","
                      << departureArgumentOfPeriapsis       << ","
                      << departureLongitudeOfAscendingNode  << ","
                      << departureTrueAnomaly               << ","
                      << arrivalPositionX                   << ","
                      << arrivalPositionY                   << ","
                      << arrivalPositionZ                   << ","
                      << arrivalVelocityX                   << ","
                      << arrivalVelocityY                   << ","
                      << arrivalVelocityZ                   << ","
                      << arrivalSemiMajorAxis               << ","
                      << arrivalEccentricity                << ","
                      << arrivalInclination                 << ","
                      << arrivalArgumentOfPeriapsis         << ","
                      << arrivalLongitudeOfAscendingNode    << ","
                      << arrivalTrueAnomaly                 << ","
                      << transferSemiMajorAxis              << ","
                      << transferEccentricity               << ","
                      << transferInclination                << ","
                      << transferArgumentOfPeriapsis        << ","
                      << transferLongitudeOfAscendingNode   << ","
                      << transferTrueAnomaly                << ","
                      << departureDeltaVX                   << ","
                      << departureDeltaVY                   << ","
                      << departureDeltaVZ                   << ","
                      << arrivalDeltaVX                     << ","
                      << arrivalDeltaVY                     << ","
                      << arrivalDeltaVZ                     << ","
                      << transferDeltaV
                      << std::endl;
    }

    shortlistFile.close( );
}

} // namespace d2d

// // Create sgp4_scanner_results table in SQLite database.
//     std::cout << "Creating SQLite database table if needed ... " << std::endl;
//     createSGP4ScannerTable( database );
//     std::cout << "SQLite database set up successfully!" << std::endl;




//     // Open database in read/write mode.
//     SQLite::Database database( input.databasePath.c_str( ),
//                                SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );
// // Parse catalog and store TLE objects.
    // std::ifstream catalogFile( input.catalogPath.c_str( ) );
    // std::string catalogLine;

    // // Check if catalog is 2-line or 3-line version.
    // std::getline( catalogFile, catalogLine );
    // const int tleLines = getTleCatalogType( catalogLine );

    // // Reset file stream to start of file.
    // catalogFile.seekg( 0, std::ios::beg );

    // typedef std::vector< std::string > TleStrings;
    // typedef std::vector< Tle > TleObjects;
    // TleObjects tleObjects;

    // while ( std::getline( catalogFile, catalogLine ) )
    // {
    //     TleStrings tleStrings;
    //     removeNewline( catalogLine );
    //     tleStrings.push_back( catalogLine );
    //     std::getline( catalogFile, catalogLine );
    //     removeNewline( catalogLine );
    //     tleStrings.push_back( catalogLine );

    //     if ( tleLines == 3 )
    //     {
    //         std::getline( catalogFile, catalogLine );
    //         removeNewline( catalogLine );
    //         tleStrings.push_back( catalogLine );
    //         tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ], tleStrings[ 2 ] ) );
    //     }

    //     else if ( tleLines == 2 )
    //     {
    //         tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ] ) );
    //     }
    // }

    // catalogFile.close( );
    // std::cout << tleObjects.size( ) << " TLE objects parsed from catalog!" << std::endl;



        // // Compute departure state.
        // Tle departureObject = tleObjects[ i ];
        // SGP4 sgp4Departure( departureObject );

        // DateTime departureEpoch = input.departureEpoch;
        // if ( input.departureEpoch == DateTime( ) )
        // {
        //     departureEpoch = departureObject.Epoch( );
        // }

        // const Eci tleDepartureState = sgp4Departure.FindPosition( departureEpoch );
        // const Vector6 departureState = getStateVector( tleDepartureState );

        // Vector3 departurePosition;
        // std::copy( departureState.begin( ),
        //            departureState.begin( ) + 3,
        //            departurePosition.begin( ) );

        // Vector3 departureVelocity;
        // std::copy( departureState.begin( ) + 3,
        //            departureState.end( ),
        //            departureVelocity.begin( ) );

        // const Vector6 departureStateKepler
        //     = astro::convertCartesianToKeplerianElements( departureState,
        //                                                   earthGravitationalParameter );
        // const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );

        // // Loop over arrival objects.
        // for ( unsigned int j = 0; j < tleObjects.size( ); j++ )
        // {
        //     // Skip the case of the departure and arrival objects being the same.
        //     if ( i == j )
        //     {
        //         continue;
        //     }

        //     Tle arrivalObject = tleObjects[ j ];
        //     SGP4 sgp4Arrival( arrivalObject );
        //     const int arrivalObjectId = static_cast< int >( arrivalObject.NoradNumber( ) );

        //     // Loop over time-of-flight grid.
        //     for ( int k = 0; k < input.timeOfFlightSteps; k++ )
        //     {
        //         const double timeOfFlight
        //             = input.timeOfFlightMinimum + k * input.timeOfFlightStepSize;

        //         const DateTime arrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
        //         const Eci tleArrivalState   = sgp4Arrival.FindPosition( arrivalEpoch );
        //         const Vector6 arrivalState  = getStateVector( tleArrivalState );

        //         Vector3 arrivalPosition;
        //         std::copy( arrivalState.begin( ),
        //                    arrivalState.begin( ) + 3,
        //                    arrivalPosition.begin( ) );

        //         Vector3 arrivalVelocity;
        //         std::copy( arrivalState.begin( ) + 3,
        //                    arrivalState.end( ),
        //                    arrivalVelocity.begin( ) );
        //         const Vector6 arrivalStateKepler
        //             = astro::convertCartesianToKeplerianElements( arrivalState,
        //                                                           earthGravitationalParameter );

        //         kep_toolbox::lambert_problem targeter( departurePosition,
        //                                                arrivalPosition,
        //                                                timeOfFlight,
        //                                                earthGravitationalParameter,
        //                                                !input.isPrograde,
        //                                                input.revolutionsMaximum );

        //         const int numberOfSolutions = targeter.get_v1( ).size( );

        //         // Compute Delta-Vs for transfer and determine index of lowest.
        //         typedef std::vector< Vector3 > VelocityList;
        //         VelocityList departureDeltaVs( numberOfSolutions );
        //         VelocityList arrivalDeltaVs( numberOfSolutions );

        //         typedef std::vector< double > TransferDeltaVList;
        //         TransferDeltaVList transferDeltaVs( numberOfSolutions );

        //         for ( int i = 0; i < numberOfSolutions; i++ )
        //         {
        //             // Compute Delta-V for transfer.
        //             const Vector3 transferDepartureVelocity = targeter.get_v1( )[ i ];
        //             const Vector3 transferArrivalVelocity = targeter.get_v2( )[ i ];

        //             departureDeltaVs[ i ] = sml::add( transferDepartureVelocity,
        //                                               sml::multiply( departureVelocity, -1.0 ) );
        //             arrivalDeltaVs[ i ]   = sml::add( transferArrivalVelocity,
        //                                               sml::multiply( arrivalVelocity, -1.0 ) );

        //             transferDeltaVs[ i ]
        //                 = sml::norm< double >( departureDeltaVs[ i ] )
        //                     + sml::norm< double >( arrivalDeltaVs[ i ] );
        //         }

        //         const TransferDeltaVList::iterator minimumDeltaVIterator
        //             = std::min_element( transferDeltaVs.begin( ), transferDeltaVs.end( ) );
        //         const int minimumDeltaVIndex
        //             = std::distance( transferDeltaVs.begin( ), minimumDeltaVIterator );

        //         const int revolutions = std::floor( ( minimumDeltaVIndex + 1 ) / 2 );

        //         Vector6 transferState;
        //         std::copy( departurePosition.begin( ),
        //                    departurePosition.begin( ) + 3,
        //                    transferState.begin( ) );
        //         std::copy( targeter.get_v1( )[ minimumDeltaVIndex ].begin( ),
        //                    targeter.get_v1( )[ minimumDeltaVIndex ].begin( ) + 3,
        //                    transferState.begin( ) + 3 );

        //         const Vector6 transferStateKepler
        //             = astro::convertCartesianToKeplerianElements( transferState,
        //                                                           earthGravitationalParameter );

        //         // Bind values to SQL insert query.
        //         atomQuery.bind( ":departure_object_id",  departureObjectId );
        //         atomQuery.bind( ":arrival_object_id",    arrivalObjectId );
        //         atomQuery.bind( ":departure_epoch",      departureEpoch.ToJulian( ) );
        //         atomQuery.bind( ":time_of_flight",       timeOfFlight );
        //         atomQuery.bind( ":revolutions",          revolutions );
        //         atomQuery.bind( ":prograde",             input.isPrograde );
        //         atomQuery.bind( ":departure_position_x", departureState[ astro::xPositionIndex ] );
        //         atomQuery.bind( ":departure_position_y", departureState[ astro::yPositionIndex ] );
        //         atomQuery.bind( ":departure_position_z", departureState[ astro::zPositionIndex ] );
        //         atomQuery.bind( ":departure_velocity_x", departureState[ astro::xVelocityIndex ] );
        //         atomQuery.bind( ":departure_velocity_y", departureState[ astro::yVelocityIndex ] );
        //         atomQuery.bind( ":departure_velocity_z", departureState[ astro::zVelocityIndex ] );
        //         atomQuery.bind( ":departure_semi_major_axis",
        //             departureStateKepler[ astro::semiMajorAxisIndex ] );
        //         atomQuery.bind( ":departure_eccentricity",
        //             departureStateKepler[ astro::eccentricityIndex ] );
        //         atomQuery.bind( ":departure_inclination",
        //             departureStateKepler[ astro::inclinationIndex ] );
        //         atomQuery.bind( ":departure_argument_of_periapsis",
        //             departureStateKepler[ astro::argumentOfPeriapsisIndex ] );
        //         atomQuery.bind( ":departure_longitude_of_ascending_node",
        //             departureStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
        //         atomQuery.bind( ":departure_true_anomaly",
        //             departureStateKepler[ astro::trueAnomalyIndex ] );
        //         atomQuery.bind( ":arrival_position_x",  arrivalState[ astro::xPositionIndex ] );
        //         atomQuery.bind( ":arrival_position_y",  arrivalState[ astro::yPositionIndex ] );
        //         atomQuery.bind( ":arrival_position_z",  arrivalState[ astro::zPositionIndex ] );
        //         atomQuery.bind( ":arrival_velocity_x",  arrivalState[ astro::xVelocityIndex ] );
        //         atomQuery.bind( ":arrival_velocity_y",  arrivalState[ astro::yVelocityIndex ] );
        //         atomQuery.bind( ":arrival_velocity_z",  arrivalState[ astro::zVelocityIndex ] );
        //         atomQuery.bind( ":arrival_semi_major_axis",
        //             arrivalStateKepler[ astro::semiMajorAxisIndex ] );
        //         atomQuery.bind( ":arrival_eccentricity",
        //             arrivalStateKepler[ astro::eccentricityIndex ] );
        //         atomQuery.bind( ":arrival_inclination",
        //             arrivalStateKepler[ astro::inclinationIndex ] );
        //         atomQuery.bind( ":arrival_argument_of_periapsis",
        //             arrivalStateKepler[ astro::argumentOfPeriapsisIndex ] );
        //         atomQuery.bind( ":arrival_longitude_of_ascending_node",
        //             arrivalStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
        //         atomQuery.bind( ":arrival_true_anomaly",
        //             arrivalStateKepler[ astro::trueAnomalyIndex ] );
        //         atomQuery.bind( ":transfer_semi_major_axis",
        //             transferStateKepler[ astro::semiMajorAxisIndex ] );
        //         atomQuery.bind( ":transfer_eccentricity",
        //             transferStateKepler[ astro::eccentricityIndex ] );
        //         atomQuery.bind( ":transfer_inclination",
        //             transferStateKepler[ astro::inclinationIndex ] );
        //         atomQuery.bind( ":transfer_argument_of_periapsis",
        //             transferStateKepler[ astro::argumentOfPeriapsisIndex ] );
        //         atomQuery.bind( ":transfer_longitude_of_ascending_node",
        //             transferStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
        //         atomQuery.bind( ":transfer_true_anomaly",
        //             transferStateKepler[ astro::trueAnomalyIndex ] );
        //         atomQuery.bind( ":departure_delta_v_x", departureDeltaVs[ minimumDeltaVIndex ][ 0 ] );
        //         atomQuery.bind( ":departure_delta_v_y", departureDeltaVs[ minimumDeltaVIndex ][ 1 ] );
        //         atomQuery.bind( ":departure_delta_v_z", departureDeltaVs[ minimumDeltaVIndex ][ 2 ] );
        //         atomQuery.bind( ":arrival_delta_v_x",   arrivalDeltaVs[ minimumDeltaVIndex ][ 0 ] );
        //         atomQuery.bind( ":arrival_delta_v_y",   arrivalDeltaVs[ minimumDeltaVIndex ][ 1 ] );
        //         atomQuery.bind( ":arrival_delta_v_z",   arrivalDeltaVs[ minimumDeltaVIndex ][ 2 ] );
        //         atomQuery.bind( ":transfer_delta_v",    *minimumDeltaVIterator );

        //         // Execute insert query.
        //         atomQuery.executeStep( );

        //         // Reset SQL insert query.
        //         atomQuery.reset( );
        //     }
        // }


