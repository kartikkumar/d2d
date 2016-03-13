/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal (abhishek.agrawal@protonmail.com)
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

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/TimeSpan.h>
#include <libsgp4/Tle.h>

#include <Atom/atom.hpp>

#include <Astro/astro.hpp>

#include "D2D/SGP4Scanner.hpp"
// #include "D2D/SGP4Tools.hpp"
#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

#include <keplerian_toolbox.h>

namespace d2d
{

//! Execute sgp4_scanner.
void executeSGP4Scanner( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const SGP4ScannerInput input = checkSGP4ScannerInput( config );

    // Set gravitational parameter used [km^3 s^-2].
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " km^3 s^-2" << std::endl;

    // Set mean radius used [km].
    const double earthMeanRadius = kXKMPER;
    std::cout << "Earth mean radius             " << earthMeanRadius << " km" << std::endl;

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
    const double totalTleObjects = tleObjects.size( );
    std::cout << totalTleObjects << " TLE objects parsed from catalog!" << std::endl;

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
    
    SQLite::Statement query( database, lambertScannerTableSelect.str( ) );

    SQLite::Statement SGP4Query( database, sgp4ScannerTableInsert.str( ) );

    std::cout << "Propagating Lambert transfers using SGP4 and populating database ... "
              << std::endl;

    // Loop over rows in lambert_scanner_results table and propagate Lambert transfers using SGP4.
    boost::progress_display showProgress( lambertScannertTableSize );

    while ( query.executeStep( ) )
    {
        const int      lambertTransferId    = query.getColumn( 0 );
        const int      departureObjectId    = query.getColumn( 1 );

        const double   departureEpochJulian = query.getColumn( 3 );
        const double   timeOfFlight         = query.getColumn( 4 );

        const double   departurePositionX   = query.getColumn( 7 );
        const double   departurePositionY   = query.getColumn( 8 );
        const double   departurePositionZ   = query.getColumn( 9 );
        const double   departureVelocityX   = query.getColumn( 10 );
        const double   departureVelocityY   = query.getColumn( 11 );
        const double   departureVelocityZ   = query.getColumn( 12 );
        const double   departureDeltaVX     = query.getColumn( 37 );
        const double   departureDeltaVY     = query.getColumn( 38 );
        const double   departureDeltaVZ     = query.getColumn( 39 );

        const double   arrivalPositionX     = query.getColumn( 19 );
        const double   arrivalPositionY     = query.getColumn( 20 );
        const double   arrivalPositionZ     = query.getColumn( 21 );
        const double   arrivalVelocityX     = query.getColumn( 22 );
        const double   arrivalVelocityY     = query.getColumn( 23 );
        const double   arrivalVelocityZ     = query.getColumn( 24 );
        const double   arrivalDeltaVX       = query.getColumn( 40 );
        const double   arrivalDeltaVY       = query.getColumn( 41 );
        const double   arrivalDeltaVZ       = query.getColumn( 42 );

        // Set up DateTime object for departure epoch using Julian date.
        // NB: 1721425.5 corresponds to the Gregorian epoch: 0001 Jan 01 00:00:00.0
        DateTime departureEpoch( ( departureEpochJulian - 1721425.5 ) * TicksPerDay );

        // get the transfer departure tle for the current departure object id
        Tle transferDepartureTle;
        bool breakFlag = false;
        int searchCounter = 0;
        while ( searchCounter < totalTleObjects && breakFlag == false )
        {
            if ( departureObjectId == tleObjects[ searchCounter ].NoradNumber( ) )
            {
                transferDepartureTle = tleObjects[ searchCounter ];
                breakFlag = true;
            }
            ++searchCounter;
        }
        if ( transferDepartureTle.NoradNumber( ) != departureObjectId )
            throw std::runtime_error( "ERROR: TLE not found in catalog! in routine SGP4Scanner.cpp" );

        const SGP4 sgp4( transferDepartureTle );
        DateTime arrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
        const Eci tleTransferArrivalState = sgp4.FindPosition( arrivalEpoch );
        const Vector6 transferArrivalState = getStateVector( tleTransferArrivalState );

        //compute the required results
        double arrival_position_x = transferArrivalState[ astro::xPositionIndex ];
        double arrival_position_y = transferArrivalState[ astro::yPositionIndex ];
        double arrival_position_z = transferArrivalState[ astro::zPositionIndex ];

        double arrival_velocity_x = transferArrivalState[ astro::xVelocityIndex ];
        double arrival_velocity_y = transferArrivalState[ astro::yVelocityIndex ];
        double arrival_velocity_z = transferArrivalState[ astro::zVelocityIndex ];

        double arrival_position_x_error = arrival_position_x - arrivalPositionX;
        double arrival_position_y_error = arrival_position_y - arrivalPositionY;
        double arrival_position_z_error = arrival_position_z - arrivalPositionZ;
        std::vector< double > positionErrorVector( 3 );
        positionErrorVector[ 0 ] = arrival_position_x_error;
        positionErrorVector[ 1 ] = arrival_position_y_error;
        positionErrorVector[ 2 ] = arrival_position_z_error;
        double arrival_position_error = sml::norm< double >( positionErrorVector );

        double arrival_velocity_x_error = arrival_velocity_x - ( arrivalVelocityX - arrivalDeltaVX );
        double arrival_velocity_y_error = arrival_velocity_y - ( arrivalVelocityY - arrivalDeltaVY );
        double arrival_velocity_z_error = arrival_velocity_z - ( arrivalVelocityZ - arrivalDeltaVZ );
        std::vector< double > velocityErrorVector( 3 );
        velocityErrorVector[ 0 ] = arrival_velocity_x_error;
        velocityErrorVector[ 1 ] = arrival_velocity_y_error;
        velocityErrorVector[ 2 ] = arrival_velocity_z_error;
        double arrival_velocity_error = sml::norm< double >( velocityErrorVector );

        // Bind values to SQL insert sgp4Query
        SGP4Query.bind( ":lambert_transfer_id",         lambertTransferId );
        SGP4Query.bind( ":arrival_position_x",          arrival_position_x );
        SGP4Query.bind( ":arrival_position_y",          arrival_position_y );
        SGP4Query.bind( ":arrival_position_z",          arrival_position_z );
        SGP4Query.bind( ":arrival_velocity_x",          arrival_velocity_x );
        SGP4Query.bind( ":arrival_velocity_y",          arrival_velocity_y );
        SGP4Query.bind( ":arrival_velocity_z",          arrival_velocity_z );
        SGP4Query.bind( ":arrival_position_x_error",    arrival_position_x_error );
        SGP4Query.bind( ":arrival_position_y_error",    arrival_position_y_error );
        SGP4Query.bind( ":arrival_position_z_error",    arrival_position_z_error );
        SGP4Query.bind( ":arrival_position_error",      arrival_position_error );
        SGP4Query.bind( ":arrival_velocity_x_error",    arrival_velocity_x_error );
        SGP4Query.bind( ":arrival_velocity_y_error",    arrival_velocity_y_error );
        SGP4Query.bind( ":arrival_velocity_z_error",    arrival_velocity_z_error );
        SGP4Query.bind( ":arrival_velocity_error",      arrival_velocity_error );

        // execute insert SGP4Query
        SGP4Query.executeStep( );

        // Reset SQL insert SGP4Query
        SGP4Query.reset( );

        ++showProgress;
    }

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;
}

//! Check sgp4_scanner input parameters.
SGP4ScannerInput checkSGP4ScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                     " << shortlistPath << std::endl;
    }

    return SGP4ScannerInput( catalogPath,
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
        << "\"transfer_id\"                 INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"         INTEGER,"
        << "\"arrival_position_x\"          REAL,"
        << "\"arrival_position_y\"          REAL,"
        << "\"arrival_position_z\"          REAL,"
        << "\"arrival_velocity_x\"          REAL,"
        << "\"arrival_velocity_y\"          REAL,"
        << "\"arrival_velocity_z\"          REAL,"
        << "\"arrival_position_x_error\"    REAL,"
        << "\"arrival_position_y_error\"    REAL,"
        << "\"arrival_position_z_error\"    REAL,"
        << "\"arrival_position_error\"      REAL,"
        << "\"arrival_velocity_x_error\"    REAL,"
        << "\"arrival_velocity_y_error\"    REAL,"
        << "\"arrival_velocity_z_error\"    REAL,"
        << "\"arrival_velocity_error\"      REAL"
        <<                                  ");";

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
        throw std::runtime_error( "ERROR: Creating table 'sgp4_scanner_results' failed! in routine SGP4Scanner.cpp" );
    }
}

} // namespace d2d
