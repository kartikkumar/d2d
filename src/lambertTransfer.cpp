/*
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <fstream>
// #include <iomanip>
#include <iostream>
// #include <limits>
#include <map>
#include <sstream>
// #include <vector>

#include <keplerian_toolbox.h>

#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>

#include <SML/sml.hpp>

#include <Astro/astro.hpp>

#include "D2D/lambertTransfer.hpp"
#include "D2D/tools.hpp"

namespace d2d
{

//! Execute single Lambert transfer.
void executeLambertTransfer( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertTransferInput input = checkLambertTransferInput( config );

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                             Simulation                           " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    SGP4 sgp4Departure( input.departureObject );
    Eci tleDepartureState = sgp4Departure.FindPosition( input.departureEpoch );

    Vector6 departureState;
    departureState[ astro::xPositionIndex ] = tleDepartureState.Position( ).x;
    departureState[ astro::yPositionIndex ] = tleDepartureState.Position( ).y;
    departureState[ astro::zPositionIndex ] = tleDepartureState.Position( ).z;
    departureState[ astro::xVelocityIndex ] = tleDepartureState.Velocity( ).x;
    departureState[ astro::yVelocityIndex ] = tleDepartureState.Velocity( ).y;
    departureState[ astro::zVelocityIndex ] = tleDepartureState.Velocity( ).z;

    Vector3 departurePosition;
    std::copy( departureState.begin( ), departureState.begin( ) + 3, departurePosition.begin( ) );

    Vector3 departureVelocity;
    std::copy( departureState.begin( ) + 3, departureState.end( ), departureVelocity.begin( ) );

    SGP4 sgp4Arrival( input.arrivalObject );
    const DateTime arrivalEpoch = input.departureEpoch.AddSeconds( input.timeOfFlight );
    Eci tleArrivalState = sgp4Arrival.FindPosition( arrivalEpoch );

    Vector6 arrivalState;
    arrivalState[ astro::xPositionIndex ] = tleArrivalState.Position( ).x;
    arrivalState[ astro::yPositionIndex ] = tleArrivalState.Position( ).y;
    arrivalState[ astro::zPositionIndex ] = tleArrivalState.Position( ).z;
    arrivalState[ astro::xVelocityIndex ] = tleArrivalState.Velocity( ).x;
    arrivalState[ astro::yVelocityIndex ] = tleArrivalState.Velocity( ).y;
    arrivalState[ astro::zVelocityIndex ] = tleArrivalState.Velocity( ).z;

    Vector3 arrivalPosition;
    std::copy( arrivalState.begin( ), arrivalState.begin( ) + 3, arrivalPosition.begin( ) );

    Vector3 arrivalVelocity;
    std::copy( arrivalState.begin( ) + 3, arrivalState.end( ), arrivalVelocity.begin( ) );

    const double earthGravitationalParameter = kMU;

    std::cout << "Computing Lambert transfer ... " << std::endl;

    kep_toolbox::lambert_problem targeter( departurePosition,
                                           arrivalPosition,
                                           input.timeOfFlight,
                                           earthGravitationalParameter,
                                           !input.isPrograde,
                                           input.revolutionsMaximum );

    const int numberOfSolutions = targeter.get_v1( ).size( );

    std::cout << "Total number of solutions computed: " << numberOfSolutions << std::endl;
    if ( numberOfSolutions > 1 )
    {
        if ( input.solutionOutput.compare( "all" ) == 0 )
        {
            std::cout << "All solutions found will be written to file ... " << std::endl;
        }

        else if ( input.solutionOutput.compare( "best" ) == 0 )
        {
            std::cout << "The lowest Delta-V solution found will be written to file ... "
                      << std::endl;
        }
    }

    else
    {
        std::cout << "Solution found will be written to file ... " << std::endl;
    }

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                               Output                             " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    std::cout << "Writing output files ... " << std::endl;
    std::cout << std::endl;

    for ( int i = 0; i < numberOfSolutions; i++ )
    {
        const int solutionId = i + 1;
        const int revolutions = static_cast< int >( ( i + 1 ) / 2 );
        std::cout << "Writing solution #" << solutionId << " ... " << std::endl;

        // Compute total Delta-V for transfer.
        const Vector3 transferDepartureVelocity = targeter.get_v1( )[ i ];
        const Vector3 transferArrivalVelocity = targeter.get_v2( )[ i ];
        const double transferDeltaV
            = sml::norm< double >(
                sml::add( transferDepartureVelocity, sml::multiply( departureVelocity, -1.0 ) ) );
              + sml::norm< double >(
                    sml::add( transferArrivalVelocity, sml::multiply( arrivalVelocity, -1.0 ) ) );

        // Write metadata to file.
        std::ostringstream metadataPath;
        metadataPath << input.outputDirectory << "/sol" << solutionId << "_"
                     << input.metadataFilename;
        std::ofstream metadataFile( metadataPath.str( ).c_str( ) );
        printMetaParameter(
            metadataFile, "departure_id", input.departureObject.NoradNumber( ), "-" );
        printMetaParameter( metadataFile, "arrival_id", input.arrivalObject.NoradNumber( ), "-" );
        printMetaParameter(
            metadataFile, "departure_epoch", input.departureEpoch.ToJulian( ), "JD" );
        printMetaParameter( metadataFile, "time_of_flight", targeter.get_tof( ), "s" );
        printMetaParameter( metadataFile, "is_prograde", input.isPrograde, "-" );
        printMetaParameter( metadataFile, "revolutions", revolutions, "-" );
        printMetaParameter( metadataFile, "transfer_delta_v", transferDeltaV, "km/s" );
        metadataFile.close( );

        // Defined common header line for all the ephemeric files generated below.
        const std::string ephemerisFileHeader = "jd,x,y,z,xdot,ydot,zdot";

        // Compute period of departure orbit.
        const Vector6 departureStateKepler = astro::convertCartesianToKeplerianElements(
            departureState, earthGravitationalParameter );
        const double departureOrbitalPeriod = astro::computeKeplerOrbitalPeriod(
            departureStateKepler[ astro::semiMajorAxisIndex ], earthGravitationalParameter );

        // Sample departure orbit.
        const StateHistory departureOrbit = sampleKeplerOrbit( departureState,
                                                               departureOrbitalPeriod,
                                                               input.outputSteps,
                                                               earthGravitationalParameter,
                                                               input.departureEpoch.ToJulian( ) );

        // Write sampled departure orbit to file.
        std::ostringstream departureOrbitFilePath;
        departureOrbitFilePath << input.outputDirectory << "/sol" << solutionId << "_"
                               << input.departureOrbitFilename;
        std::ofstream departureOrbitFile( departureOrbitFilePath.str( ).c_str( ) );
        printStateHistory( departureOrbitFile, departureOrbit, ephemerisFileHeader );
        departureOrbitFile.close( );

        // Sample departure path.
        const StateHistory departurePath
            = sampleKeplerOrbit( departureState,
                                 input.timeOfFlight,
                                 input.outputSteps,
                                 earthGravitationalParameter,
                                 input.departureEpoch.ToJulian( ) );


        // Write sampled departure path to file.
        std::ostringstream departurePathFilePath;
        departurePathFilePath << input.outputDirectory << "/sol" << solutionId << "_"
                              << input.departurePathFilename;
        std::ofstream departurePathFile( departurePathFilePath.str( ).c_str( ) );
        printStateHistory( departurePathFile, departurePath, ephemerisFileHeader );
        departurePathFile.close( );

        // Compute period of arrival orbit.
        const Vector6 arrivalStateKepler = astro::convertCartesianToKeplerianElements(
            arrivalState, earthGravitationalParameter );
        const double arrivalOrbitalPeriod = astro::computeKeplerOrbitalPeriod(
            arrivalStateKepler[ astro::semiMajorAxisIndex ], earthGravitationalParameter );

        // Sample arrival orbit.
        const StateHistory arrivalOrbit = sampleKeplerOrbit( arrivalState,
                                                             arrivalOrbitalPeriod,
                                                             input.outputSteps,
                                                             earthGravitationalParameter,
                                                             input.departureEpoch.ToJulian( ) );

        // Write sampled arrival orbit to file.
        std::ostringstream arrivalOrbitFilePath;
        arrivalOrbitFilePath << input.outputDirectory << "/sol" << solutionId << "_"
                             << input.arrivalOrbitFilename;
        std::ofstream arrivalOrbitFile( arrivalOrbitFilePath.str( ).c_str( ) );
        printStateHistory( arrivalOrbitFile, arrivalOrbit, ephemerisFileHeader );
        arrivalOrbitFile.close( );

        // Sample arrival path.
        const StateHistory arrivalPath = sampleKeplerOrbit( arrivalState,
                                                            input.timeOfFlight,
                                                            input.outputSteps,
                                                            earthGravitationalParameter,
                                                            input.departureEpoch.ToJulian( ) );

        // Write sampled arrival path to file.
        std::ostringstream arrivalPathFilePath;
        arrivalPathFilePath << input.outputDirectory << "/sol" << solutionId << "_"
                            << input.arrivalPathFilename;
        std::ofstream arrivalPathFile( arrivalPathFilePath.str( ).c_str( ) );
        printStateHistory( arrivalPathFile, arrivalPath, ephemerisFileHeader );
        arrivalPathFile.close( );

        // Sample transfer trajectory.
        Vector6 transferDepartureState;
        std::copy( departurePosition.begin( ),
                   departurePosition.begin( ) + 3,
                   transferDepartureState.begin( ) );
        std::copy( targeter.get_v1( )[ i ].begin( ),
                   targeter.get_v1( )[ i ].begin( ) + 3,
                   transferDepartureState.begin( ) + 3 );

        // Compute period of transfer orbit.
        const Vector6 transferDepartureStateKepler = astro::convertCartesianToKeplerianElements(
            transferDepartureState, earthGravitationalParameter );
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
                                 input.departureEpoch.ToJulian( ) );

        // Write sampled transfer orbit to file.
        std::ostringstream transferOrbitFilePath;
        transferOrbitFilePath << input.outputDirectory << "/sol" << solutionId << "_"
                              << input.transferOrbitFilename;
        std::ofstream transferOrbitFile( transferOrbitFilePath.str( ).c_str( ) );
        printStateHistory( transferOrbitFile, transferOrbit, ephemerisFileHeader );
        transferOrbitFile.close( );

        // Sample transfer path.
        const StateHistory transferPath = sampleKeplerOrbit( transferDepartureState,
                                                             input.timeOfFlight,
                                                             input.outputSteps,
                                                             earthGravitationalParameter,
                                                             input.departureEpoch.ToJulian( ) );

        // Write sampled transfer path to file.
        std::ostringstream transferPathFilePath;
        transferPathFilePath << input.outputDirectory << "/sol" << solutionId << "_"
                             << input.transferPathFilename;
        std::ofstream transferPathFile( transferPathFilePath.str( ).c_str( ) );
        printStateHistory( transferPathFile, transferPath, ephemerisFileHeader );
        transferPathFile.close( );
    }

    std::cout << std::endl;
    std::cout << "Output successfully written to file!" << std::endl;
}

//! Check input parameters Lambert transfer.
LambertTransferInput checkLambertTransferInput( const rapidjson::Document& config )
{
    const std::string departureTleLine0
        = findParameter( config, "departure_tle_line0" )->value.GetString( );
    std::cout << "Departure TLE Line 0          " << departureTleLine0 << std::endl;
    const std::string departureTleLine1
        = findParameter( config, "departure_tle_line1" )->value.GetString( );
    std::cout << "Departure TLE Line 1          " << departureTleLine1 << std::endl;
    const std::string departureTleLine2
        = findParameter( config, "departure_tle_line2" )->value.GetString( );
    std::cout << "Departure TLE Line 2          " << departureTleLine2 << std::endl;
    const Tle departureObject( departureTleLine0, departureTleLine1, departureTleLine2 );

    const std::string arrivalTleLine0
        = findParameter( config, "arrival_tle_line0" )->value.GetString( );
    std::cout << "Arrival TLE Line 0            " << arrivalTleLine0 << std::endl;
    const std::string arrivalTleLine1
        = findParameter( config, "arrival_tle_line1" )->value.GetString( );
    std::cout << "Arrival TLE Line 1            " << arrivalTleLine1 << std::endl;
    const std::string arrivalTleLine2
        = findParameter( config, "arrival_tle_line2" )->value.GetString( );
    std::cout << "Arrival TLE Line 2            " << arrivalTleLine2 << std::endl;
    const Tle arrivalObject( arrivalTleLine0, arrivalTleLine1, arrivalTleLine2 );

    const ConfigIterator departureEpochIterator = findParameter( config, "departure_epoch" );
    std::map< std::string, int > departureEpochElements;
    if ( departureEpochIterator->value.Size( ) == 0 )
    {
        departureEpochElements[ "year" ]    = departureObject.Epoch( ).Year( );
        departureEpochElements[ "month" ]   = departureObject.Epoch( ).Month( );
        departureEpochElements[ "day" ]     = departureObject.Epoch( ).Day( );
        departureEpochElements[ "hours" ]   = departureObject.Epoch( ).Hour( );
        departureEpochElements[ "minutes" ] = departureObject.Epoch( ).Minute( );
        departureEpochElements[ "seconds" ] = departureObject.Epoch( ).Second( );
    }

    else
    {
        departureEpochElements[ "year" ]  = departureEpochIterator->value[ 0 ].GetInt( );
        departureEpochElements[ "month" ] = departureEpochIterator->value[ 1 ].GetInt( );
        departureEpochElements[ "day" ]   = departureEpochIterator->value[ 2 ].GetInt( );

        if ( departureEpochIterator->value.Size( ) > 3 )
        {
            departureEpochElements[ "hours" ] = departureEpochIterator->value[ 3 ].GetInt( );

            if ( departureEpochIterator->value.Size( ) > 4 )
            {
                departureEpochElements[ "minutes" ] = departureEpochIterator->value[ 4 ].GetInt( );

                if ( departureEpochIterator->value.Size( ) > 5 )
                {
                    departureEpochElements[ "seconds" ]
                        = departureEpochIterator->value[ 5 ].GetInt( );
                }
            }
        }
    }

    const DateTime departureEpoch( departureEpochElements[ "year" ],
                                   departureEpochElements[ "month" ],
                                   departureEpochElements[ "day" ],
                                   departureEpochElements[ "hours" ],
                                   departureEpochElements[ "minutes" ],
                                   departureEpochElements[ "seconds" ] );
    std::cout << "Departure epoch               " << departureEpoch << std::endl;

    const double timeOfFlight = findParameter( config, "time_of_flight" )->value.GetDouble( );
    std::cout << "Time-of-Flight                " << timeOfFlight << std::endl;

    const bool isPrograde = findParameter( config, "is_prograde" )->value.GetBool( );
    std::cout << "Prograde?                     ";
    if ( !isPrograde )
    {
        std::cout << "true" << std::endl;
    }
    else
    {
        std::cout << "false" << std::endl;
    }

    const int revolutionsMaximum = findParameter( config, "revolutions_maximum" )->value.GetInt( );
    std::cout << "Revolutions (max)             " << revolutionsMaximum << std::endl;

    std::string solutionOutput = findParameter( config, "solution_output" )->value.GetString( );
    std::transform( solutionOutput.begin( ), solutionOutput.end( ),
                    solutionOutput.begin( ), ::tolower );
    std::cout << "Solution output               " << solutionOutput << std::endl;

    const int outputSteps = findParameter( config, "output_steps" )->value.GetInt( );
    std::cout << "Output steps                  " << outputSteps << std::endl;

    const std::string outputDirectory
        = findParameter( config, "output_directory" )->value.GetString( );
    std::cout << "Output directory              " << outputDirectory << std::endl;

    const std::string metadataFilename = findParameter( config, "metadata" )->value.GetString( );
    std::cout << "Metadata file                 " << metadataFilename << std::endl;

    const std::string departureOrbitFilename
        = findParameter( config, "departure_orbit" )->value.GetString( );
    std::cout << "Departure orbit file          " << departureOrbitFilename << std::endl;

    const std::string departurePathFilename
        = findParameter( config, "departure_path" )->value.GetString( );
    std::cout << "Departure path file           " << departurePathFilename << std::endl;

    const std::string arrivalOrbitFilename
        = findParameter( config, "arrival_orbit" )->value.GetString( );
    std::cout << "Arrival orbit file            " << arrivalOrbitFilename << std::endl;

    const std::string arrivalPathFilename
        = findParameter( config, "arrival_path" )->value.GetString( );
    std::cout << "Arrival path file             " << arrivalPathFilename << std::endl;

    const std::string transferOrbitFilename
        = findParameter( config, "transfer_orbit" )->value.GetString( );
    std::cout << "Transfer orbit file           " << transferOrbitFilename << std::endl;

    const std::string transferPathFilename
        = findParameter( config, "transfer_path" )->value.GetString( );
    std::cout << "Transfer path file            " << transferPathFilename << std::endl;

    return LambertTransferInput( departureObject,
                                 arrivalObject,
                                 departureEpoch,
                                 timeOfFlight,
                                 isPrograde,
                                 revolutionsMaximum,
                                 solutionOutput,
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