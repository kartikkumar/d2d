/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <fstream>
#include <iostream>
#include <sstream>

#include <keplerian_toolbox.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <sqlite3.h>

#include <Astro/astro.hpp>

#include "D2D/lambertFetch.hpp"
#include "D2D/tools.hpp"

namespace d2d
{

//! Fetch details of specific debris-to-debris Lambert transfer.
void fetchLambertTransfer( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertFetchInput input = checkLambertFetchInput( config );

    // Set gravitational parameter used by Lambert targeter.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s_2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                             Simulation                           " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    std::cout << "Fetching transfer from database ... " << std::endl;

    // Connect to database and fetch metadata required to construct LambertTransferInput object.
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READONLY );

    std::ostringstream transferSelect;
    transferSelect << "SELECT * FROM lambert_scanner_results WHERE transfer_id = "
                   << input.transferId << ";";
    SQLite::Statement query( database, transferSelect.str( ) );
    query.executeStep( );

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
    const double arrivalPositionX                   = query.getColumn( 19 );
    const double arrivalPositionY                   = query.getColumn( 20 );
    const double arrivalPositionZ                   = query.getColumn( 21 );
    const double arrivalVelocityX                   = query.getColumn( 22 );
    const double arrivalVelocityY                   = query.getColumn( 23 );
    const double arrivalVelocityZ                   = query.getColumn( 24 );
    const double departureDeltaVX                   = query.getColumn( 37 );
    const double departureDeltaVY                   = query.getColumn( 38 );
    const double departureDeltaVZ                   = query.getColumn( 39 );
    const double transferDeltaV                     = query.getColumn( 43 );

    std::cout << "Transfer successfully fetched from database!" << std::endl;

    std::cout << "Propagating transfer ... " << std::endl;

    // Compute and store transfer state history by propagating conic section (Kepler orbit).
    Vector6 transferDepartureState;
    transferDepartureState[ 0 ] = departurePositionX;
    transferDepartureState[ 1 ] = departurePositionY;
    transferDepartureState[ 2 ] = departurePositionZ;
    transferDepartureState[ 3 ] = departureVelocityX + departureDeltaVX;
    transferDepartureState[ 4 ] = departureVelocityY + departureDeltaVY;
    transferDepartureState[ 5 ] = departureVelocityZ + departureDeltaVZ;

    const StateHistory transferPath = sampleKeplerOrbit( transferDepartureState,
                                                         timeOfFlight,
                                                         input.outputSteps,
                                                         earthGravitationalParameter,
                                                         departureEpoch );

    std::cout << "Transfer propagated successfully!" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                               Output                             " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Write metadata to file.
    std::ostringstream metadataPath;
    metadataPath << input.outputDirectory << "/transfer" << input.transferId << "_"
                 << input.metadataFilename;
    std::ofstream metadataFile( metadataPath.str( ).c_str( ) );
    print( metadataFile, "departure_id", departureObjectId, "-" );
    metadataFile << std::endl;
    print( metadataFile, "arrival_id", arrivalObjectId, "-" );
    metadataFile << std::endl;
    print( metadataFile, "departure_epoch", departureEpoch, "JD" );
    metadataFile << std::endl;
    print( metadataFile, "time_of_flight", timeOfFlight, "s" );
    metadataFile << std::endl;

    if ( prograde == 1 )
    {
        print( metadataFile, "is_prograde", "true", "-" );
    }
    else
    {
        print( metadataFile, "is_prograde", "false", "-" );
    }
    metadataFile << std::endl;

    print( metadataFile, "revolutions", revolutions, "-" );
    metadataFile << std::endl;
    print( metadataFile, "transfer_delta_v", transferDeltaV, "km/s" );
    metadataFile.close( );

    // Defined common header line for all the ephemeric files generated below.
    const std::string ephemerisFileHeader = "jd,x,y,z,xdot,ydot,zdot";

    Vector6 departureState;
    departureState[ 0 ] = departurePositionX;
    departureState[ 1 ] = departurePositionY;
    departureState[ 2 ] = departurePositionZ;
    departureState[ 3 ] = departureVelocityX;
    departureState[ 4 ] = departureVelocityY;
    departureState[ 5 ] = departureVelocityZ;

    // Compute period of departure orbit.
    const Vector6 departureStateKepler
        = astro::convertCartesianToKeplerianElements( departureState,
                                                      earthGravitationalParameter );
    const double departureOrbitalPeriod
        = astro::computeKeplerOrbitalPeriod( departureStateKepler[ astro::semiMajorAxisIndex ],
                                             earthGravitationalParameter );

    // Sample departure orbit.
    const StateHistory departureOrbit = sampleKeplerOrbit( departureState,
                                                           departureOrbitalPeriod,
                                                           input.outputSteps,
                                                           earthGravitationalParameter,
                                                           departureEpoch );

    // Write sampled departure orbit to file.
    std::ostringstream departureOrbitFilePath;
    departureOrbitFilePath << input.outputDirectory << "/transfer" << input.transferId << "_"
                           << input.departureOrbitFilename;
    std::ofstream departureOrbitFile( departureOrbitFilePath.str( ).c_str( ) );
    print( departureOrbitFile, departureOrbit, ephemerisFileHeader );

    // Sample departure path.
    const StateHistory departurePath = sampleKeplerOrbit( departureState,
                                                          timeOfFlight,
                                                          input.outputSteps,
                                                          earthGravitationalParameter,
                                                          departureEpoch );
    // Write sampled departure path to file.
    std::ostringstream departurePathFilePath;
    departurePathFilePath << input.outputDirectory << "/transfer" << input.transferId << "_"
                          << input.departurePathFilename;
    std::ofstream departurePathFile( departurePathFilePath.str( ).c_str( ) );
    print( departurePathFile, departurePath, ephemerisFileHeader );
    departurePathFile.close( );

    Vector6 arrivalState;
    arrivalState[ 0 ] = arrivalPositionX;
    arrivalState[ 1 ] = arrivalPositionY;
    arrivalState[ 2 ] = arrivalPositionZ;
    arrivalState[ 3 ] = arrivalVelocityX;
    arrivalState[ 4 ] = arrivalVelocityY;
    arrivalState[ 5 ] = arrivalVelocityZ;

    // Compute period of arrival orbit.
    const Vector6 arrivalStateKepler
        = astro::convertCartesianToKeplerianElements( arrivalState,
                                                      earthGravitationalParameter );
    const double arrivalOrbitalPeriod
        = astro::computeKeplerOrbitalPeriod( arrivalStateKepler[ astro::semiMajorAxisIndex ],
                                             earthGravitationalParameter );

    // Sample arrival orbit.
    const StateHistory arrivalOrbit = sampleKeplerOrbit( arrivalState,
                                                         arrivalOrbitalPeriod,
                                                         input.outputSteps,
                                                         earthGravitationalParameter,
                                                         departureEpoch );

    // Write sampled arrival orbit to file.
    std::ostringstream arrivalOrbitFilePath;
    arrivalOrbitFilePath << input.outputDirectory << "/transfer" << input.transferId << "_"
                         << input.arrivalOrbitFilename;
    std::ofstream arrivalOrbitFile( arrivalOrbitFilePath.str( ).c_str( ) );
    print( arrivalOrbitFile, arrivalOrbit, ephemerisFileHeader );
    arrivalOrbitFile.close( );

    // Sample arrival path.
    const double timeOfFlightInDays = timeOfFlight / ( 24.0 * 3600.0 );
    const StateHistory arrivalPath = sampleKeplerOrbit( arrivalState,
                                                        -timeOfFlight,
                                                        input.outputSteps,
                                                        earthGravitationalParameter,
                                                        departureEpoch + timeOfFlightInDays );

    // Write sampled arrival path to file.
    std::ostringstream arrivalPathFilePath;
    arrivalPathFilePath << input.outputDirectory << "/transfer" << input.transferId << "_"
                        << input.arrivalPathFilename;
    std::ofstream arrivalPathFile( arrivalPathFilePath.str( ).c_str( ) );
    print( arrivalPathFile, arrivalPath, ephemerisFileHeader );
    arrivalPathFile.close( );

    // Sample transfer trajectory.

    // Compute period of transfer orbit.
    const Vector6 transferDepartureStateKepler
        = astro::convertCartesianToKeplerianElements( transferDepartureState,
                                                      earthGravitationalParameter );
    const double transferOrbitalPeriod
        = astro::computeKeplerOrbitalPeriod(
            transferDepartureStateKepler[ astro::semiMajorAxisIndex ],
            earthGravitationalParameter );

    // Sample transfer orbit.
    const StateHistory transferOrbit
        = sampleKeplerOrbit( transferDepartureState,
                             transferOrbitalPeriod,
                             input.outputSteps,
                             earthGravitationalParameter,
                             departureEpoch );

    // Write sampled transfer orbit to file.
    std::ostringstream transferOrbitFilePath;
    transferOrbitFilePath << input.outputDirectory << "/transfer" << input.transferId << "_"
                          << input.transferOrbitFilename;
    std::ofstream transferOrbitFile( transferOrbitFilePath.str( ).c_str( ) );
    print( transferOrbitFile, transferOrbit, ephemerisFileHeader );
    transferOrbitFile.close( );

    // Write sampled transfer path to file.
    std::ostringstream transferPathFilePath;
    transferPathFilePath << input.outputDirectory << "/transfer" << input.transferId << "_"
                         << input.transferPathFilename;
    std::ofstream transferPathFile( transferPathFilePath.str( ).c_str( ) );
    print( transferPathFile, transferPath, ephemerisFileHeader );
    transferPathFile.close( );
}

//! Check input parameters for lambert_fetch.
LambertFetchInput checkLambertFetchInput( const rapidjson::Document& config )
{
    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const int transferId = find( config, "transfer_id" )->value.GetInt( );
    std::cout << "Transfer ID                   " << transferId << std::endl;

    const int outputSteps = find( config, "output_steps" )->value.GetInt( );
    std::cout << "Output steps                  " << outputSteps << std::endl;

    const std::string outputDirectory = find( config, "output_directory" )->value.GetString( );
    std::cout << "Output directory              " << outputDirectory << std::endl;

    const std::string metadataFilename = find( config, "metadata" )->value.GetString( );
    std::cout << "Metadata file                 " << metadataFilename << std::endl;

    const std::string departureOrbitFilename
        = find( config, "departure_orbit" )->value.GetString( );
    std::cout << "Departure orbit file          " << departureOrbitFilename << std::endl;

    const std::string departurePathFilename
        = find( config, "departure_path" )->value.GetString( );
    std::cout << "Departure path file           " << departurePathFilename << std::endl;

    const std::string arrivalOrbitFilename = find( config, "arrival_orbit" )->value.GetString( );
    std::cout << "Arrival orbit file            " << arrivalOrbitFilename << std::endl;

    const std::string arrivalPathFilename = find( config, "arrival_path" )->value.GetString( );
    std::cout << "Arrival path file             " << arrivalPathFilename << std::endl;

    const std::string transferOrbitFilename = find( config, "transfer_orbit" )->value.GetString( );
    std::cout << "Transfer orbit file           " << transferOrbitFilename << std::endl;

    const std::string transferPathFilename = find( config, "transfer_path" )->value.GetString( );
    std::cout << "Transfer path file            " << transferPathFilename << std::endl;

    return LambertFetchInput( databasePath,
                              transferId,
                              outputSteps,
                              outputDirectory,
                              metadataFilename,
                              departureOrbitFilename,
                              departurePathFilename,
                              arrivalOrbitFilename,
                              arrivalPathFilename,
                              transferOrbitFilename,
                              transferPathFilename );
}

} // namespace d2d
