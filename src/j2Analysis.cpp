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

#include <Astro/astro.hpp>

#include <boost/progress.hpp>

#include "D2D/j2Analysis.hpp"
#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

#include <keplerian_toolbox.h>

namespace d2d
{

//! Execute j2_analysis.
void executeJ2Analysis( const rapidjson::Document& config )
{
    std::cout.precision( 15 );

    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const j2AnalysisInput input = checkJ2AnalysisInput( config );

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
    // N.B.: Database must already exist and contain populated tables called
    //       "lambert_scanner_results" and "sgp4_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create j2_analysis_results table in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createJ2AnalysisTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Fetch number of rows in sgp4_scanner_results table where success = 1.
    std::ostringstream sgp4ScannerTableSizeSelect;
    sgp4ScannerTableSizeSelect << "SELECT COUNT(*) FROM sgp4_scanner_results WHERE success = 1;";
    const int sgp4ScannertTableSize
        = database.execAndGet( sgp4ScannerTableSizeSelect.str( ) );
    std::cout << "# of cases to be considered in the J2 analysis = " << sgp4ScannertTableSize;
    std::cout << std::endl;

    // Set up select query to fetch data from lambert_scanner_results table.
    std::ostringstream lambertScannerTableSelect;
    lambertScannerTableSelect << "SELECT        lambert_scanner_results.transfer_id,"
                              << "              lambert_scanner_results.time_of_flight,"
                              << "              lambert_scanner_results.departure_position_x,"
                              << "              lambert_scanner_results.departure_position_y,"
                              << "              lambert_scanner_results.departure_position_z,"
                              << "              lambert_scanner_results.departure_velocity_x,"
                              << "              lambert_scanner_results.departure_velocity_y,"
                              << "              lambert_scanner_results.departure_velocity_z,"
                              << "              lambert_scanner_results.departure_delta_v_x,"
                              << "              lambert_scanner_results.departure_delta_v_y,"
                              << "              lambert_scanner_results.departure_delta_v_z,"
                              << "              lambert_scanner_results.arrival_position_x,"
                              << "              lambert_scanner_results.arrival_position_y,"
                              << "              lambert_scanner_results.arrival_position_z,"
                              << "              lambert_scanner_results.arrival_velocity_x,"
                              << "              lambert_scanner_results.arrival_velocity_y,"
                              << "              lambert_scanner_results.arrival_velocity_z,"
                              << "              lambert_scanner_results.arrival_delta_v_x,"
                              << "              lambert_scanner_results.arrival_delta_v_y,"
                              << "              lambert_scanner_results.arrival_delta_v_z "
                              << "FROM          lambert_scanner_results "
                              << "INNER JOIN    sgp4_scanner_results "
                              << "ON            sgp4_scanner_results.success = 1 "
                              << "AND           sgp4_scanner_results.lambert_transfer_id = "
                              << "              lambert_scanner_results.transfer_id;";

    SQLite::Statement lambertQuery( database, lambertScannerTableSelect.str( ) );
    std::cout << "Data selection from lambert_scanner_results table successfull!" << std::endl;

    // Set up insert query to insert data into j2_analysis_results table.
    std::ostringstream j2AnalysisTableInsert;
    j2AnalysisTableInsert << "INSERT INTO j2_analysis_results VALUES ("
        << "NULL,"
        << ":lambert_transfer_id,"
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
        << ":arrival_velocity_error"
        << ");";

    SQLite::Statement j2Query( database, j2AnalysisTableInsert.str( ) );
    std::cout << "Column headers set up successfully for j2_analysis_results table!" << std::endl;

    std::cout << "Performing J2 Analysis on transfer orbits ..." << std::endl << std::endl;

    boost::progress_display showProgress( sgp4ScannertTableSize );

    // Step through select query to fetch data from lambert_scanner_results.
    while ( lambertQuery.executeStep( ) )
    {
        const int      lambertTransferId                    = lambertQuery.getColumn( 0 );
        const double   timeOfFlight                         = lambertQuery.getColumn( 1 );

        const double   departurePositionX                   = lambertQuery.getColumn( 2 );
        const double   departurePositionY                   = lambertQuery.getColumn( 3 );
        const double   departurePositionZ                   = lambertQuery.getColumn( 4 );
        const double   departureVelocityX                   = lambertQuery.getColumn( 5 );
        const double   departureVelocityY                   = lambertQuery.getColumn( 6 );
        const double   departureVelocityZ                   = lambertQuery.getColumn( 7 );
        const double   departureDeltaVX                     = lambertQuery.getColumn( 8 );
        const double   departureDeltaVY                     = lambertQuery.getColumn( 9 );
        const double   departureDeltaVZ                     = lambertQuery.getColumn( 10 );

        const double   lambertArrivalPositionX              = lambertQuery.getColumn( 11 );
        const double   lambertArrivalPositionY              = lambertQuery.getColumn( 12 );
        const double   lambertArrivalPositionZ              = lambertQuery.getColumn( 13 );
        const double   lambertArrivalVelocityX              = lambertQuery.getColumn( 14 );
        const double   lambertArrivalVelocityY              = lambertQuery.getColumn( 15 );
        const double   lambertArrivalVelocityZ              = lambertQuery.getColumn( 16 );
        const double   lambertArrivalDeltaVX                = lambertQuery.getColumn( 17 );
        const double   lambertArrivalDeltaVY                = lambertQuery.getColumn( 18 );
        const double   lambertArrivalDeltaVZ                = lambertQuery.getColumn( 19 );

        // Get departure state for the transfer object ([km] and [km/s]).
        Vector6 transferDepartureState;
        transferDepartureState[ astro::xPositionIndex ] = departurePositionX;
        transferDepartureState[ astro::yPositionIndex ] = departurePositionY;
        transferDepartureState[ astro::zPositionIndex ] = departurePositionZ;
        transferDepartureState[ astro::xVelocityIndex ] = departureVelocityX + departureDeltaVX;
        transferDepartureState[ astro::yVelocityIndex ] = departureVelocityY + departureDeltaVY;
        transferDepartureState[ astro::zVelocityIndex ] = departureVelocityZ + departureDeltaVZ;

        // Convert transfer object's departure state to Orbital elements.
        Vector6 departureOrbitalElements;
        const double tolerance = 10.0 * std::numeric_limits< double >::epsilon( );
        departureOrbitalElements = astro::convertCartesianToKeplerianElements(
                                        transferDepartureState,
                                        earthGravitationalParameter,
                                        tolerance );

        double semiMajorAxis = departureOrbitalElements[ astro::semiMajorAxisIndex ];
        double eccentricity  = departureOrbitalElements[ astro::eccentricityIndex ];
        double inclination   = departureOrbitalElements[ astro::inclinationIndex ];
        double argumentOfPeriapsis
                             = departureOrbitalElements[ astro::argumentOfPeriapsisIndex ];
        double longitudeAscendingNode
                             = departureOrbitalElements[ astro::longitudeOfAscendingNodeIndex ];
        double trueAnomaly   = departureOrbitalElements[ astro::trueAnomalyIndex ];

        // Evaluate change in longitude of ascending node due to J2 perturbation.
        const double massOfOrbitingBody = 0.0;
        double meanMotion = astro::computeKeplerMeanMotion(
                                semiMajorAxis,
                                earthGravitationalParameter,
                                massOfOrbitingBody );

        double meanMotionDegreesPerDay = ( meanMotion * 180.0 / sml::SML_PI ) * 86400.0;

        const double j2Constant = 0.00108263;

        // Change in longitude of ascending node in degrees per day:
        double longitudeAscendingNodeDot;
        longitudeAscendingNodeDot = -1.5 * meanMotionDegreesPerDay * j2Constant *
                                        std::pow( ( earthMeanRadius / semiMajorAxis ), 2 ) *
                                            std::cos( inclination ) /
                                                std::pow( ( 1 - std::pow( eccentricity, 2 ) ), 2 );

        // Total change in longitude of ascending node in degrees over the time of flight:
        double deltaLongitudeAscendingNode
                = ( ( longitudeAscendingNodeDot / 86400.0 ) * sml::SML_PI / 180.0 ) * timeOfFlight;

        // Evaluate change in argument of periapsis due to J2 perturbation.
        // Change in argument of periapsis in degrees per day:
        double argumentOfPeriapsisDot;
        argumentOfPeriapsisDot = 0.75 * meanMotionDegreesPerDay * j2Constant *
                                    std::pow( ( earthMeanRadius / semiMajorAxis ), 2 ) *
                                        ( 4 - 5 * std::pow( std::sin( inclination ), 2 ) ) /
                                            std::pow( ( 1 - std::pow( eccentricity, 2 ) ), 2 );

        // Total change in largument of periapsis in degrees over the time of flight:
        double deltaArgumentOfPeriapsis
                = ( ( argumentOfPeriapsisDot / 86400.0 ) * sml::SML_PI / 180.0 ) * timeOfFlight;

        // Evaluate change in true anomaly over the time of flight.
        // Get initial eccentric anomaly from the initial true anomaly:
        double initialEccentricAnomaly = astro::convertTrueAnomalyToEllipticalEccentricAnomaly(
                                            trueAnomaly,
                                            eccentricity );

        // Get initial mean anomaly from the initial eccentric anomaly:
        double initialMeanAnomaly = astro::convertEllipticalEccentricAnomalyToMeanAnomaly(
                                        initialEccentricAnomaly,
                                        eccentricity );

        // Get final mean anomaly at the arrival point of the trasnfer orbit:
        double finalMeanAnomaly = sml::computeModulo(
                        ( meanMotion * timeOfFlight + initialMeanAnomaly ),
                          2 * sml::SML_PI );

        // Get final eccentric anomaly at the arrival point of the transfer orbit:
        double finalEccentricAnomaly = sml::computeModulo(
                 kep_toolbox::m2e( finalMeanAnomaly, eccentricity ),
                 2 * sml::SML_PI );

        // Get final true anomaly at the arrival point of the transfer orbit:
        double finalTrueAnomaly = astro::convertEllipticalEccentricAnomalyToTrueAnomaly(
                                     finalEccentricAnomaly,
                                     eccentricity );
        finalTrueAnomaly = sml::computeModulo( finalTrueAnomaly, 2 * sml::SML_PI );

        // Update the Orbital elements.
        longitudeAscendingNode = longitudeAscendingNode + deltaLongitudeAscendingNode;
        argumentOfPeriapsis    = argumentOfPeriapsis + deltaArgumentOfPeriapsis;
        trueAnomaly            = finalTrueAnomaly;

        Vector6 keplerianElements;
        keplerianElements[ astro::semiMajorAxisIndex ]              = semiMajorAxis;
        keplerianElements[ astro::eccentricityIndex ]               = eccentricity;
        keplerianElements[ astro::inclinationIndex ]                = inclination;
        keplerianElements[ astro::argumentOfPeriapsisIndex ]        = argumentOfPeriapsis;
        keplerianElements[ astro::longitudeOfAscendingNodeIndex ]   = longitudeAscendingNode;
        keplerianElements[ astro::trueAnomalyIndex ]                = trueAnomaly;

        // Convert the Orbital elements at the arrival point back to cartesian state.
        Vector6 arrivalTransferState = astro::convertKeplerianToCartesianElements(
                                            keplerianElements,
                                            earthGravitationalParameter,
                                            tolerance );

        // Compute the required results.
        const double j2ArrivalPositionX = arrivalTransferState[ astro::xPositionIndex ];
        const double j2ArrivalPositionY = arrivalTransferState[ astro::yPositionIndex ];
        const double j2ArrivalPositionZ = arrivalTransferState[ astro::zPositionIndex ];

        const double j2ArrivalVelocityX = arrivalTransferState[ astro::xVelocityIndex ];
        const double j2ArrivalVelocityY = arrivalTransferState[ astro::yVelocityIndex ];
        const double j2ArrivalVelocityZ = arrivalTransferState[ astro::zVelocityIndex ];

        const double arrivalPositionErrorX = j2ArrivalPositionX - lambertArrivalPositionX;
        const double arrivalPositionErrorY = j2ArrivalPositionY - lambertArrivalPositionY;
        const double arrivalPositionErrorZ = j2ArrivalPositionZ - lambertArrivalPositionZ;

        Vector3 positionError;
        positionError[ astro::xPositionIndex ] = arrivalPositionErrorX;
        positionError[ astro::yPositionIndex ] = arrivalPositionErrorY;
        positionError[ astro::zPositionIndex ] = arrivalPositionErrorZ;
        const double arrivalPositionErrorNorm = sml::norm< double >( positionError );

        const double arrivalVelocityErrorX = j2ArrivalVelocityX -
                                            ( lambertArrivalVelocityX - lambertArrivalDeltaVX );
        const double arrivalVelocityErrorY = j2ArrivalVelocityY -
                                            ( lambertArrivalVelocityY - lambertArrivalDeltaVY );
        const double arrivalVelocityErrorZ = j2ArrivalVelocityZ -
                                            ( lambertArrivalVelocityZ - lambertArrivalDeltaVZ );

        Vector3 velocityError;
        velocityError[ 0 ] = arrivalVelocityErrorX;
        velocityError[ 1 ] = arrivalVelocityErrorY;
        velocityError[ 2 ] = arrivalVelocityErrorZ;
        double arrivalVelocityErrorNorm = sml::norm< double >( velocityError );

        // Bind computed values to sgp4Query.
        j2Query.bind( ":lambert_transfer_id",                   lambertTransferId );
        j2Query.bind( ":arrival_position_x",                    j2ArrivalPositionX );
        j2Query.bind( ":arrival_position_y",                    j2ArrivalPositionY );
        j2Query.bind( ":arrival_position_z",                    j2ArrivalPositionZ );
        j2Query.bind( ":arrival_velocity_x",                    j2ArrivalVelocityX );
        j2Query.bind( ":arrival_velocity_y",                    j2ArrivalVelocityY );
        j2Query.bind( ":arrival_velocity_z",                    j2ArrivalVelocityZ );
        j2Query.bind( ":arrival_position_x_error",              arrivalPositionErrorX );
        j2Query.bind( ":arrival_position_y_error",              arrivalPositionErrorY );
        j2Query.bind( ":arrival_position_z_error",              arrivalPositionErrorZ );
        j2Query.bind( ":arrival_position_error",                arrivalPositionErrorNorm );
        j2Query.bind( ":arrival_velocity_x_error",              arrivalVelocityErrorX );
        j2Query.bind( ":arrival_velocity_y_error",              arrivalVelocityErrorY );
        j2Query.bind( ":arrival_velocity_z_error",              arrivalVelocityErrorZ );
        j2Query.bind( ":arrival_velocity_error",                arrivalVelocityErrorNorm );

        j2Query.executeStep( );
        j2Query.reset( );

        ++showProgress;
    }

    // Fetch number of rows in j2_analysis_results table.
    std::ostringstream j2AnalysisTableSizeSelect;
    j2AnalysisTableSizeSelect << "SELECT COUNT(*) FROM j2_analysis_results;";
    const int j2AnalysistTableSize
        = database.execAndGet( j2AnalysisTableSizeSelect.str( ) );

    std::cout << std::endl;
    std::cout << "Total SGP4 (success) cases = " << sgp4ScannertTableSize << std::endl;
    std::cout << std::endl;
    std::cout << "Total j2 analysis cases = " << j2AnalysistTableSize << std::endl;

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;

    // Check if shortlist file should be created; call function to write output.
    if ( input.shortlistLength > 0 )
    {
        std::cout << "Writing shortlist to file ... " << std::endl;
        writeJ2TransferShortlist( database, input.shortlistLength, input.shortlistPath );
        std::cout << "Shortlist file created successfully!" << std::endl;
    }
}

//! Check j2_analysis input parameters.
j2AnalysisInput checkJ2AnalysisInput( const rapidjson::Document& config )
{
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

    return j2AnalysisInput( databasePath,
                            shortlistLength,
                            shortlistPath );
}

//! Create j2_analysis_results table.
void createJ2AnalysisTable( SQLite::Database& database )
{
    // Check that lambert_scanner_results table exists.
    if ( !database.tableExists( "lambert_scanner_results" ) )
    {
        throw std::runtime_error(
            "ERROR: \"lambert_scanner_results\" must exist and be populated!" );
    }

    // Check that sgp4_scanner_results table exists.
    if ( !database.tableExists( "sgp4_scanner_results" ) )
    {
        throw std::runtime_error(
            "ERROR: \"sgp4_scanner_results\" must exist and be populated!" );
    }

    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS j2_analysis_results;" );

    // Set up SQL command to create table to store J2 Analysis results.
    std::ostringstream j2AnalysisTableCreate;
    j2AnalysisTableCreate
        << "CREATE TABLE j2_analysis_results ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"                     INTEGER,"
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
        << "\"arrival_velocity_error\"                  REAL"
        <<                                              ");";

    // Execute command to create table.
    database.exec( j2AnalysisTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_position_error column.
    std::ostringstream arrivalPositionErrorIndexCreate;
    arrivalPositionErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_position_error\" on "
                                    << "j2_analysis_results (arrival_position_error ASC);";
    database.exec( arrivalPositionErrorIndexCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_velocity_error column.
    std::ostringstream arrivalVelocityErrorIndexCreate;
    arrivalVelocityErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_velocity_error\" on "
                                    << "j2_analysis_results (arrival_velocity_error ASC);";
    database.exec( arrivalVelocityErrorIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "j2_analysis_results" ) )
    {
        std::ostringstream errorMessage;
        errorMessage << "ERROR: Creating table 'j2_analysis_results' failed in j2Analysis.cpp!";
        throw std::runtime_error( errorMessage.str( ) );
    }
}

//! Write transfer shortlist to file.
void writeJ2TransferShortlist( SQLite::Database& database,
                               const int shortlistNumber,
                               const std::string& shortlistPath )
{
    // Fetch transfers to include in shortlist.
    // The j2_analysis_results table is sorted by transfer_delta_v from the lambert_scanner_results
    // table in ascending order.
    std::ostringstream shortlistSelect;
    shortlistSelect << "SELECT      j2_analysis_results.transfer_id, "
                    << "            j2_analysis_results.lambert_transfer_id, "
                    << "            lambert_scanner_results.transfer_delta_v, "
                    << "            lambert_scanner_results.departure_object_id, "
                    << "            lambert_scanner_results.arrival_object_id, "
                    << "            j2_analysis_results.arrival_position_x, "
                    << "            j2_analysis_results.arrival_position_y, "
                    << "            j2_analysis_results.arrival_position_z, "
                    << "            j2_analysis_results.arrival_velocity_x, "
                    << "            j2_analysis_results.arrival_velocity_y, "
                    << "            j2_analysis_results.arrival_velocity_z, "
                    << "            j2_analysis_results.arrival_position_x_error, "
                    << "            j2_analysis_results.arrival_position_y_error, "
                    << "            j2_analysis_results.arrival_position_z_error, "
                    << "            j2_analysis_results.arrival_position_error, "
                    << "            j2_analysis_results.arrival_velocity_x_error, "
                    << "            j2_analysis_results.arrival_velocity_y_error, "
                    << "            j2_analysis_results.arrival_velocity_z_error, "
                    << "            j2_analysis_results.arrival_velocity_error "
                    << "FROM        j2_analysis_results "
                    << "INNER JOIN  lambert_scanner_results "
                    << "ON          lambert_scanner_results.transfer_id = "
                    << "            j2_analysis_results.lambert_transfer_id "
                    << "ORDER BY    lambert_scanner_results.transfer_delta_v ASC LIMIT "
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

        const double arrivalPositionX                   = query.getColumn( 5 );
        const double arrivalPositionY                   = query.getColumn( 6 );
        const double arrivalPositionZ                   = query.getColumn( 7 );
        const double arrivalVelocityX                   = query.getColumn( 8 );
        const double arrivalVelocityY                   = query.getColumn( 9 );
        const double arrivalVelocityZ                   = query.getColumn( 10 );

        const double arrivalPositionErrorX              = query.getColumn( 11 );
        const double arrivalPositionErrorY              = query.getColumn( 12 );
        const double arrivalPositionErrorZ              = query.getColumn( 13 );
        const double arrivalPositionError               = query.getColumn( 14 );
        const double arrivalVelocityErrorX              = query.getColumn( 15 );
        const double arrivalVelocityErrorY              = query.getColumn( 16 );
        const double arrivalVelocityErrorZ              = query.getColumn( 17 );
        const double arrivalVelocityError               = query.getColumn( 18 );

        shortlistFile << transferId                         << ","
                      << lambertTransferId                  << ",";

        shortlistFile << std::setprecision( std::numeric_limits< double >::digits10 )
                      << lambertTransferDeltaV              << ",";

        shortlistFile << departureObjectId                  << ","
                      << arrivalObjectId                    << ",";

        shortlistFile << std::setprecision( std::numeric_limits< double >::digits10 )
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
