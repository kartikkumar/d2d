/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
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

#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>

#include <SML/sml.hpp>
#include <Astro/astro.hpp>

#include "D2D/lambertScanner.hpp"

namespace d2d
{

//! Execute lambert_scanner.
void executeLambertScanner( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertScannerInput input = checkLambertScannerInput( config );

    // Set gravitational parameter used by Lambert targeter.
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

    TleObjects tleObjects;

    //! List of TLE strings extracted from catalog.
    typedef std::vector< std::string > TleStrings;

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

    // Create tables for Lambert scanner results in SQLite database.
    std::cout << "Creating SQLite database tables if needed ... " << std::endl;
    createLambertScannerTables( database, input.sequenceLength );
    std::cout << "SQLite database set up successfully!" << std::endl;

    std::cout << "Enumerating sequences and populating database ... " << std::endl;

    // Start SQL transaction to populate table of sequences.
    SQLite::Transaction sequencesTransaction( database );

    // Set up insert query to add sequences to table in database.
    std::ostringstream lambertScannerSequencesTableInsert;
    lambertScannerSequencesTableInsert
        << "INSERT INTO sequences VALUES ("
        << ":sequence_id,";
    for ( int i = 0; i < input.sequenceLength - 1; ++i )
    {
        lambertScannerSequencesTableInsert
            << ":target_" << i << ",";
    }
    lambertScannerSequencesTableInsert
        << ":target_" << input.sequenceLength - 1 << ",";
    for ( int i = 0; i < input.sequenceLength - 2; ++i )
    {
        lambertScannerSequencesTableInsert
            << "0,";
    }
    lambertScannerSequencesTableInsert
        << "0,"
        << "0.0"
        << ");";

    SQLite::Statement sequencesQuery( database, lambertScannerSequencesTableInsert.str( ) );

    // Enumerate all sequences for the given pool of TLE objects using a recursive function that
    // generates IDs leg-by-leg.
    ListOfSequences listOfSequences;
    Sequence sequence( input.sequenceLength );
    int currentSequencePosition = 0;
    int sequenceId = 1;

    recurseSequences( currentSequencePosition, tleObjects, sequence, sequenceId, listOfSequences );

    // Write list of sequences to table in database.
    for ( ListOfSequences::iterator iteratorSequences = listOfSequences.begin( );
          iteratorSequences != listOfSequences.end( );
          ++iteratorSequences )
    {
        sequencesQuery.bind( ":sequence_id", iteratorSequences->first );

        for ( unsigned int j = 0; j < sequence.size( ); ++j )
        {
            std::ostringstream sequencesQueryTargetNumber;
            sequencesQueryTargetNumber << ":target_" << j;
            sequencesQuery.bind(
                sequencesQueryTargetNumber.str( ),
                static_cast< int >( iteratorSequences->second[ j ].NoradNumber( ) ) );
        }

        // Execute insert query.
        sequencesQuery.executeStep( );

        // Reset SQL insert query.
        sequencesQuery.reset( );
    }

    // Commit sequences transaction.
    sequencesTransaction.commit( );

    std::cout << "Sequences successfully enumerated!" << std::endl;

    std::cout << "Pre-computing all departure-arrival epochs for each leg ... " << std::endl;

    // Pre-compute all departure and arrival epochs for each leg.
    const AllEpochs allEpochs = computeAllPorkChopPlotEpochs( input.sequenceLength,
                                                              input.stayTime,
                                                              input.departureEpochInitial,
                                                              input.departureEpochSteps,
                                                              input.departureEpochStepSize,
                                                              input.timeOfFlightMinimum,
                                                              input.timeOfFlightSteps,
                                                              input.timeOfFlightStepSize );

    std::cout << "All departure-arrival epochs successfully computed!" << std::endl;

    std::cout << "Computing all pork-chop plot transfers for each leg ... " << std::endl;

    // Compute pork-chop sets for 1-to-1 transfers on a leg-by-leg basis, by stepping through a
    // given sequence using recursion.
    currentSequencePosition = 0;

    // Compute total set of all Lambert pork-chop data.
    AllLambertPorkChopPlots allPorkChopPlots;
    int transferId = 1;
    recurseLambertTransfers( currentSequencePosition,
                             tleObjects,
                             allEpochs,
                             input.isPrograde,
                             input.revolutionsMaximum,
                             sequence,
                             transferId,
                             allPorkChopPlots );

    std::cout << "All pork-chop plot transfers successfully computed! " << std::endl;

    std::cout << "Populating database with pork-chop plot transfers ... " << std::endl;

    // Start SQL transaction to populate database with computed transfers.
    SQLite::Transaction transfersTransaction( database );

    // Set up insert query to add transfer data to table in database.
    std::ostringstream lambertScannerResultsTableInsert;
    lambertScannerResultsTableInsert
        << "INSERT INTO lambert_scanner_transfers VALUES ("
        << ":transfer_id,"
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
        << ":transfer_delta_v,"
        << ":leg_id"
        << ");";

    SQLite::Statement resultsQuery( database, lambertScannerResultsTableInsert.str( ) );

    for ( AllLambertPorkChopPlots::iterator it = allPorkChopPlots.begin( );
          it != allPorkChopPlots.end( );
          it++ )
    {
        PorkChopPlotId porkChopId = it->first;
        LambertPorkChopPlot porkChopPlot = it->second;

        for ( unsigned int i = 0; i < porkChopPlot.size( ); ++i )
        {
            // Bind values to SQL insert query.
            resultsQuery.bind( ":transfer_id",          porkChopPlot[ i ].transferId );
            resultsQuery.bind( ":departure_object_id",  porkChopId.departureObjectId );
            resultsQuery.bind( ":arrival_object_id",    porkChopId.arrivalObjectId );
            resultsQuery.bind( ":departure_epoch",
                porkChopPlot[ i ].departureEpoch.ToJulian( ) );
            resultsQuery.bind( ":time_of_flight",       porkChopPlot[ i ].timeOfFlight );
            resultsQuery.bind( ":revolutions",          porkChopPlot[ i ].revolutions );
            resultsQuery.bind( ":prograde",             porkChopPlot[ i ].isPrograde );
            resultsQuery.bind( ":departure_position_x",
                porkChopPlot[ i ].departureState[ astro::xPositionIndex ] );
            resultsQuery.bind( ":departure_position_y",
                porkChopPlot[ i ].departureState[ astro::yPositionIndex ] );
            resultsQuery.bind( ":departure_position_z",
                porkChopPlot[ i ].departureState[ astro::zPositionIndex ] );
            resultsQuery.bind( ":departure_velocity_x",
                porkChopPlot[ i ].departureState[ astro::xVelocityIndex ] );
            resultsQuery.bind( ":departure_velocity_y",
                porkChopPlot[ i ].departureState[ astro::yVelocityIndex ] );
            resultsQuery.bind( ":departure_velocity_z",
                porkChopPlot[ i ].departureState[ astro::zVelocityIndex ] );
            resultsQuery.bind( ":departure_semi_major_axis",
                porkChopPlot[ i ].departureStateKepler[ astro::semiMajorAxisIndex ] );
            resultsQuery.bind( ":departure_eccentricity",
                porkChopPlot[ i ].departureStateKepler[ astro::eccentricityIndex ] );
            resultsQuery.bind( ":departure_inclination",
                porkChopPlot[ i ].departureStateKepler[ astro::inclinationIndex ] );
            resultsQuery.bind( ":departure_argument_of_periapsis",
                porkChopPlot[ i ].departureStateKepler[ astro::argumentOfPeriapsisIndex ] );
            resultsQuery.bind( ":departure_longitude_of_ascending_node",
                porkChopPlot[ i ].departureStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
            resultsQuery.bind( ":departure_true_anomaly",
                porkChopPlot[ i ].departureStateKepler[ astro::trueAnomalyIndex ] );
            resultsQuery.bind( ":arrival_position_x",
                porkChopPlot[ i ].arrivalState[ astro::xPositionIndex ] );
            resultsQuery.bind( ":arrival_position_y",
                porkChopPlot[ i ].arrivalState[ astro::yPositionIndex ] );
            resultsQuery.bind( ":arrival_position_z",
                porkChopPlot[ i ].arrivalState[ astro::zPositionIndex ] );
            resultsQuery.bind( ":arrival_velocity_x",
                porkChopPlot[ i ].arrivalState[ astro::xVelocityIndex ] );
            resultsQuery.bind( ":arrival_velocity_y",
                porkChopPlot[ i ].arrivalState[ astro::yVelocityIndex ] );
            resultsQuery.bind( ":arrival_velocity_z",
                porkChopPlot[ i ].arrivalState[ astro::zVelocityIndex ] );
            resultsQuery.bind( ":arrival_semi_major_axis",
                porkChopPlot[ i ].arrivalStateKepler[ astro::semiMajorAxisIndex ] );
            resultsQuery.bind( ":arrival_eccentricity",
                porkChopPlot[ i ].arrivalStateKepler[ astro::eccentricityIndex ] );
            resultsQuery.bind( ":arrival_inclination",
                porkChopPlot[ i ].arrivalStateKepler[ astro::inclinationIndex ] );
            resultsQuery.bind( ":arrival_argument_of_periapsis",
                porkChopPlot[ i ].arrivalStateKepler[ astro::argumentOfPeriapsisIndex ] );
            resultsQuery.bind( ":arrival_longitude_of_ascending_node",
                porkChopPlot[ i ].arrivalStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
            resultsQuery.bind( ":arrival_true_anomaly",
                porkChopPlot[ i ].arrivalStateKepler[ astro::trueAnomalyIndex ] );
            resultsQuery.bind( ":transfer_semi_major_axis",
                porkChopPlot[ i ].transferStateKepler[ astro::semiMajorAxisIndex ] );
            resultsQuery.bind( ":transfer_eccentricity",
                porkChopPlot[ i ].transferStateKepler[ astro::eccentricityIndex ] );
            resultsQuery.bind( ":transfer_inclination",
                porkChopPlot[ i ].transferStateKepler[ astro::inclinationIndex ] );
            resultsQuery.bind( ":transfer_argument_of_periapsis",
                porkChopPlot[ i ].transferStateKepler[ astro::argumentOfPeriapsisIndex ] );
            resultsQuery.bind( ":transfer_longitude_of_ascending_node",
                porkChopPlot[ i ].transferStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
            resultsQuery.bind( ":transfer_true_anomaly",
                porkChopPlot[ i ].transferStateKepler[ astro::trueAnomalyIndex ] );
            resultsQuery.bind( ":departure_delta_v_x", porkChopPlot[ i ].departureDeltaV[ 0 ] );
            resultsQuery.bind( ":departure_delta_v_y", porkChopPlot[ i ].departureDeltaV[ 1 ] );
            resultsQuery.bind( ":departure_delta_v_z", porkChopPlot[ i ].departureDeltaV[ 2 ] );
            resultsQuery.bind( ":arrival_delta_v_x",   porkChopPlot[ i ].arrivalDeltaV[ 0 ] );
            resultsQuery.bind( ":arrival_delta_v_y",   porkChopPlot[ i ].arrivalDeltaV[ 1 ] );
            resultsQuery.bind( ":arrival_delta_v_z",   porkChopPlot[ i ].arrivalDeltaV[ 2 ] );
            resultsQuery.bind( ":transfer_delta_v",    porkChopPlot[ i ].transferDeltaV );
            resultsQuery.bind( ":leg_id",              porkChopId.legId );

            // Execute insert query.
            resultsQuery.executeStep( );

            // Reset SQL insert query.
            resultsQuery.reset( );
        }
    }

    // Commit transfers transaction.
    transfersTransaction.commit( );

    std::cout << "Database successfully populated with pork-chop plot transfers! " << std::endl;

    std::cout << "Computing the best transfers per sequence ..." << std::endl;

    std::cout << "The best transfers per sequence successfully computed!" << std::endl;

    std::cout << "Populating the database with the best transfers per sequence ..." << std::endl;

    std::cout << "Database successfully populated with the best transfers per sequence!"
              << std::endl;
}

//! Check lambert_scanner input parameters.
LambertScannerInput checkLambertScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath  = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const int sequenceLength       = find( config, "sequence_length" )->value.GetInt( );
    std::cout << "Sequence length               " << sequenceLength << std::endl;

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

    const double departureEpochRange
        = find( config, "departure_epoch_grid" )->value[ 0 ].GetDouble( );
    std::cout << "Departure epoch grid range    " << departureEpochRange << std::endl;
    const double departureGridSteps
        = find( config, "departure_epoch_grid" )->value[ 1 ].GetDouble( );
    std::cout << "Departure epoch grid steps    " << departureGridSteps << std::endl;

    const double timeOfFlightMinimum
        = find( config, "time_of_flight_grid" )->value[ 0 ].GetDouble( );
    std::cout << "Minimum time-of-flight        " << timeOfFlightMinimum << std::endl;
    const double timeOfFlightMaximum
        = find( config, "time_of_flight_grid" )->value[ 1 ].GetDouble( );
    std::cout << "Maximum time-of-flight        " << timeOfFlightMaximum << std::endl;

    if ( timeOfFlightMinimum > timeOfFlightMaximum )
    {
        throw std::runtime_error( "ERROR: Maximum time-of-flight must be larger than minimum!" );
    }

    const double timeOfFlightSteps
        = find( config, "time_of_flight_grid" )->value[ 2 ].GetDouble( );
    std::cout << "# Time-of-flight steps        " << timeOfFlightSteps << std::endl;

    const double stayTime = find( config, "stay_time" )->value.GetDouble( );
    std::cout << "Stay time                     " << stayTime << std::endl;

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
                                databasePath,
                                sequenceLength,
                                departureEpoch,
                                departureGridSteps,
                                departureEpochRange/departureGridSteps,
                                timeOfFlightMinimum,
                                timeOfFlightMaximum,
                                timeOfFlightSteps,
                                ( timeOfFlightMaximum - timeOfFlightMinimum ) / timeOfFlightSteps,
                                stayTime,
                                isPrograde,
                                revolutionsMaximum,
                                shortlistLength,
                                shortlistPath );
}

//! Create lambert_scanner tables.
void createLambertScannerTables( SQLite::Database& database, const int sequenceLength )
{
    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS sequences;" );
    database.exec( "DROP TABLE IF EXISTS lambert_scanner_transfers;" );

    // Set up SQL command to create table to store lambert_sequences.
    std::ostringstream lambertScannerSequencesTableCreate;
    lambertScannerSequencesTableCreate
        << "CREATE TABLE sequences ("
        << "\"sequence_id\"                               INTEGER PRIMARY KEY              ,";
    for ( int i = 0; i < sequenceLength - 1; ++i )
    {
        lambertScannerSequencesTableCreate
            << "\"target_" << i << "\"                    INTEGER                          ,";
    }
    lambertScannerSequencesTableCreate
        << "\"target_" << sequenceLength - 1 << "\"       INTEGER                          ,";

    for ( int i = 0; i < sequenceLength - 2; ++i )
    {
        lambertScannerSequencesTableCreate
            << "\"transfer_id_" << i + 1 << "\"           INTEGER                          ,";
    }
    lambertScannerSequencesTableCreate
        << "\"transfer_id_" << sequenceLength - 1 << "\"  INTEGER                          ,";
    lambertScannerSequencesTableCreate
        << "\"cumulative_delta_v\"                        REAL                            );";

    // // Execute command to create table.
    database.exec( lambertScannerSequencesTableCreate.str( ).c_str( ) );

    if ( !database.tableExists( "sequences" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'sequences' failed!" );
    }

    // Set up SQL command to create table to store lambert_scanner results.
    std::ostringstream lambertScannerResultsTableCreate;
    lambertScannerResultsTableCreate
        << "CREATE TABLE lambert_scanner_transfers ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY,"
        << "\"departure_object_id\"                     INTEGER,"
        << "\"arrival_object_id\"                       INTEGER,"
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
        << "\"transfer_delta_v\"                        REAL,"
        << "\"leg_id\"                                  INTEGER"
        <<                                              ");";

    // Execute command to create table.
    database.exec( lambertScannerResultsTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
                              << "lambert_scanner_transfers (transfer_delta_v ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "lambert_scanner_transfers" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'lambert_scanner_transfers' failed!" );
    }
}

//! Recurse through sequences leg-by-leg and compute pork-chop plots.
void recurseLambertTransfers( const int                currentSequencePosition,
                              const TleObjects&        tleObjects,
                              const AllEpochs&         allEpochs,
                              const bool               isPrograde,
                              const int                revolutionsMaximum,
                              Sequence&                sequence,
                              int&                     transferId,
                              AllLambertPorkChopPlots& allPorkChopPlots )
{
    if ( currentSequencePosition == static_cast< int >( sequence.size( ) ) )
    {
        return;
    }

    for ( unsigned int i = 0; i < tleObjects.size( ); i++ )
    {
        sequence[ currentSequencePosition ] = tleObjects[ i ];

        // Check if the current position in the sequence is beyond the first.
        // (currentSequencePosition = 0 means that the first TLE object ID has to be set).
        if ( currentSequencePosition > 0 )
        {
            const Tle departureObject = sequence[ currentSequencePosition - 1 ];
            const Tle arrivalObject   = sequence[ currentSequencePosition ];
            const int currentLeg      = currentSequencePosition;
            const PorkChopPlotId porkChopPlotId( currentLeg,
                                                 departureObject.NoradNumber( ),
                                                 arrivalObject.NoradNumber( ) );

            // Check if pork-chop ID exists already, i.e., combination of current leg, departure
            // object ID and arrival object ID.
            // If it exists already, skip the generation of the pork-chop plot.
            if ( allPorkChopPlots.find( porkChopPlotId ) == allPorkChopPlots.end( ) )
            {
                allPorkChopPlots[ porkChopPlotId ]
                    = computeLambertPorkChopPlot( departureObject,
                                                  arrivalObject,
                                                  allEpochs.at( currentLeg ),
                                                  isPrograde,
                                                  revolutionsMaximum,
                                                  transferId );
            }
        }

        // Erase the selected TLE object at position currentSequencePosition in sequence.
        TleObjects tleObjectsLocal = tleObjects;
        tleObjectsLocal.erase( tleObjectsLocal.begin( ) + i );

        recurseLambertTransfers( currentSequencePosition + 1,
                                 tleObjectsLocal,
                                 allEpochs,
                                 isPrograde,
                                 revolutionsMaximum,
                                 sequence,
                                 transferId,
                                 allPorkChopPlots );
    }
}

//! Compute Lambert pork-chop plot.
LambertPorkChopPlot computeLambertPorkChopPlot( const Tle&          departureObject,
                                                const Tle&          arrivalObject,
                                                const ListOfEpochs& listOfEpochs,
                                                const bool          isPrograde,
                                                const int           revolutionsMaximum,
                                                      int&          transferId )
{
    LambertPorkChopPlot porkChopPlot;

    const double earthGravitationalParameter = kMU;

    // Loop over all departure and arrival epoch pairs and compute transfers.
    for ( unsigned int i = 0; i < listOfEpochs.size( ); ++i )
    {
        DateTime departureEpoch = listOfEpochs[ i ].first;
        DateTime arrivalEpoch   = listOfEpochs[ i ].second;

        // Compute Cartesian departure position and velocity.
        SGP4 sgp4Departure( departureObject );
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

        // Compute Cartesian departure position and velocity.
        SGP4 sgp4Arrival( arrivalObject );
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

        const double timeOfFlight
            = ( arrivalEpoch.ToJulian( ) - departureEpoch.ToJulian( ) ) * 24.0 * 3600.0;

        kep_toolbox::lambert_problem targeter( departurePosition,
                                               arrivalPosition,
                                               timeOfFlight,
                                               earthGravitationalParameter,
                                               !isPrograde,
                                               revolutionsMaximum );

        const int numberOfSolutions = targeter.get_v1( ).size( );

        // Compute Delta-Vs for transfer and determine index of lowest.
        typedef std::vector< Vector3 > VelocityList;
        VelocityList departureDeltaVs( numberOfSolutions );
        VelocityList arrivalDeltaVs( numberOfSolutions );

        typedef std::vector< double > TransferDeltaVList;
        TransferDeltaVList transferDeltaVs( numberOfSolutions );

        for ( int j = 0; j < numberOfSolutions; ++j )
        {
            // Compute Delta-V for transfer.
            const Vector3 transferDepartureVelocity = targeter.get_v1( )[ j ];
            const Vector3 transferArrivalVelocity = targeter.get_v2( )[ j ];

            departureDeltaVs[ j ] = sml::add( transferDepartureVelocity,
                                              sml::multiply( departureVelocity, -1.0 ) );
            arrivalDeltaVs[ j ]   = sml::add( arrivalVelocity,
                                              sml::multiply( transferArrivalVelocity, -1.0 ) );

            transferDeltaVs[ j ]
                = sml::norm< double >( departureDeltaVs[ j ] )
                    + sml::norm< double >( arrivalDeltaVs[ j ] );
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

        porkChopPlot.push_back(
            LambertPorkChopPlotGridPoint( transferId,
                                          departureEpoch,
                                          arrivalEpoch,
                                          timeOfFlight,
                                          revolutionsMaximum,
                                          isPrograde,
                                          departureState,
                                          departureStateKepler,
                                          arrivalState,
                                          arrivalStateKepler,
                                          transferStateKepler,
                                          departureDeltaVs[ minimumDeltaVIndex ],
                                          arrivalDeltaVs[ minimumDeltaVIndex ],
                                          *minimumDeltaVIterator ) );
        transferId++;

    }
    return porkChopPlot;
}

} // namespace d2d
