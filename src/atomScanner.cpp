/*
 * Copyright (c) 2014-2015 Kartik Kumar (me@kartikkumar.com)
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
#include <Astro/astro.hpp>

#include "D2D/atomScanner.hpp"
#include "D2D/tools.hpp"

namespace d2d
{

//! Execute atom_scanner.
void executeAtomScanner( const rapidjson::Document& config )
{

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

    std::cout << "Parsing TLE catalog ... " << std::endl;

    // Parse catalog and store TLE objects.
    std::ifstream catalogFile( input.catalogPath.c_str( ) );
    std::string catalogLine;

    // Check if catalog is 2-line or 3-line version.
    std::getline( catalogFile, catalogLine );
    const int tleLines = getTleCatalogType( catalogLine );

    // Reset file stream to start of file.
    catalogFile.seekg( 0, std::ios::beg );

    typedef std::vector< std::string > TleStrings;
    typedef std::vector< Tle > TleObjects;
    TleObjects tleObjects;

    while ( std::getline( catalogFile, catalogLine ) )
    {
        TleStrings tleStrings;
        removeNewline( catalogLine );
        tleStrings.push_back( catalogLine );
        std::getline( catalogFile, catalogLine );
        removeNewline( catalogLine );
        tleStrings.push_back( catalogLine );

        if ( tleLines == 3 )
        {
            std::getline( catalogFile, catalogLine );
            removeNewline( catalogLine );
            tleStrings.push_back( catalogLine );
            tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ], tleStrings[ 2 ] ) );
        }

        else if ( tleLines == 2 )
        {
            tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ] ) );
        }
    }

    catalogFile.close( );
    std::cout << tleObjects.size( ) << " TLE objects parsed from catalog!" << std::endl;

    // Open database in read/write mode.
    SQLite::Database database( input.databasePath.c_str( ),
                               SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    // Create table for Atom scanner results in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createAtomScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

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

    SQLite::Statement query( database, atomScannerTableInsert.str( ) );

    std::cout << "Computing Atom transfers and populating database ... " << std::endl;

    // Loop over TLE objects and compute transfers based on Atom targeter across time-of-flight
    // grid.
    boost::progress_display showProgress( tleObjects.size( ) );

    // Loop over all departure objects.
    for ( unsigned int i = 0; i < tleObjects.size( ); i++ )
    {
        // Compute departure state.
        Tle departureObject = tleObjects[ i ];
        SGP4 sgp4Departure( departureObject );

        DateTime departureEpoch = input.departureEpoch;
        if ( input.departureEpoch == DateTime( ) )
        {
            departureEpoch = departureObject.Epoch( );
        }

        const Eci tleDepartureState = sgp4Departure.FindPosition( departureEpoch );
        const Vector6 departureState = getStateVector( tleDepartureState );

        Vector3 departurePosition;
        std::copy( departureState.begin( ),
                   departureState.begin( ) + 3,
                   departurePosition.begin( ) );

        Vector3 departureVelocity;
        std::copy( departureState.begin( ) + 3,
                   departureState.end( ),
                   departureVelocity.begin( ) );

        const Vector6 departureStateKepler
            = astro::convertCartesianToKeplerianElements( departureState,
                                                          earthGravitationalParameter );
        const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );

        // Loop over arrival objects.
        for ( unsigned int j = 0; j < tleObjects.size( ); j++ )
        {
            // Skip the case of the departure and arrival objects being the same.
            if ( i == j )
            {
                continue;
            }

            Tle arrivalObject = tleObjects[ j ];
            SGP4 sgp4Arrival( arrivalObject );
            const int arrivalObjectId = static_cast< int >( arrivalObject.NoradNumber( ) );

            // Loop over time-of-flight grid.
            for ( int k = 0; k < input.timeOfFlightSteps; k++ )
            {
                const double timeOfFlight
                    = input.timeOfFlightMinimum + k * input.timeOfFlightStepSize;

                const DateTime arrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
                const Eci tleArrivalState   = sgp4Arrival.FindPosition( arrivalEpoch );
                const Vector6 arrivalState  = getStateVector( tleArrivalState );

                Vector3 arrivalPosition;
                std::copy( arrivalState.begin( ),
                           arrivalState.begin( ) + 3,
                           arrivalPosition.begin( ) );

                Vector3 arrivalVelocity;
                std::copy( arrivalState.begin( ) + 3,
                           arrivalState.end( ),
                           arrivalVelocity.begin( ) );
                const Vector6 arrivalStateKepler
                    = astro::convertCartesianToKeplerianElements( arrivalState,
                                                                  earthGravitationalParameter );

                kep_toolbox::lambert_problem targeter( departurePosition,
                                                       arrivalPosition,
                                                       timeOfFlight,
                                                       earthGravitationalParameter,
                                                       !input.isPrograde,
                                                       input.revolutionsMaximum );

                const int numberOfSolutions = targeter.get_v1( ).size( );

                // Compute Delta-Vs for transfer and determine index of lowest.
                typedef std::vector< Vector3 > VelocityList;
                VelocityList departureDeltaVs( numberOfSolutions );
                VelocityList arrivalDeltaVs( numberOfSolutions );

                typedef std::vector< double > TransferDeltaVList;
                TransferDeltaVList transferDeltaVs( numberOfSolutions );

                for ( int i = 0; i < numberOfSolutions; i++ )
                {
                    // Compute Delta-V for transfer.
                    const Vector3 transferDepartureVelocity = targeter.get_v1( )[ i ];
                    const Vector3 transferArrivalVelocity = targeter.get_v2( )[ i ];

                    departureDeltaVs[ i ] = sml::add( transferDepartureVelocity,
                                                      sml::multiply( departureVelocity, -1.0 ) );
                    arrivalDeltaVs[ i ]   = sml::add( transferArrivalVelocity,
                                                      sml::multiply( arrivalVelocity, -1.0 ) );

                    transferDeltaVs[ i ]
                        = sml::norm< double >( departureDeltaVs[ i ] )
                            + sml::norm< double >( arrivalDeltaVs[ i ] );
                }

                const TransferDeltaVList::iterator minimumDeltaVIterator
                    = std::min_element( transferDeltaVs.begin( ), transferDeltaVs.end( ) );
                const int minimumDeltaVIndex
                    = std::distance( transferDeltaVs.begin( ), minimumDeltaVIterator );

                const int revolutions = std::floor( ( minimumDeltaVIndex + 1 ) / 2 );

                Vector6 transferState;
                std::copy( departurePosition.begin( ),
                           departurePosition.begin( ) + 3,
                           transferState.begin( ) );
                std::copy( targeter.get_v1( )[ minimumDeltaVIndex ].begin( ),
                           targeter.get_v1( )[ minimumDeltaVIndex ].begin( ) + 3,
                           transferState.begin( ) + 3 );

                const Vector6 transferStateKepler
                    = astro::convertCartesianToKeplerianElements( transferState,
                                                                  earthGravitationalParameter );

                // Bind values to SQL insert query.
                query.bind( ":departure_object_id",  departureObjectId );
                query.bind( ":arrival_object_id",    arrivalObjectId );
                query.bind( ":departure_epoch",      departureEpoch.ToJulian( ) );
                query.bind( ":time_of_flight",       timeOfFlight );
                query.bind( ":revolutions",          revolutions );
                query.bind( ":prograde",             input.isPrograde );
                query.bind( ":departure_position_x", departureState[ astro::xPositionIndex ] );
                query.bind( ":departure_position_y", departureState[ astro::yPositionIndex ] );
                query.bind( ":departure_position_z", departureState[ astro::zPositionIndex ] );
                query.bind( ":departure_velocity_x", departureState[ astro::xVelocityIndex ] );
                query.bind( ":departure_velocity_y", departureState[ astro::yVelocityIndex ] );
                query.bind( ":departure_velocity_z", departureState[ astro::zVelocityIndex ] );
                query.bind( ":departure_semi_major_axis",
                    departureStateKepler[ astro::semiMajorAxisIndex ] );
                query.bind( ":departure_eccentricity",
                    departureStateKepler[ astro::eccentricityIndex ] );
                query.bind( ":departure_inclination",
                    departureStateKepler[ astro::inclinationIndex ] );
                query.bind( ":departure_argument_of_periapsis",
                    departureStateKepler[ astro::argumentOfPeriapsisIndex ] );
                query.bind( ":departure_longitude_of_ascending_node",
                    departureStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                query.bind( ":departure_true_anomaly",
                    departureStateKepler[ astro::trueAnomalyIndex ] );
                query.bind( ":arrival_position_x",  arrivalState[ astro::xPositionIndex ] );
                query.bind( ":arrival_position_y",  arrivalState[ astro::yPositionIndex ] );
                query.bind( ":arrival_position_z",  arrivalState[ astro::zPositionIndex ] );
                query.bind( ":arrival_velocity_x",  arrivalState[ astro::xVelocityIndex ] );
                query.bind( ":arrival_velocity_y",  arrivalState[ astro::yVelocityIndex ] );
                query.bind( ":arrival_velocity_z",  arrivalState[ astro::zVelocityIndex ] );
                query.bind( ":arrival_semi_major_axis",
                    arrivalStateKepler[ astro::semiMajorAxisIndex ] );
                query.bind( ":arrival_eccentricity",
                    arrivalStateKepler[ astro::eccentricityIndex ] );
                query.bind( ":arrival_inclination",
                    arrivalStateKepler[ astro::inclinationIndex ] );
                query.bind( ":arrival_argument_of_periapsis",
                    arrivalStateKepler[ astro::argumentOfPeriapsisIndex ] );
                query.bind( ":arrival_longitude_of_ascending_node",
                    arrivalStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                query.bind( ":arrival_true_anomaly",
                    arrivalStateKepler[ astro::trueAnomalyIndex ] );
                query.bind( ":transfer_semi_major_axis",
                    transferStateKepler[ astro::semiMajorAxisIndex ] );
                query.bind( ":transfer_eccentricity",
                    transferStateKepler[ astro::eccentricityIndex ] );
                query.bind( ":transfer_inclination",
                    transferStateKepler[ astro::inclinationIndex ] );
                query.bind( ":transfer_argument_of_periapsis",
                    transferStateKepler[ astro::argumentOfPeriapsisIndex ] );
                query.bind( ":transfer_longitude_of_ascending_node",
                    transferStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                query.bind( ":transfer_true_anomaly",
                    transferStateKepler[ astro::trueAnomalyIndex ] );
                query.bind( ":departure_delta_v_x", departureDeltaVs[ minimumDeltaVIndex ][ 0 ] );
                query.bind( ":departure_delta_v_y", departureDeltaVs[ minimumDeltaVIndex ][ 1 ] );
                query.bind( ":departure_delta_v_z", departureDeltaVs[ minimumDeltaVIndex ][ 2 ] );
                query.bind( ":arrival_delta_v_x",   arrivalDeltaVs[ minimumDeltaVIndex ][ 0 ] );
                query.bind( ":arrival_delta_v_y",   arrivalDeltaVs[ minimumDeltaVIndex ][ 1 ] );
                query.bind( ":arrival_delta_v_z",   arrivalDeltaVs[ minimumDeltaVIndex ][ 2 ] );
                query.bind( ":transfer_delta_v",    *minimumDeltaVIterator );

                // Execute insert query.
                query.executeStep( );

                // Reset SQL insert query.
                query.reset( );
            }
        }

        ++showProgress;
    }

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
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const ConfigIterator departureEpochIterator = find( config, "departure_epoch" );
    std::map< std::string, int > departureEpochElements;
    if ( departureEpochIterator->value.Size( ) == 0 )
    {
        DateTime dummyEpoch;
        departureEpochElements[ "year" ]    = dummyEpoch.Year( );
        departureEpochElements[ "month" ]   = dummyEpoch.Month( );
        departureEpochElements[ "day" ]     = dummyEpoch.Day( );
        departureEpochElements[ "hours" ]   = dummyEpoch.Hour( );
        departureEpochElements[ "minutes" ] = dummyEpoch.Minute( );
        departureEpochElements[ "seconds" ] = dummyEpoch.Second( );
    }

    else
    {
        departureEpochElements[ "year" ]    = departureEpochIterator->value[ 0 ].GetInt( );
        departureEpochElements[ "month" ]   = departureEpochIterator->value[ 1 ].GetInt( );
        departureEpochElements[ "day" ]     = departureEpochIterator->value[ 2 ].GetInt( );

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

    if ( departureEpochIterator->value.Size( ) == 0 )
    {
        std::cout << "Departure epoch               TLE epoch" << std::endl;
    }

    else
    {
        std::cout << "Departure epoch               " << departureEpoch << std::endl;
    }

    const double timeOfFlightMinimum
        = find( config, "time_of_flight_grid" )->value[ 0 ].GetDouble( );
    std::cout << "Minimum Time-of-Flight        " << timeOfFlightMinimum << std::endl;
    const double timeOfFlightMaximum
        = find( config, "time_of_flight_grid" )->value[ 1 ].GetDouble( );
    std::cout << "Maximum Time-of-Flight        " << timeOfFlightMaximum << std::endl;

    if ( timeOfFlightMinimum > timeOfFlightMaximum )
    {
        throw std::runtime_error( "ERROR: Maximum time-of-flight must be larger than minimum!" );
    }

    const double timeOfFlightSteps
        = find( config, "time_of_flight_grid" )->value[ 2 ].GetDouble( );
    std::cout << "# Time-of-Flight steps        " << timeOfFlightSteps << std::endl;

    const bool isPrograde = find( config, "is_prograde" )->value.GetBool( );
    if ( isPrograde )
    {
        std::cout << "Prograde transfer?            true" << std::endl;
    }
    else
    {
        std::cout << "Prograde transfer?            false" << std::endl;
    }

    const int revolutionsMaximum = find( config, "revolutions_maximum" )->value.GetInt( );
    std::cout << "Maximum revolutions           " << revolutionsMaximum << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                     " << shortlistPath << std::endl;
    }

    return AtomScannerInput( catalogPath,
                                databasePath,
                                departureEpoch,
                                timeOfFlightMinimum,
                                timeOfFlightMaximum,
                                timeOfFlightSteps,
                                ( timeOfFlightMaximum - timeOfFlightMinimum ) / timeOfFlightSteps,
                                isPrograde,
                                revolutionsMaximum,
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
