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
        << "0.0,"
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
    std::ostringstream lambertScannerTransfersTableInsert;
    lambertScannerTransfersTableInsert
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

    SQLite::Statement transfersQuery( database, lambertScannerTransfersTableInsert.str( ) );

    for ( AllLambertPorkChopPlots::iterator it = allPorkChopPlots.begin( );
          it != allPorkChopPlots.end( );
          it++ )
    {
        const PorkChopPlotId porkChopId = it->first;
        const LambertPorkChopPlot porkChopPlot = it->second;

        for ( unsigned int i = 0; i < porkChopPlot.size( ); ++i )
        {
            // Bind values to SQL insert query.
            transfersQuery.bind( ":transfer_id",          porkChopPlot[ i ].transferId );
            transfersQuery.bind( ":departure_object_id",  porkChopId.departureObjectId );
            transfersQuery.bind( ":arrival_object_id",    porkChopId.arrivalObjectId );
            transfersQuery.bind( ":departure_epoch",
                porkChopPlot[ i ].departureEpoch.ToJulian( ) );
            transfersQuery.bind( ":time_of_flight",       porkChopPlot[ i ].timeOfFlight );
            transfersQuery.bind( ":revolutions",          porkChopPlot[ i ].revolutions );
            transfersQuery.bind( ":prograde",             porkChopPlot[ i ].isPrograde );
            transfersQuery.bind( ":departure_position_x",
                porkChopPlot[ i ].departureState[ astro::xPositionIndex ] );
            transfersQuery.bind( ":departure_position_y",
                porkChopPlot[ i ].departureState[ astro::yPositionIndex ] );
            transfersQuery.bind( ":departure_position_z",
                porkChopPlot[ i ].departureState[ astro::zPositionIndex ] );
            transfersQuery.bind( ":departure_velocity_x",
                porkChopPlot[ i ].departureState[ astro::xVelocityIndex ] );
            transfersQuery.bind( ":departure_velocity_y",
                porkChopPlot[ i ].departureState[ astro::yVelocityIndex ] );
            transfersQuery.bind( ":departure_velocity_z",
                porkChopPlot[ i ].departureState[ astro::zVelocityIndex ] );
            transfersQuery.bind( ":departure_semi_major_axis",
                porkChopPlot[ i ].departureStateKepler[ astro::semiMajorAxisIndex ] );
            transfersQuery.bind( ":departure_eccentricity",
                porkChopPlot[ i ].departureStateKepler[ astro::eccentricityIndex ] );
            transfersQuery.bind( ":departure_inclination",
                porkChopPlot[ i ].departureStateKepler[ astro::inclinationIndex ] );
            transfersQuery.bind( ":departure_argument_of_periapsis",
                porkChopPlot[ i ].departureStateKepler[ astro::argumentOfPeriapsisIndex ] );
            transfersQuery.bind( ":departure_longitude_of_ascending_node",
                porkChopPlot[ i ].departureStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
            transfersQuery.bind( ":departure_true_anomaly",
                porkChopPlot[ i ].departureStateKepler[ astro::trueAnomalyIndex ] );
            transfersQuery.bind( ":arrival_position_x",
                porkChopPlot[ i ].arrivalState[ astro::xPositionIndex ] );
            transfersQuery.bind( ":arrival_position_y",
                porkChopPlot[ i ].arrivalState[ astro::yPositionIndex ] );
            transfersQuery.bind( ":arrival_position_z",
                porkChopPlot[ i ].arrivalState[ astro::zPositionIndex ] );
            transfersQuery.bind( ":arrival_velocity_x",
                porkChopPlot[ i ].arrivalState[ astro::xVelocityIndex ] );
            transfersQuery.bind( ":arrival_velocity_y",
                porkChopPlot[ i ].arrivalState[ astro::yVelocityIndex ] );
            transfersQuery.bind( ":arrival_velocity_z",
                porkChopPlot[ i ].arrivalState[ astro::zVelocityIndex ] );
            transfersQuery.bind( ":arrival_semi_major_axis",
                porkChopPlot[ i ].arrivalStateKepler[ astro::semiMajorAxisIndex ] );
            transfersQuery.bind( ":arrival_eccentricity",
                porkChopPlot[ i ].arrivalStateKepler[ astro::eccentricityIndex ] );
            transfersQuery.bind( ":arrival_inclination",
                porkChopPlot[ i ].arrivalStateKepler[ astro::inclinationIndex ] );
            transfersQuery.bind( ":arrival_argument_of_periapsis",
                porkChopPlot[ i ].arrivalStateKepler[ astro::argumentOfPeriapsisIndex ] );
            transfersQuery.bind( ":arrival_longitude_of_ascending_node",
                porkChopPlot[ i ].arrivalStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
            transfersQuery.bind( ":arrival_true_anomaly",
                porkChopPlot[ i ].arrivalStateKepler[ astro::trueAnomalyIndex ] );
            transfersQuery.bind( ":transfer_semi_major_axis",
                porkChopPlot[ i ].transferStateKepler[ astro::semiMajorAxisIndex ] );
            transfersQuery.bind( ":transfer_eccentricity",
                porkChopPlot[ i ].transferStateKepler[ astro::eccentricityIndex ] );
            transfersQuery.bind( ":transfer_inclination",
                porkChopPlot[ i ].transferStateKepler[ astro::inclinationIndex ] );
            transfersQuery.bind( ":transfer_argument_of_periapsis",
                porkChopPlot[ i ].transferStateKepler[ astro::argumentOfPeriapsisIndex ] );
            transfersQuery.bind( ":transfer_longitude_of_ascending_node",
                porkChopPlot[ i ].transferStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
            transfersQuery.bind( ":transfer_true_anomaly",
                porkChopPlot[ i ].transferStateKepler[ astro::trueAnomalyIndex ] );
            transfersQuery.bind( ":departure_delta_v_x", porkChopPlot[ i ].departureDeltaV[ 0 ] );
            transfersQuery.bind( ":departure_delta_v_y", porkChopPlot[ i ].departureDeltaV[ 1 ] );
            transfersQuery.bind( ":departure_delta_v_z", porkChopPlot[ i ].departureDeltaV[ 2 ] );
            transfersQuery.bind( ":arrival_delta_v_x",   porkChopPlot[ i ].arrivalDeltaV[ 0 ] );
            transfersQuery.bind( ":arrival_delta_v_y",   porkChopPlot[ i ].arrivalDeltaV[ 1 ] );
            transfersQuery.bind( ":arrival_delta_v_z",   porkChopPlot[ i ].arrivalDeltaV[ 2 ] );
            transfersQuery.bind( ":transfer_delta_v",    porkChopPlot[ i ].transferDeltaV );
            transfersQuery.bind( ":leg_id",              porkChopId.legId );

            // Execute insert query.
            transfersQuery.executeStep( );

            // Reset SQL insert query.
            transfersQuery.reset( );
        }
    }

    // Commit transfers transaction.
    transfersTransaction.commit( );

    std::cout << "Database successfully populated with pork-chop plot transfers! " << std::endl;

    std::cout << "Computing all multi-leg transfers for each sequence ..." << std::endl;

    AllMultiLegTransfers allMultiLegTransfers;
    ListOfMultiLegTransfers listOfMultiLegTransfers;
    MultiLegTransferData multiLegTransferData;

    // Loop over all sequences and call recursive function to compute multi-leg transfers.
    for ( ListOfSequences::iterator iteratorSequences = listOfSequences.begin( );
          iteratorSequences != listOfSequences.end( );
          iteratorSequences++ )
    {
        recurseMuiltiLegLambertTransfers( 0,
                                          iteratorSequences->second,
                                          allPorkChopPlots,
                                          input.stayTime,
                                          listOfMultiLegTransfers,
                                          multiLegTransferData  );

        allMultiLegTransfers[ iteratorSequences->first ] = listOfMultiLegTransfers;

        listOfMultiLegTransfers.clear( );
        multiLegTransferData.clear( );
    }

    std::cout << "All multi-leg transfers successfully computed!" << std::endl;

    std::cout << "Populating the database with all multi-leg transfers ..." << std::endl;

    // Start SQL transaction to populate database with computed multi-leg transfers.
    SQLite::Transaction multiLegTransfersTransaction( database );

    // Set up insert query to add multi-leg transfer data to table in database.
    std::ostringstream lambertScannerMultiLegTransfersTableInsert;
    lambertScannerMultiLegTransfersTableInsert
        << "INSERT INTO lambert_scanner_multi_leg_transfers VALUES ("
        << "NULL,"
        << ":sequence_id,";
    for ( int i = 0; i < input.sequenceLength - 1; ++i )
    {

        lambertScannerMultiLegTransfersTableInsert
            << ":transfer_id_" << i + 1 << ",";
    }
    lambertScannerMultiLegTransfersTableInsert
        << ":mission_duration,"
        << ":total_transfer_delta_v"
        << ");";

    SQLite::Statement multiLegTransferQuery( database,
                                             lambertScannerMultiLegTransfersTableInsert.str( ) );

    for ( AllMultiLegTransfers::iterator iteratorMultiLegTransfers = allMultiLegTransfers.begin( );
          iteratorMultiLegTransfers != allMultiLegTransfers.end( );
          iteratorMultiLegTransfers++ )
    {
        const int sequenceId = iteratorMultiLegTransfers->first;
        const ListOfMultiLegTransfers listOfMultiLegTransfers = iteratorMultiLegTransfers->second;

        for ( unsigned int i = 0; i < listOfMultiLegTransfers.size( ); ++i )
        {

            // Bind values to SQL insert query.
            multiLegTransferQuery.bind( ":sequence_id",            sequenceId );

            double missionDuration = 0.0;
            double totalTransferDeltaV = 0.0;

            for ( int j = 0; j < input.sequenceLength - 1; ++j )
            {
                std::ostringstream multiLegTransfersQueryTransferNumber;
                multiLegTransfersQueryTransferNumber << ":transfer_id_" << j + 1;
                multiLegTransferQuery.bind(
                    multiLegTransfersQueryTransferNumber.str( ),
                    listOfMultiLegTransfers[ i ].multiLegTransferData[ j ].transferId );

                missionDuration
                    += listOfMultiLegTransfers[ i ].multiLegTransferData[ j ].timeOfFlight;
                totalTransferDeltaV
                    += listOfMultiLegTransfers[ i ].multiLegTransferData[ j ].transferDeltaV;
            }

            multiLegTransferQuery.bind( ":mission_duration",       missionDuration );
            multiLegTransferQuery.bind( ":total_transfer_delta_v", totalTransferDeltaV );

            // Execute insert query.
            multiLegTransferQuery.executeStep( );

            // Reset SQL insert query.
            multiLegTransferQuery.reset( );
        }
    }

    // Commit multi-leg transfers transaction.
    multiLegTransfersTransaction.commit( );

    std::cout << "Database successfully populated with all multi-leg transfers!" << std::endl;

    std::cout << "Update sequences table with best multi-leg transfers ... " << std::endl;

    std::ostringstream bestMultiLegTransfersReplace;
    bestMultiLegTransfersReplace
        << "REPLACE INTO sequences "
        << "SELECT       sequence_id, ";
    for ( int i = 0; i < input.sequenceLength; ++i )
    {
        bestMultiLegTransfersReplace
            << "         target_" << i << ", ";
    }
    for ( int i = 0; i < input.sequenceLength - 1; ++i )
    {
        bestMultiLegTransfersReplace
            << "         transfer_id_" << i + 1 << "_, ";
    }
    bestMultiLegTransfersReplace
            << "        transfer_delta_v_, "
            << "        mission_duration_ "
            << "FROM    (SELECT *  "
            << "         FROM   sequences "
            << "         AS     SEQ "
            << "         JOIN   (SELECT   sequence_id                 AS \"sequence_id_match\", ";
    for ( int i = 0; i < input.sequenceLength - 1; ++i )
    {
        bestMultiLegTransfersReplace
            << "                          transfer_id_" << i + 1 << " AS transfer_id_"
                                                                 << i + 1 << "_, ";
    }
    bestMultiLegTransfersReplace
            << "                          min(total_transfer_delta_v) AS \"transfer_delta_v_\", "
            << "                          mission_duration            AS \"mission_duration_\" "
            << "                 FROM     lambert_scanner_multi_leg_transfers "
            << "                 GROUP BY lambert_scanner_multi_leg_transfers.sequence_id) "
            << "         AS MULTI "
            << "         ON SEQ.sequence_id = MULTI.sequence_id_match);";

    database.exec( bestMultiLegTransfersReplace.str( ).c_str( ) );

    std::cout << "Sequences table successfully updated with best multi-leg transfers!" << std::endl;

    // Write sequences file.
    std::cout << "Writing best multi-leg transfers for each sequence to file ... " << std::endl;
    writeSequencesToFile( database, input.sequencesPath, input.sequenceLength );
    std::cout << "Sequences file created successfully!" << std::endl;
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

    const std::string sequencesPath = find( config, "sequences" )->value.GetString( );
    std::cout << "Sequences file                 " << sequencesPath << std::endl;

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
                                sequencesPath );
}

//! Create lambert_scanner tables.
void createLambertScannerTables( SQLite::Database& database, const int sequenceLength )
{
    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS sequences;" );
    database.exec( "DROP TABLE IF EXISTS lambert_scanner_transfers;" );
    database.exec( "DROP TABLE IF EXISTS lambert_scanner_multi_leg_transfers;" );

    // Set up SQL command to create table to store lambert_sequences.
    std::ostringstream lambertScannerSequencesTableCreate;
    lambertScannerSequencesTableCreate
        << "CREATE TABLE sequences ("
        << "\"sequence_id\"                               INTEGER PRIMARY KEY              ,";
    for ( int i = 0; i < sequenceLength; ++i )
    {
        lambertScannerSequencesTableCreate
            << "\"target_" << i << "\"                    INTEGER                          ,";
    }
    for ( int i = 0; i < sequenceLength - 1; ++i )
    {
        lambertScannerSequencesTableCreate
            << "\"transfer_id_" << i + 1 << "\"           INTEGER                          ,";
    }
    lambertScannerSequencesTableCreate
        << "\"total_transfer_delta_v\"                    REAL                             ,"
        << "\"mission_duration\"                          REAL                            );";


    // // Execute command to create table.
    database.exec( lambertScannerSequencesTableCreate.str( ).c_str( ) );

    if ( !database.tableExists( "sequences" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'sequences' failed!" );
    }

    // Set up SQL command to create table to store lambert_scanner transfers.
    std::ostringstream lambertScannerTransfersTableCreate;
    lambertScannerTransfersTableCreate
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
    database.exec( lambertScannerTransfersTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
                              << "lambert_scanner_transfers (transfer_delta_v ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "lambert_scanner_transfers" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'lambert_scanner_transfers' failed!" );
    }

    // Set up SQL command to create table to store lambert_scanner multi-leg transfers.
    std::ostringstream lambertScannerMultiLegTransfersTableCreate;
    lambertScannerMultiLegTransfersTableCreate
        << "CREATE TABLE lambert_scanner_multi_leg_transfers ("
        << "\"multi_leg_transfer_id\"                   INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"sequence_id\"                             INTEGER,";
    for ( int i = 0; i < sequenceLength - 1; ++i )
    {
        lambertScannerMultiLegTransfersTableCreate
            << "\"transfer_id_" << i + 1 << "\"         INTEGER,";
    }
    lambertScannerMultiLegTransfersTableCreate
        << "\"mission_duration\"                        REAL,"
        << "\"total_transfer_delta_v\"                  REAL"
        <<                                              ");";

    // Execute command to create table.
    database.exec( lambertScannerMultiLegTransfersTableCreate.str( ).c_str( ) );

    // Execute command to create index on total transfer Delta-V column.
    std::ostringstream totalTransferDeltaVIndexCreate;
    totalTransferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"total_transfer_delta_v\" on "
                                   << "lambert_scanner_multi_leg_transfers "
                                   << "(total_transfer_delta_v ASC);";
    database.exec( totalTransferDeltaVIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "lambert_scanner_multi_leg_transfers" ) )
    {
        throw std::runtime_error(
            "ERROR: Creating table 'lambert_scanner_multi_leg_transfers' failed!" );
    }
}

//! Write best multi-leg Lambert transfers for each sequence to file.
void writeSequencesToFile( SQLite::Database&    database,
                           const std::string&   sequencesPath,
                           const int            sequenceLength  )
{
    // Fetch sequences tables from database and sort from lowest to highest Delta-V.
    std::ostringstream sequencesSelect;
    sequencesSelect << "SELECT * FROM sequences ORDER BY total_transfer_delta_v ASC;";
    SQLite::Statement query( database, sequencesSelect.str( ) );

    // Write sequences to file.
    std::ofstream sequencesFile( sequencesPath.c_str( ) );

    // Print file header.
    sequencesFile << "sequence_id,";
    for ( int i = 0; i < sequenceLength; ++i )
    {
        sequencesFile << "target_" << i << ",";
    }
    for ( int i = 0; i < sequenceLength - 1; ++i )
    {
        sequencesFile << "transfer_id_" << i + 1 << ",";
    }
    sequencesFile << "total_transfer_delta_v,"
                  << "mission_duration"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int       sequenceId                     = query.getColumn( 0 );
        std::vector< int > targets( sequenceLength );
        for ( unsigned int i = 0; i < targets.size( ); ++i )
        {
            targets[ i ]                               = query.getColumn( i + 1 );
        }
        std::vector< int > transferIds( sequenceLength - 1 );
        for ( unsigned int i = 0; i < transferIds.size( ); ++i )
        {
            transferIds[ i ]                           = query.getColumn( i + sequenceLength + 1 );
        }
        const double    totalTransferDeltaV            = query.getColumn( 2 * sequenceLength );
        const double    missionDuration                = query.getColumn( 2 * sequenceLength + 1 );

        sequencesFile << sequenceId                    << ",";
        for ( unsigned int i = 0; i < targets.size( ); ++i )
        {
            sequencesFile << targets[ i ]              << ",";
        }
        for ( unsigned int i = 0; i < transferIds.size( ); ++i )
        {
            sequencesFile << transferIds[ i ]          << ",";
        }
        sequencesFile << totalTransferDeltaV           << ","
                      << missionDuration
                      << std::endl;
    }

    sequencesFile.close( );
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
    // Check if the end of the sequence has been reached.
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

//! Recurse through sequence leg-by-leg and compute multi-leg transfers.
void recurseMuiltiLegLambertTransfers(  const int                 currentSequencePosition,
                                        const Sequence&           sequence,
                                        AllLambertPorkChopPlots&  allPorkChopPlots,
                                        const double              stayTime,
                                        ListOfMultiLegTransfers&  listOfMultiLegTransfers,
                                        MultiLegTransferData&     multiLegTransferData,
                                        DateTime                  launchEpoch,
                                        DateTime                  lastArrivalEpoch )
{
    // Check if the end of the sequence has been reached.
    if ( currentSequencePosition + 1 == static_cast< int >( sequence.size( ) ) )
    {
        // Add transfer to list.
        listOfMultiLegTransfers.push_back( MultiLegTransfer( launchEpoch, multiLegTransferData ) );

        return;
    }

    // Extract pork-chop plot based on current position in sequence.
    const PorkChopPlotId porkChopPlotId( currentSequencePosition + 1,
                                         sequence[ currentSequencePosition ].NoradNumber( ),
                                         sequence[ currentSequencePosition + 1 ].NoradNumber( ) );
    LambertPorkChopPlot porkChopPlot = allPorkChopPlots[ porkChopPlotId ];
    LambertPorkChopPlot porkChopPlotMatched;

    // If we are beyond the first leg, the pork-chop plot needs to be filtered to ensure that only
    // the departure epochs that match the last arrival epoch + stay time are retained.
    if ( currentSequencePosition > 0 )
    {
        // Declare list of indices in pork-chop plot list to keep based on matching the arrival
        // epoch from the previous leg with the departure epochs for the current leg.
        std::vector< int > matchIndices;

        // Loop over pork-chop plot list and save indices where the last arrival epoch + stay time
        // matches the departure epochs for the current leg.
        for ( unsigned int i = 0; i < porkChopPlot.size( ); ++i )
        {
            DateTime matchEpoch = lastArrivalEpoch;
            matchEpoch.AddSeconds( stayTime );
            if ( porkChopPlot[ i ].departureEpoch == matchEpoch )
            {
                matchIndices.push_back( i );
            }
        }

        // Loop over the matched indices and copy the pork-chop plot grid points to the matched
        // list.
        for ( unsigned int i = 0; i < matchIndices.size( ); ++i )
        {
            porkChopPlotMatched.push_back( porkChopPlot[ matchIndices[ i ] ] );
        }
    }
    else
    {
        porkChopPlotMatched = porkChopPlot;
    }

    // Loop over all departure-arrival epoch pairs in pork-chop plot.
    for ( unsigned int i = 0; i < porkChopPlotMatched.size( ); ++i )
    {
        // If the current position is the start of the first leg, set the launch epoch to the
        // departure epoch for the multi-leg transfer.
        if ( currentSequencePosition == 0 )
        {
            launchEpoch = porkChopPlotMatched[ i ].departureEpoch;
        }

        // Add the time-of-flight and Delta V of the current transfer to the data container.
        multiLegTransferData.push_back( TransferData( porkChopPlotMatched[ i ].transferId,
                            porkChopPlotMatched[ i ].timeOfFlight,
                            porkChopPlotMatched[ i ].transferDeltaV ) );

        // Update the last arrival epoch to the current arrival epoch.
        lastArrivalEpoch = porkChopPlotMatched[ i ].arrivalEpoch;

        recurseMuiltiLegLambertTransfers( currentSequencePosition + 1,
                                          sequence,
                                          allPorkChopPlots,
                                          stayTime,
                                          listOfMultiLegTransfers,
                                          multiLegTransferData,
                                          launchEpoch,
                                          lastArrivalEpoch );

        // Remove last entry in multi-leg transfer;
        multiLegTransferData.pop_back( );
    }
}

} // namespace d2d
