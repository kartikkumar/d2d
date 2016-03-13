/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <fstream>
#include <iostream>
#include <sstream>

#include <keplerian_toolbox.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Astro/astro.hpp>

#include <Atom/convertCartesianStateToTwoLineElements.hpp>

#include <libsgp4/Tle.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/DateTime.h>

#include "D2D/sgp4Fetch.hpp"
#include "D2D/lambertFetch.hpp"
#include "D2D/tools.hpp"

namespace d2d
{

//! Fetch details of specific debris-to-debris SGP4 transfer.
void fetchSGP4Transfer( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const SGP4FetchInput input = checkSGP4FetchInput( config );

    // Set gravitational parameter used by SGP4 targeter.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s_2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                             Simulation                           " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    std::cout << "Fetching transfer from database ... " << std::endl;

    // Connect to database and fetch metadata required to construct SGP4TransferInput object.
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READONLY );

    // select transfer data from the lambert_scanner_results table in database
    std::ostringstream lambertTransferSelect;
    lambertTransferSelect << "SELECT * FROM lambert_scanner_results WHERE transfer_id = "
                   << input.transferId << ";";
    SQLite::Statement lambertQuery( database, lambertTransferSelect.str( ) );

    // select transfer data from the sgp4_scanner_results table in database
    std::ostringstream SGP4TransferSelect;
    SGP4TransferSelect << "SELECT * FROM sgp4_scanner_results WHERE lambert_transfer_id = "
                   << input.transferId << ";";
    SQLite::Statement SGP4Query( database, SGP4TransferSelect.str( ) );

    // retrieve the data from lambert_scanner_results
    lambertQuery.executeStep( );

    const int    departureObjectId                  = lambertQuery.getColumn( 1 );
    const int    arrivalObjectId                    = lambertQuery.getColumn( 2 );
    const double departureEpoch                     = lambertQuery.getColumn( 3 );
    const double timeOfFlight                       = lambertQuery.getColumn( 4 );
    const int    revolutions                        = lambertQuery.getColumn( 5 );
    const int    prograde                           = lambertQuery.getColumn( 6 );
    const double departurePositionX                 = lambertQuery.getColumn( 7 );
    const double departurePositionY                 = lambertQuery.getColumn( 8 );
    const double departurePositionZ                 = lambertQuery.getColumn( 9 );
    const double departureVelocityX                 = lambertQuery.getColumn( 10 );
    const double departureVelocityY                 = lambertQuery.getColumn( 11 );
    const double departureVelocityZ                 = lambertQuery.getColumn( 12 );
    const double arrivalPositionX                   = lambertQuery.getColumn( 19 );
    const double arrivalPositionY                   = lambertQuery.getColumn( 20 );
    const double arrivalPositionZ                   = lambertQuery.getColumn( 21 );
    const double arrivalVelocityX                   = lambertQuery.getColumn( 22 );
    const double arrivalVelocityY                   = lambertQuery.getColumn( 23 );
    const double arrivalVelocityZ                   = lambertQuery.getColumn( 24 );
    const double departureDeltaVX                   = lambertQuery.getColumn( 37 );
    const double departureDeltaVY                   = lambertQuery.getColumn( 38 );
    const double departureDeltaVZ                   = lambertQuery.getColumn( 39 );
    const double transferDeltaV                     = lambertQuery.getColumn( 43 );

    std::cout << "Lambert transfer successfully fetched from database!" << std::endl;

    // retrieve the data from the sgp4_scanner_results
    SGP4Query.executeStep( );

    const int lambertTransferId                     = SGP4Query.getColumn( 1 );
    const double sgp4ArrivalPositionX               = SGP4Query.getColumn( 2 );
    const double sgp4ArrivalPositionY               = SGP4Query.getColumn( 3 );
    const double sgp4ArrivalPositionZ               = SGP4Query.getColumn( 4 );
    const double sgp4ArrivalVelocityX               = SGP4Query.getColumn( 5 );
    const double sgp4ArrivalVelocityY               = SGP4Query.getColumn( 6 );
    const double sgp4ArrivalVelocityZ               = SGP4Query.getColumn( 7 );
    const double sgp4ArrivalPositionErrorX          = SGP4Query.getColumn( 8 );
    const double sgp4ArrivalPositionErrorY          = SGP4Query.getColumn( 9 );
    const double sgp4ArrivalPositionErrorZ          = SGP4Query.getColumn( 10 );
    const double sgp4ArrivalPositionError           = SGP4Query.getColumn( 11 );
    const double sgp4ArrivalVelocityErrorX          = SGP4Query.getColumn( 12 );
    const double sgp4ArrivalVelocityErrorY          = SGP4Query.getColumn( 13 );
    const double sgp4ArrivalVelocityErrorZ          = SGP4Query.getColumn( 14 );
    const double sgp4ArrivalVelocityError           = SGP4Query.getColumn( 15 );

    std::cout << "SGP4 transfer successfully fetched from database!" << std::endl;

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

    // compute and store transfer state history by sgp4 propagation
    std::vector< double > transferDepartureStateVector( 6 );
    for ( int i = 0; i < 6; i++ )
        transferDepartureStateVector[ i ] = transferDepartureState[ i ];
    
    DateTime transferDepartureEpoch( ( departureEpoch - 1721425.5 ) * TicksPerDay );
    // Tle transferOrbitTle = atom::convertCartesianStateToTwoLineElements< double, std::vector< double > >( transferDepartureStateVector,
    //                                                                                                       transferDepartureEpoch );
    // std::cout << "transfer Orbit Tle computed successfully!" << std::endl;

    // const StateHistory sgp4TransferPath = sampleSGP4Orbit( transferOrbitTle,
    //                                                        departureEpoch,
    //                                                        timeOfFlight,
    //                                                        input.outputSteps );

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
