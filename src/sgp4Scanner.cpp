/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>


#include <boost/progress.hpp>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/TimeSpan.h>
#include <libsgp4/Tle.h>
#include <libsgp4/Vector.h>

#include <Atom/atom.hpp>

#include <Astro/astro.hpp>

#include "D2D/sgp4Scanner.hpp"
#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

namespace d2d
{

//! Execute sgp4_scanner.
void executeSGP4Scanner( const rapidjson::Document& config )
{
    std::cout.precision( 15 );

    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const sgp4ScannerInput input = checkSGP4ScannerInput( config );

    // Set gravitational parameter used [km^3 s^-2].
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter   " << earthGravitationalParameter
              << " km^3 s^-2" << std::endl;

    // Set mean radius used [km].
    const double earthMeanRadius = kXKMPER;
    std::cout << "Earth mean radius               " << earthMeanRadius << " km" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Open database in read/write mode.
    // N.B.: Database must already exist and contain a populated table called
    //       "lambert_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create sgp4_scanner_results table in SQLite database.
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

    // Setup insert query to insert data into sgp4_scanner_results table.
    std::ostringstream sgp4ScannerTableInsert;
    sgp4ScannerTableInsert << "INSERT INTO sgp4_scanner_results VALUES ("
        << "NULL,"
        << ":lambert_transfer_id,"
        << ":departure_object_id,"
        << ":arrival_object_id,"
        << ":departure_epoch,"
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
        << ":arrival_position_x_error,"
        << ":arrival_position_y_error,"
        << ":arrival_position_z_error,"
        << ":arrival_position_error,"
        << ":arrival_velocity_x_error,"
        << ":arrival_velocity_y_error,"
        << ":arrival_velocity_z_error,"
        << ":arrival_velocity_error,"
        << ":success"
        << ");";

    SQLite::Statement lambertQuery( database, lambertScannerTableSelect.str( ) );

    SQLite::Statement sgp4Query( database, sgp4ScannerTableInsert.str( ) );

    std::cout << "Propagating Lambert transfers using SGP4 and populating database ... "
              << std::endl;

    // Loop over rows in lambert_scanner_results table and propagate Lambert transfers using SGP4.
    boost::progress_display showProgress( lambertScannertTableSize );

    // Declare counters for different fail cases.
    int virtualTleFailCounter = 0;
    int arrivalEpochPropagationFailCounter = 0;

    while ( lambertQuery.executeStep( ) )
    {
        const int      lambertTransferId                    = lambertQuery.getColumn( 0 );
        const int      departureObjectId                    = lambertQuery.getColumn( 1 );
        const int      arrivalObjectId                      = lambertQuery.getColumn( 2 );

        const double   departureEpochJulian                 = lambertQuery.getColumn( 3 );
        const double   timeOfFlight                         = lambertQuery.getColumn( 4 );

        const double   departureSemiMajorAxis               = lambertQuery.getColumn( 13 );
        const double   departureEccentricity                = lambertQuery.getColumn( 14 );
        const double   departureInclination                 = lambertQuery.getColumn( 15 );
        const double   departureArgumentOfPeriapsis         = lambertQuery.getColumn( 16 );
        const double   departureLongitudeAscendingNode      = lambertQuery.getColumn( 17 );
        const double   departureTrueAnomaly                 = lambertQuery.getColumn( 18 );

        const double   departurePositionX                   = lambertQuery.getColumn( 7 );
        const double   departurePositionY                   = lambertQuery.getColumn( 8 );
        const double   departurePositionZ                   = lambertQuery.getColumn( 9 );
        const double   departureVelocityX                   = lambertQuery.getColumn( 10 );
        const double   departureVelocityY                   = lambertQuery.getColumn( 11 );
        const double   departureVelocityZ                   = lambertQuery.getColumn( 12 );
        const double   departureDeltaVX                     = lambertQuery.getColumn( 37 );
        const double   departureDeltaVY                     = lambertQuery.getColumn( 38 );
        const double   departureDeltaVZ                     = lambertQuery.getColumn( 39 );

        const double   lambertArrivalPositionX              = lambertQuery.getColumn( 19 );
        const double   lambertArrivalPositionY              = lambertQuery.getColumn( 20 );
        const double   lambertArrivalPositionZ              = lambertQuery.getColumn( 21 );
        const double   lambertArrivalVelocityX              = lambertQuery.getColumn( 22 );
        const double   lambertArrivalVelocityY              = lambertQuery.getColumn( 23 );
        const double   lambertArrivalVelocityZ              = lambertQuery.getColumn( 24 );
        const double   lambertArrivalDeltaVX                = lambertQuery.getColumn( 40 );
        const double   lambertArrivalDeltaVY                = lambertQuery.getColumn( 41 );
        const double   lambertArrivalDeltaVZ                = lambertQuery.getColumn( 42 );

        const double   lambertTotalDeltaV                   = lambertQuery.getColumn( 43 );

        // Set up DateTime object for departure epoch using Julian date.
        DateTime departureEpoch( ( departureEpochJulian
                                   - astro::ASTRO_GREGORIAN_EPOCH_IN_JULIAN_DAYS ) * TicksPerDay );

        // Get departure state for the transfer object.
        Vector6 transferDepartureState;
        transferDepartureState[ astro::xPositionIndex ] = departurePositionX;
        transferDepartureState[ astro::yPositionIndex ] = departurePositionY;
        transferDepartureState[ astro::zPositionIndex ] = departurePositionZ;
        transferDepartureState[ astro::xVelocityIndex ] = departureVelocityX + departureDeltaVX;
        transferDepartureState[ astro::yVelocityIndex ] = departureVelocityY + departureDeltaVY;
        transferDepartureState[ astro::zVelocityIndex ] = departureVelocityZ + departureDeltaVZ;

        // Calculate transfer object's velocity norm at the departure point.
        Vector3 transferDepartureVelocity;
        for ( int i = 0; i < 3; i++ )
        {
            transferDepartureVelocity[ i ] = transferDepartureState[ i + 3 ];
        }
        double transferDepartureVelocityNorm = sml::norm< double >( transferDepartureVelocity );

        // Filter out cases using transfer deltaV cut off given through input file.
        if ( lambertTotalDeltaV > input.transferDeltaVCutoff )
        {
            // Bind zeroes to SQL insert sgp4Query.
            std::string bindZeroes = bindZeroesSGP4ScannerTable( lambertTransferId,
                                                                 departureObjectId,
                                                                 arrivalObjectId,
                                                                 departureEpochJulian,
                                                                 departureSemiMajorAxis,
                                                                 departureEccentricity,
                                                                 departureInclination,
                                                                 departureArgumentOfPeriapsis,
                                                                 departureLongitudeAscendingNode,
                                                                 departureTrueAnomaly );
            SQLite::Statement zeroQuery( database, bindZeroes );
            zeroQuery.executeStep( );
            zeroQuery.reset( );

            ++showProgress;
            continue;
        }

        // Create virtual TLE for the transfer object's orbit from its departure state.
        // This TLE will be propagated using the SGP4 transfer.
        Tle transferTle;
        std::string solverStatusSummary;
        int numberOfIterations;
        const Tle referenceTle = Tle( );
        const int maximumIterations = 100;

        try
        {
            transferTle = atom::convertCartesianStateToTwoLineElements< double, Vector6 >(
                transferDepartureState,
                departureEpoch,
                solverStatusSummary,
                numberOfIterations,
                referenceTle,
                earthGravitationalParameter,
                earthMeanRadius,
                input.absoluteTolerance,
                input.relativeTolerance,
                maximumIterations );
        }
        catch( std::exception& virtualTleError )
        {
            // At the moment we just catch the exceptions that are thrown internally and proceed.
            // @todo: Figure out how to handle and register these exceptions.
        }

        // Check if transferTle is correct.
        const SGP4 sgp4Check( transferTle );
        bool testPassed = false;

        Eci propagatedStateEci = sgp4Check.FindPosition( 0.0 );
        Vector6 propagatedState;
        propagatedState[ astro::xPositionIndex ] = propagatedStateEci.Position( ).x;
        propagatedState[ astro::yPositionIndex ] = propagatedStateEci.Position( ).y;
        propagatedState[ astro::zPositionIndex ] = propagatedStateEci.Position( ).z;
        propagatedState[ astro::xVelocityIndex ] = propagatedStateEci.Velocity( ).x;
        propagatedState[ astro::yVelocityIndex ] = propagatedStateEci.Velocity( ).y;
        propagatedState[ astro::zVelocityIndex ] = propagatedStateEci.Velocity( ).z;

        Vector6 trueDepartureState;
        for ( int i = 0; i < 6; i++ )
        {
            trueDepartureState[ i ] = transferDepartureState[ i ];
        }

        testPassed = executeVirtualTleConvergenceTest( propagatedState,
                                                       trueDepartureState,
                                                       input.relativeTolerance,
                                                       input.absoluteTolerance );

        if ( testPassed == false )
        {
            // Bind zeroes to SQL insert sgp4Query.
            std::string bindZeroes = bindZeroesSGP4ScannerTable( lambertTransferId,
                                                                 departureObjectId,
                                                                 arrivalObjectId,
                                                                 departureEpochJulian,
                                                                 departureSemiMajorAxis,
                                                                 departureEccentricity,
                                                                 departureInclination,
                                                                 departureArgumentOfPeriapsis,
                                                                 departureLongitudeAscendingNode,
                                                                 departureTrueAnomaly );
            SQLite::Statement zeroQuery( database, bindZeroes );
            zeroQuery.executeStep( );
            zeroQuery.reset( );
            ++virtualTleFailCounter;
            ++showProgress;
            continue;
        }

        // Propagate transfer object using the SGP4 propagator.
        const SGP4 sgp4( transferTle );
        DateTime sgp4ArrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
        Vector eciPosition = Vector( );
        Vector eciVelocity = Vector( );
        Eci sgp4ArrivalStateEci( sgp4ArrivalEpoch, eciPosition, eciVelocity );
        try
        {
            sgp4ArrivalStateEci = sgp4.FindPosition( sgp4ArrivalEpoch );
        }
        catch( std::exception& sgp4PropagationError )
        {
            // Bind zeroes to SQL insert sgp4Query.
            std::string bindZeroes = bindZeroesSGP4ScannerTable( lambertTransferId,
                                                                 departureObjectId,
                                                                 arrivalObjectId,
                                                                 departureEpochJulian,
                                                                 departureSemiMajorAxis,
                                                                 departureEccentricity,
                                                                 departureInclination,
                                                                 departureArgumentOfPeriapsis,
                                                                 departureLongitudeAscendingNode,
                                                                 departureTrueAnomaly );
            SQLite::Statement zeroQuery( database, bindZeroes );
            zeroQuery.executeStep( );
            zeroQuery.reset( );
            ++arrivalEpochPropagationFailCounter;
            ++showProgress;
            continue;
        }

        const Vector6 sgp4ArrivalState = getStateVector( sgp4ArrivalStateEci );

        // Compute the required results.
        const double sgp4ArrivalPositionX = sgp4ArrivalState[ astro::xPositionIndex ];
        const double sgp4ArrivalPositionY = sgp4ArrivalState[ astro::yPositionIndex ];
        const double sgp4ArrivalPositionZ = sgp4ArrivalState[ astro::zPositionIndex ];

        const double sgp4ArrivalVelocityX = sgp4ArrivalState[ astro::xVelocityIndex ];
        const double sgp4ArrivalVelocityY = sgp4ArrivalState[ astro::yVelocityIndex ];
        const double sgp4ArrivalVelocityZ = sgp4ArrivalState[ astro::zVelocityIndex ];

        const double arrivalPositionErrorX = sgp4ArrivalPositionX - lambertArrivalPositionX;
        const double arrivalPositionErrorY = sgp4ArrivalPositionY - lambertArrivalPositionY;
        const double arrivalPositionErrorZ = sgp4ArrivalPositionZ - lambertArrivalPositionZ;

        Vector3 positionError;
        positionError[ astro::xPositionIndex ] = arrivalPositionErrorX;
        positionError[ astro::yPositionIndex ] = arrivalPositionErrorY;
        positionError[ astro::zPositionIndex ] = arrivalPositionErrorZ;
        const double arrivalPositionErrorNorm = sml::norm< double >( positionError );

        const double arrivalVelocityErrorX = sgp4ArrivalVelocityX -
                                            ( lambertArrivalVelocityX - lambertArrivalDeltaVX );
        const double arrivalVelocityErrorY = sgp4ArrivalVelocityY -
                                            ( lambertArrivalVelocityY - lambertArrivalDeltaVY );
        const double arrivalVelocityErrorZ = sgp4ArrivalVelocityZ -
                                            ( lambertArrivalVelocityZ - lambertArrivalDeltaVZ );

        Vector3 velocityError;
        velocityError[ 0 ] = arrivalVelocityErrorX;
        velocityError[ 1 ] = arrivalVelocityErrorY;
        velocityError[ 2 ] = arrivalVelocityErrorZ;
        double arrivalVelocityErrorNorm = sml::norm< double >( velocityError );

        // Bind values to SQL insert sgp4Query
        sgp4Query.bind( ":lambert_transfer_id",                   lambertTransferId );
        sgp4Query.bind( ":departure_object_id",                   departureObjectId );
        sgp4Query.bind( ":arrival_object_id",                     arrivalObjectId );
        sgp4Query.bind( ":departure_epoch",                       departureEpochJulian );

        sgp4Query.bind( ":departure_semi_major_axis",             departureSemiMajorAxis );
        sgp4Query.bind( ":departure_eccentricity",                departureEccentricity );
        sgp4Query.bind( ":departure_inclination",                 departureInclination );
        sgp4Query.bind( ":departure_argument_of_periapsis",       departureArgumentOfPeriapsis );
        sgp4Query.bind( ":departure_longitude_of_ascending_node", departureLongitudeAscendingNode );
        sgp4Query.bind( ":departure_true_anomaly",                departureTrueAnomaly );

        sgp4Query.bind( ":arrival_position_x",                    sgp4ArrivalPositionX );
        sgp4Query.bind( ":arrival_position_y",                    sgp4ArrivalPositionY );
        sgp4Query.bind( ":arrival_position_z",                    sgp4ArrivalPositionZ );
        sgp4Query.bind( ":arrival_velocity_x",                    sgp4ArrivalVelocityX );
        sgp4Query.bind( ":arrival_velocity_y",                    sgp4ArrivalVelocityY );
        sgp4Query.bind( ":arrival_velocity_z",                    sgp4ArrivalVelocityZ );
        sgp4Query.bind( ":arrival_position_x_error",              arrivalPositionErrorX );
        sgp4Query.bind( ":arrival_position_y_error",              arrivalPositionErrorY );
        sgp4Query.bind( ":arrival_position_z_error",              arrivalPositionErrorZ );
        sgp4Query.bind( ":arrival_position_error",                arrivalPositionErrorNorm );
        sgp4Query.bind( ":arrival_velocity_x_error",              arrivalVelocityErrorX );
        sgp4Query.bind( ":arrival_velocity_y_error",              arrivalVelocityErrorY );
        sgp4Query.bind( ":arrival_velocity_z_error",              arrivalVelocityErrorZ );
        sgp4Query.bind( ":arrival_velocity_error",                arrivalVelocityErrorNorm );
        sgp4Query.bind( ":success",                               1 );

        sgp4Query.executeStep( );
        sgp4Query.reset( );

        ++showProgress;
    }

    // Fetch number of rows in sgp4_scanner_results table.
    std::ostringstream sgp4ScannerTableSizeSelect;
    sgp4ScannerTableSizeSelect << "SELECT COUNT(*) FROM sgp4_scanner_results;";
    const int sgp4ScannertTableSize
        = database.execAndGet( sgp4ScannerTableSizeSelect.str( ) );

    std::ostringstream totalLambertCasesConsideredSelect;
    totalLambertCasesConsideredSelect
        << "SELECT COUNT(*) FROM lambert_scanner_results WHERE transfer_delta_v <= "
        << input.transferDeltaVCutoff << ";";
    const int totalLambertCasesConsidered
        = database.execAndGet( totalLambertCasesConsideredSelect.str( ) );

    std::cout << std::endl;
    std::cout << "Total Lambert cases = " << lambertScannertTableSize << std::endl;
    std::cout << "Total SGP4 cases = " << sgp4ScannertTableSize << std::endl;
    std::cout << std::endl;
    std::cout << "Number of Lambert cases considered with the transfer deltaV cut-off = "
              << totalLambertCasesConsidered << std::endl;
    std::cout << "Number of virtual TLE convergence fail cases = "
              << virtualTleFailCounter << std::endl;
    std::cout << "Number of arrival epoch propagation fail cases = "
              << arrivalEpochPropagationFailCounter << std::endl;

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;

    // Check if shortlist file should be created; call function to write output.
    if ( input.shortlistLength > 0 )
    {
        std::cout << "Writing shortlist to file ... " << std::endl;
        writeSGP4TransferShortlist( database, input.shortlistLength, input.shortlistPath );
        std::cout << "Shortlist file created successfully!" << std::endl;
    }
}

//! Check sgp4_scanner input parameters.
sgp4ScannerInput checkSGP4ScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                         " << catalogPath << std::endl;

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

    return sgp4ScannerInput( catalogPath,
                             transferDeltaVCutoff,
                             relativeTolerance,
                             absoluteTolerance,
                             databasePath,
                             shortlistLength,
                             shortlistPath );
}

//! Create sgp4_scanner table.
void createSGP4ScannerTable( SQLite::Database& database )
{
    // Check that lambert_scanner_results table exists.
    if ( !database.tableExists( "lambert_scanner_results" ) )
    {
        throw std::runtime_error(
            "ERROR: \"lambert_scanner_results\" must exist and be populated!" );
    }

    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS sgp4_scanner_results;" );

    // Set up SQL command to create table to store SGP4 Scanner results.
    std::ostringstream sgp4ScannerTableCreate;
    sgp4ScannerTableCreate
        << "CREATE TABLE sgp4_scanner_results ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"                     INTEGER,"
        << "\"departure_object_id\"                     INTEGER,"
        << "\"arrival_object_id\"                       INTEGER,"
        << "\"departure_epoch\"                         REAL,"
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
        << "\"arrival_position_x_error\"                REAL,"
        << "\"arrival_position_y_error\"                REAL,"
        << "\"arrival_position_z_error\"                REAL,"
        << "\"arrival_position_error\"                  REAL,"
        << "\"arrival_velocity_x_error\"                REAL,"
        << "\"arrival_velocity_y_error\"                REAL,"
        << "\"arrival_velocity_z_error\"                REAL,"
        << "\"arrival_velocity_error\"                  REAL,"
        << "\"success\"                                 INTEGER"
        <<                                              ");";

    // Execute command to create table.
    database.exec( sgp4ScannerTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_position_error column.
    std::ostringstream arrivalPositionErrorIndexCreate;
    arrivalPositionErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_position_error\" on "
                                    << "sgp4_scanner_results (arrival_position_error ASC);";
    database.exec( arrivalPositionErrorIndexCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_velocity_error column.
    std::ostringstream arrivalVelocityErrorIndexCreate;
    arrivalVelocityErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_velocity_error\" on "
                                    << "sgp4_scanner_results (arrival_velocity_error ASC);";
    database.exec( arrivalVelocityErrorIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "sgp4_scanner_results" ) )
    {
        std::ostringstream errorMessage;
        errorMessage << "ERROR: Creating table 'sgp4_scanner_results' failed in sgp4Scanner.cpp!";
        throw std::runtime_error( errorMessage.str( ) );
    }
}

//! Bind zeroes into sgp4_scanner_results table.
std::string bindZeroesSGP4ScannerTable( const int lambertTransferId,
                                        const int departureObjectId,
                                        const int arrivalObjectId,
                                        const double departureEpochJulian,
                                        const double departureSemiMajorAxis,
                                        const double departureEccentricity,
                                        const double departureInclination,
                                        const double departureArgumentOfPeriapsis,
                                        const double departureLongitudeAscendingNode,
                                        const double departureTrueAnomaly )
{
    std::ostringstream zeroEntry;
    zeroEntry << "INSERT INTO sgp4_scanner_results VALUES ("
              << "NULL"                          << ","
              << lambertTransferId               << ","
              << departureObjectId               << ","
              << arrivalObjectId                 << ","
              << departureEpochJulian            << ","
              << departureSemiMajorAxis          << ","
              << departureEccentricity           << ","
              << departureInclination            << ","
              << departureArgumentOfPeriapsis    << ","
              << departureLongitudeAscendingNode << ","
              << departureTrueAnomaly            << ","
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
void writeSGP4TransferShortlist( SQLite::Database& database,
                                 const int shortlistNumber,
                                 const std::string& shortlistPath )
{
    // Fetch transfers to include in shortlist.
    // The sgp4_scanner_results table is sorted by transfer_delta_v from the lambert_scanner_results
    // table in ascending order.
    std::ostringstream shortlistSelect;
    shortlistSelect << "SELECT sgp4_scanner_results.transfer_id, "
                    << "sgp4_scanner_results.lambert_transfer_id, "
                    << "lambert_scanner_results.transfer_delta_v, "
                    << "sgp4_scanner_results.departure_object_id, "
                    << "sgp4_scanner_results.arrival_object_id, "
                    << "sgp4_scanner_results.departure_epoch, "
                    << "sgp4_scanner_results.departure_semi_major_axis, "
                    << "sgp4_scanner_results.departure_eccentricity, "
                    << "sgp4_scanner_results.departure_inclination, "
                    << "sgp4_scanner_results.departure_argument_of_periapsis, "
                    << "sgp4_scanner_results.departure_longitude_of_ascending_node, "
                    << "sgp4_scanner_results.departure_true_anomaly, "
                    << "sgp4_scanner_results.arrival_position_x, "
                    << "sgp4_scanner_results.arrival_position_y, "
                    << "sgp4_scanner_results.arrival_position_z, "
                    << "sgp4_scanner_results.arrival_velocity_x, "
                    << "sgp4_scanner_results.arrival_velocity_y, "
                    << "sgp4_scanner_results.arrival_velocity_z, "
                    << "sgp4_scanner_results.arrival_position_x_error, "
                    << "sgp4_scanner_results.arrival_position_y_error, "
                    << "sgp4_scanner_results.arrival_position_z_error, "
                    << "sgp4_scanner_results.arrival_position_error, "
                    << "sgp4_scanner_results.arrival_velocity_x_error, "
                    << "sgp4_scanner_results.arrival_velocity_y_error, "
                    << "sgp4_scanner_results.arrival_velocity_z_error, "
                    << "sgp4_scanner_results.arrival_velocity_error "
                    << "FROM sgp4_scanner_results INNER JOIN lambert_scanner_results "
                    << "ON lambert_scanner_results.transfer_id = "
                    << "sgp4_scanner_results.lambert_transfer_id "
                    << "ORDER BY lambert_scanner_results.transfer_delta_v ASC LIMIT "
                    << shortlistNumber << ";";

    SQLite::Statement query( database, shortlistSelect.str( ) );

    // Write fetch data to file.
    std::ofstream shortlistFile( shortlistPath.c_str( ) );

    // Print file header.
    shortlistFile << "transfer_id,"
                  << "lambert_transfer_id,"
                  << "transfer_delta_v,"
                  << "departure_object_id,"
                  << "arrival_object_id,"
                  << "departure_epoch,"
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
                  << "arrival_position_x_error,"
                  << "arrival_position_y_error,"
                  << "arrival_position_z_error,"
                  << "arrival_position_error,"
                  << "arrival_velocity_x_error,"
                  << "arrival_velocity_y_error,"
                  << "arrival_velocity_z_error,"
                  << "arrival_velocity_error"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int    transferId                         = query.getColumn( 0 );
        const int    lambertTransferId                  = query.getColumn( 1 );
        const double lambertTransferDeltaV              = query.getColumn( 2 );
        const int    departureObjectId                  = query.getColumn( 3 );
        const int    arrivalObjectId                    = query.getColumn( 4 );

        const double departureEpoch                     = query.getColumn( 5 );
        const double departureSemiMajorAxis             = query.getColumn( 6 );
        const double departureEccentricity              = query.getColumn( 7 );
        const double departureInclination               = query.getColumn( 8 );
        const double departureArgumentOfPeriapsis       = query.getColumn( 9 );
        const double departureLongitudeOfAscendingNode  = query.getColumn( 10 );
        const double departureTrueAnomaly               = query.getColumn( 11 );

        const double arrivalPositionX                   = query.getColumn( 12 );
        const double arrivalPositionY                   = query.getColumn( 13 );
        const double arrivalPositionZ                   = query.getColumn( 14 );
        const double arrivalVelocityX                   = query.getColumn( 15 );
        const double arrivalVelocityY                   = query.getColumn( 16 );
        const double arrivalVelocityZ                   = query.getColumn( 17 );

        const double arrivalPositionErrorX              = query.getColumn( 18 );
        const double arrivalPositionErrorY              = query.getColumn( 19 );
        const double arrivalPositionErrorZ              = query.getColumn( 20 );
        const double arrivalPositionError               = query.getColumn( 21 );
        const double arrivalVelocityErrorX              = query.getColumn( 22 );
        const double arrivalVelocityErrorY              = query.getColumn( 23 );
        const double arrivalVelocityErrorZ              = query.getColumn( 24 );
        const double arrivalVelocityError               = query.getColumn( 25 );

        shortlistFile << transferId                         << ","
                      << lambertTransferId                  << ",";

        shortlistFile << std::setprecision( std::numeric_limits< double >::digits10 )
                      << lambertTransferDeltaV              << ",";

        shortlistFile << departureObjectId                  << ",";

        shortlistFile << std::setprecision( std::numeric_limits< double >::digits10 )
                      << arrivalObjectId                    << ","
                      << departureEpoch                     << ","
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
                      << arrivalPositionErrorX              << ","
                      << arrivalPositionErrorY              << ","
                      << arrivalPositionErrorZ              << ","
                      << arrivalPositionError               << ","
                      << arrivalVelocityErrorX              << ","
                      << arrivalVelocityErrorY              << ","
                      << arrivalVelocityErrorZ              << ","
                      << arrivalVelocityError               << ","
                      << std::endl;
    }

    shortlistFile.close( );
}

} // namespace d2d
