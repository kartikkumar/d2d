/*
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
// #include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
// #include <numeric>
#include <sstream>
#include <vector>

#include <boost/progress.hpp>

// #include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <SML/sml.hpp>
#include <Astro/astro.hpp>

#include "D2D/lambertScanner.hpp"
#include "D2D/tools.hpp"

namespace d2d
{

//! Execute Lambert scanner.
void executeLambertScanner( const rapidjson::Document& config )
{

    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertScannerInput input = checkLambertScannerInput( config );

    // Set gravitational parameter used by Lambert targeter.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s_2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    std::cout << "Parsing TLE catalog ... " << std::endl;

    // Parse catalog and store TLE objects.
    std::ifstream catalogFile( input.catalogPath.c_str( ) );
    std::string catalogLine;
    typedef std::vector< std::string > TleStrings;
    typedef std::vector< Tle > TleObjects;
    TleObjects tleObjects;

    while ( std::getline( catalogFile, catalogLine ) )
    {
        TleStrings tleStrings;
        tleStrings.push_back( catalogLine );
        std::getline( catalogFile, catalogLine );
        tleStrings.push_back( catalogLine );

        if ( input.tleLines == 3 )
        {
            std::getline( catalogFile, catalogLine );
            tleStrings.push_back( catalogLine );
            tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ], tleStrings[ 2 ] ) );
        }

        else
        {
            tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ] ) );
        }
    }

    catalogFile.close( );
    std::cout << tleObjects.size( ) << " TLE objects parsed from catalog!" << std::endl;

    // Open database in read/write mode.
    SQLite::Database database( input.databasePath.c_str( ),
                               SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    // Create table for Lambert scanner results in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createLambertScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Setup insert query.
    std::ostringstream lambertScannerTableInsert;
    lambertScannerTableInsert
        << "INSERT INTO lambert_scanner VALUES ("
        << "NULL,"
        << ":departureObjectId,"
        << ":departureEpoch,"
        << ":departurePositionX,"
        << ":departurePositionY,"
        << ":departurePositionZ,"
        << ":departureVelocityX,"
        << ":departureVelocityY,"
        << ":departureVelocityZ,"
        << ":departureSemiMajorAxis,"
        << ":departureEccentricity,"
        << ":departureInclination,"
        << ":departureArgumentOfPeriapsis,"
        << ":departureLongitudeOfAscendingNode,"
        << ":departureTrueAnomaly,"
        << ":departureDeltaVX,"
        << ":departureDeltaVY,"
        << ":departureDeltaVZ,"
        << ":arrivalObjectId,"
        << ":arrivalPositionX,"
        << ":arrivalPositionY,"
        << ":arrivalPositionZ,"
        << ":arrivalVelocityX,"
        << ":arrivalVelocityY,"
        << ":arrivalVelocityZ,"
        << ":arrivalSemiMajorAxis,"
        << ":arrivalEccentricity,"
        << ":arrivalInclination,"
        << ":arrivalArgumentOfPeriapsis,"
        << ":arrivalLongitudeOfAscendingNode,"
        << ":arrivalTrueAnomaly,"
        << ":arrivalDeltaVX,"
        << ":arrivalDeltaVY,"
        << ":arrivalDeltaVZ,"
        << ":timeOfFlight,"
        << ":revolutions,"
        << ":transferSemiMajorAxis,"
        << ":transferEccentricity,"
        << ":transferInclination,"
        << ":transferArgumentOfPeriapsis,"
        << ":transferLongitudeOfAscendingNode,"
        << ":transferTrueAnomaly,"
        << ":transferDeltaV"
        << ");";

    SQLite::Statement query( database, lambertScannerTableInsert.str( ) );

    std::cout << "Computing Lambert transfers and populating database ... " << std::endl;

    // Loop over TLE objects and compute transfers based on Lambert targeter across time-of-flight
    // grid.
    boost::progress_display show_progress( tleObjects.size( ) );

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
                const Eci tleArrivalState = sgp4Arrival.FindPosition( arrivalEpoch );
                const Vector6 arrivalState = getStateVector( tleArrivalState );

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
                    arrivalDeltaVs[ i ] = sml::add( transferArrivalVelocity,
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
                query.bind( ":departureObjectId", departureObjectId );
                query.bind( ":departureEpoch", departureEpoch.ToJulian( ) );
                query.bind( ":departurePositionX", departureState[ astro::xPositionIndex ] );
                query.bind( ":departurePositionY", departureState[ astro::yPositionIndex ] );
                query.bind( ":departurePositionZ", departureState[ astro::zPositionIndex ] );
                query.bind( ":departureVelocityX", departureState[ astro::xVelocityIndex ] );
                query.bind( ":departureVelocityY", departureState[ astro::yVelocityIndex ] );
                query.bind( ":departureVelocityZ", departureState[ astro::zVelocityIndex ] );
                query.bind( ":departureSemiMajorAxis",
                    departureStateKepler[ astro::semiMajorAxisIndex ] );
                query.bind( ":departureEccentricity",
                    departureStateKepler[ astro::eccentricityIndex ] );
                query.bind( ":departureInclination",
                    departureStateKepler[ astro::inclinationIndex ] );
                query.bind( ":departureArgumentOfPeriapsis",
                    departureStateKepler[ astro::argumentOfPeriapsisIndex ] );
                query.bind( ":departureLongitudeOfAscendingNode",
                    departureStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                query.bind( ":departureTrueAnomaly",
                    departureStateKepler[ astro::trueAnomalyIndex ] );
                query.bind( ":departureDeltaVX", departureDeltaVs[ minimumDeltaVIndex ][ 0 ] );
                query.bind( ":departureDeltaVY", departureDeltaVs[ minimumDeltaVIndex ][ 1 ] );
                query.bind( ":departureDeltaVZ", departureDeltaVs[ minimumDeltaVIndex ][ 2 ] );
                query.bind( ":arrivalObjectId", arrivalObjectId );
                query.bind( ":arrivalPositionX", arrivalState[ astro::xPositionIndex ] );
                query.bind( ":arrivalPositionY", arrivalState[ astro::yPositionIndex ] );
                query.bind( ":arrivalPositionZ", arrivalState[ astro::zPositionIndex ] );
                query.bind( ":arrivalVelocityX", arrivalState[ astro::xVelocityIndex ] );
                query.bind( ":arrivalVelocityY", arrivalState[ astro::yVelocityIndex ] );
                query.bind( ":arrivalVelocityZ", arrivalState[ astro::zVelocityIndex ] );
                query.bind( ":arrivalSemiMajorAxis",
                    arrivalStateKepler[ astro::semiMajorAxisIndex ] );
                query.bind( ":arrivalEccentricity",
                    arrivalStateKepler[ astro::eccentricityIndex ] );
                query.bind( ":arrivalInclination",
                    arrivalStateKepler[ astro::inclinationIndex ] );
                query.bind( ":arrivalArgumentOfPeriapsis",
                    arrivalStateKepler[ astro::argumentOfPeriapsisIndex ] );
                query.bind( ":arrivalLongitudeOfAscendingNode",
                    arrivalStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                query.bind( ":arrivalTrueAnomaly",
                    arrivalStateKepler[ astro::trueAnomalyIndex ] );
                query.bind( ":arrivalDeltaVX", arrivalDeltaVs[ minimumDeltaVIndex ][ 0 ] );
                query.bind( ":arrivalDeltaVY", arrivalDeltaVs[ minimumDeltaVIndex ][ 1 ] );
                query.bind( ":arrivalDeltaVZ", arrivalDeltaVs[ minimumDeltaVIndex ][ 2 ] );
                query.bind( ":timeOfFlight", timeOfFlight );
                query.bind( ":revolutions", revolutions );
                query.bind( ":transferSemiMajorAxis",
                    transferStateKepler[ astro::semiMajorAxisIndex ] );
                query.bind( ":transferEccentricity",
                    transferStateKepler[ astro::eccentricityIndex ] );
                query.bind( ":transferInclination",
                    transferStateKepler[ astro::inclinationIndex ] );
                query.bind( ":transferArgumentOfPeriapsis",
                    transferStateKepler[ astro::argumentOfPeriapsisIndex ] );
                query.bind( ":transferLongitudeOfAscendingNode",
                    transferStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                query.bind( ":transferTrueAnomaly",
                    transferStateKepler[ astro::trueAnomalyIndex ] );
                query.bind( ":transferDeltaV", *minimumDeltaVIterator );

                // Execute insert query.
                query.executeStep( );

                // Reset SQL insert query.
                query.reset( );
            }
        }

        ++show_progress;
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
        writeTransferShortlist( database, input.shortlistLength, input.shortlistPath );
        std::cout << "Shortlist file created successfully!" << std::endl;
    }
}

//! Check Lambert scanner input parameters.
LambertScannerInput checkLambertScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const int tleLines = find( config, "tle_lines" )->value.GetInt( );
    std::cout << "# of lines per TLE            " << tleLines << std::endl;

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

    return LambertScannerInput( catalogPath,
                                tleLines,
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

//! Create Lambert scanner table.
void createLambertScannerTable( SQLite::Database& database )
{
    // Set up SQL command to create table to store Lambert scanner results.
    std::ostringstream lambertScannerTableCreate;
    lambertScannerTableCreate
        << "CREATE TABLE IF NOT EXISTS lambert_scanner ("
        << "\"transferId\" INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"departureObjectId\" INTEGER,"
        << "\"departureEpoch\" REAL,"
        << "\"departurePositionX\" REAL,"
        << "\"departurePositionY\" REAL,"
        << "\"departurePositionZ\" REAL,"
        << "\"departureVelocityX\" REAL,"
        << "\"departureVelocityY\" REAL,"
        << "\"departureVelocityZ\" REAL,"
        << "\"departureSemiMajorAxis\" REAL,"
        << "\"departureEccentricity\" REAL,"
        << "\"departureInclination\" REAL,"
        << "\"departureArgumentOfPeriapsis\" REAL,"
        << "\"departureLongitudeOfAscendingNode\" REAL,"
        << "\"departureTrueAnomaly\" REAL,"
        << "\"departureDeltaVX\" REAL,"
        << "\"departureDeltaVY\" REAL,"
        << "\"departureDeltaVZ\" REAL,"
        << "\"arrivalObjectId\" INTEGER,"
        << "\"arrivalPositionX\" REAL,"
        << "\"arrivalPositionY\" REAL,"
        << "\"arrivalPositionZ\" REAL,"
        << "\"arrivalVelocityX\" REAL,"
        << "\"arrivalVelocityY\" REAL,"
        << "\"arrivalVelocityZ\" REAL,"
        << "\"arrivalSemiMajorAxis\" REAL,"
        << "\"arrivalEccentricity\" REAL,"
        << "\"arrivalInclination\" REAL,"
        << "\"arrivalArgumentOfPeriapsis\" REAL,"
        << "\"arrivalLongitudeOfAscendingNode\" REAL,"
        << "\"arrivalTrueAnomaly\" REAL,"
        << "\"arrivalDeltaVX\" REAL,"
        << "\"arrivalDeltaVY\" REAL,"
        << "\"arrivalDeltaVZ\" REAL,"
        << "\"timeOfFlight\" REAL,"
        << "\"revolutions\" REAL,"
        << "\"transferSemiMajorAxis\" REAL,"
        << "\"transferEccentricity\" REAL,"
        << "\"transferInclination\" REAL,"
        << "\"transferArgumentOfPeriapsis\" REAL,"
        << "\"transferLongitudeOfAscendingNode\" REAL,"
        << "\"transferTrueAnomaly\" REAL,"
        << "\"transferDeltaV\" REAL"
        << ");";

    // Execute command to create table.
    database.exec( lambertScannerTableCreate.str( ).c_str( ) );

    // Execute commant to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE UNIQUE INDEX IF NOT EXISTS \"transferDeltaV\" on "
                              << "lambert_scanner (transferDeltaV ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "lambert_scanner" ) )
    {
        std::cerr << "ERROR: Creating table 'lambert_scanner' failed!" << std::endl;
        throw;
    }
}

//! Write transfer shortlist to file.
void writeTransferShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath )
{
    // Fetch transfers to include in shortlist.
    // Database table is sorted by transferDeltaV, in ascending order.
    std::ostringstream shortlistSelect;
    shortlistSelect << "SELECT * FROM lambert_scanner ORDER BY transferDeltaV ASC LIMIT "
                    << shortlistNumber;
    SQLite::Statement query( database, shortlistSelect.str( ) );

    // Write fetch data to file.
    std::ofstream shortlistFile( shortlistPath.c_str( ) );

    // Print file header.
    shortlistFile << "transferId,"
                  << "departureObjectId,"
                  << "departureEpoch,"
                  << "departurePositionX,"
                  << "departurePositionY,"
                  << "departurePositionZ,"
                  << "departureVelocityX,"
                  << "departureVelocityY,"
                  << "departureVelocityZ,"
                  << "departureSemiMajorAxis,"
                  << "departureEccentricity,"
                  << "departureInclination,"
                  << "departureArgumentOfPeriapsis,"
                  << "departureLongitudeOfAscendingNode,"
                  << "departureTrueAnomaly,"
                  << "departureDeltaVX,"
                  << "departureDeltaVY,"
                  << "departureDeltaVZ,"
                  << "arrivalObjectId,"
                  << "arrivalPositionX,"
                  << "arrivalPositionY,"
                  << "arrivalPositionZ,"
                  << "arrivalVelocityX,"
                  << "arrivalVelocityY,"
                  << "arrivalVelocityZ,"
                  << "arrivalSemiMajorAxis,"
                  << "arrivalEccentricity,"
                  << "arrivalInclination,"
                  << "arrivalArgumentOfPeriapsis,"
                  << "arrivalLongitudeOfAscendingNode,"
                  << "arrivalTrueAnomaly,"
                  << "arrivalDeltaVX,"
                  << "arrivalDeltaVY,"
                  << "arrivalDeltaVZ,"
                  << "timeOfFlight,"
                  << "revolutions,"
                  << "transferSemiMajorAxis,"
                  << "transferEccentricity,"
                  << "transferInclination,"
                  << "transferArgumentOfPeriapsis,"
                  << "transferLongitudeOfAscendingNode,"
                  << "transferTrueAnomaly,"
                  << "transferDeltaV"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int    transferId                         = query.getColumn( 0 );
        const int    departureObjectId                  = query.getColumn( 1 );
        const double departureEpoch                     = query.getColumn( 2 );
        const double departurePositionX                 = query.getColumn( 3 );
        const double departurePositionY                 = query.getColumn( 4 );
        const double departurePositionZ                 = query.getColumn( 5 );
        const double departureVelocityX                 = query.getColumn( 6 );
        const double departureVelocityY                 = query.getColumn( 7 );
        const double departureVelocityZ                 = query.getColumn( 8 );
        const double departureSemiMajorAxis             = query.getColumn( 9 );
        const double departureEccentricity              = query.getColumn( 10 );
        const double departureInclination               = query.getColumn( 11 );
        const double departureArgumentOfPeriapsis       = query.getColumn( 12 );
        const double departureLongitudeOfAscendingNode  = query.getColumn( 13 );
        const double departureTrueAnomaly               = query.getColumn( 14 );
        const double departureDeltaVX                   = query.getColumn( 15 );
        const double departureDeltaVY                   = query.getColumn( 16 );
        const double departureDeltaVZ                   = query.getColumn( 17 );
        const int    arrivalObjectId                    = query.getColumn( 18 );
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
        const double arrivalDeltaVX                     = query.getColumn( 31 );
        const double arrivalDeltaVY                     = query.getColumn( 32 );
        const double arrivalDeltaVZ                     = query.getColumn( 33 );
        const double timeOfFlight                       = query.getColumn( 34 );
        const int    revolutions                        = query.getColumn( 35 );
        const double transferSemiMajorAxis              = query.getColumn( 36 );
        const double transferEccentricity               = query.getColumn( 37 );
        const double transferInclination                = query.getColumn( 38 );
        const double transferArgumentOfPeriapsis        = query.getColumn( 39 );
        const double transferLongitudeOfAscendingNode   = query.getColumn( 40 );
        const double transferTrueAnomaly                = query.getColumn( 41 );
        const double transferDeltaV                     = query.getColumn( 42 );

        shortlistFile << transferId << ","
                      << departureObjectId << ","
                      << departureEpoch << ","
                      << departurePositionX << ","
                      << departurePositionY << ","
                      << departurePositionZ << ","
                      << departureVelocityX << ","
                      << departureVelocityY << ","
                      << departureVelocityZ << ","
                      << departureSemiMajorAxis << ","
                      << departureEccentricity << ","
                      << departureInclination << ","
                      << departureArgumentOfPeriapsis << ","
                      << departureLongitudeOfAscendingNode << ","
                      << departureTrueAnomaly << ","
                      << departureDeltaVX << ","
                      << departureDeltaVY << ","
                      << departureDeltaVZ << ","
                      << arrivalObjectId << ","
                      << arrivalPositionX << ","
                      << arrivalPositionY << ","
                      << arrivalPositionZ << ","
                      << arrivalVelocityX << ","
                      << arrivalVelocityY << ","
                      << arrivalVelocityZ << ","
                      << arrivalSemiMajorAxis << ","
                      << arrivalEccentricity << ","
                      << arrivalInclination << ","
                      << arrivalArgumentOfPeriapsis << ","
                      << arrivalLongitudeOfAscendingNode << ","
                      << arrivalTrueAnomaly << ","
                      << arrivalDeltaVX << ","
                      << arrivalDeltaVY << ","
                      << arrivalDeltaVZ << ","
                      << timeOfFlight << ","
                      << revolutions << ","
                      << transferSemiMajorAxis << ","
                      << transferEccentricity << ","
                      << transferInclination << ","
                      << transferArgumentOfPeriapsis << ","
                      << transferLongitudeOfAscendingNode << ","
                      << transferTrueAnomaly << ","
                      << transferDeltaV
                      << std::endl;
    }

    shortlistFile.close( );
}

} // namespace d2d
