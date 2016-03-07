/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <catch.hpp>

#include <libsgp4/DateTime.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include "D2D/lambertScanner.hpp"
#include "D2D/tools.hpp"

namespace d2d
{
namespace tests
{

const static std::string catalogPath           = "lambert_scanner_tle_3line_catalog_test.txt";
const static std::string databasePath          = "lambert_scanner_test.db";
const static DateTime    departureEpoch        = DateTime( 2015, 03, 24, 16, 03, 30 );
const static double      timeOfFlightMinimum   = 36000.0;
const static double      timeOfFlightMaximum   = 2.0 * 36000.0;
const static double      timeOfFlightSteps     = 2;
const static double      timeOfFlightStepSize  = ( timeOfFlightMaximum - timeOfFlightMinimum )
                                                 / timeOfFlightSteps;
const static bool        isPrograde            = true;
const static int         revolutionsMaximum    = 2;
const static int         shortlistLength       = 10;
const static std::string shortlistPath         = "lambert_scanner_shortlist_test.csv";

TEST_CASE( "Test execution of lambert_scanner application mode", "[lambert_scanner]" )
{
    // Redirect cout to buffer.
    // http://www.cplusplus.com/reference/ios/ios/rdbuf/
    std::streambuf* coutBuffer;
    std::stringstream outputBuffer;
    coutBuffer = std::cout.rdbuf( );
    std::cout.rdbuf( outputBuffer.rdbuf( ) );

    const std::string filePath = getRootPath( ) + "/test/lambert_scanner_test.json";
    std::ifstream file( filePath.c_str( ) );
    std::ostringstream buffer;
    buffer << file.rdbuf( );
    rapidjson::Document config;
    config.Parse( buffer.str( ).c_str( ) );

    const std::string shortlistExpectedPath
        = getRootPath( ) + "/test/lambert_scanner_shortlist_expected.csv";
    const std::string databaseExpectedPath
        = getRootPath( ) + "/test/lambert_scanner_expected.db";

    // Change paths to absolute.
    rapidjson::Value& databasePathConfig = config[ "database" ];
    const std::string databasePathAbsolute
        = getRootPath( ) + "/test/" + databasePathConfig.GetString( );
    databasePathConfig.SetString(
        databasePathAbsolute.c_str( ), databasePathAbsolute.size( ) );

    rapidjson::Value& shortlistConfig = config[ "shortlist" ];
    const std::string shortlistPathAbsolute
        = getRootPath( ) + "/test/" + shortlistConfig[ 1 ].GetString( );
    shortlistConfig[ 1 ].SetString(
        shortlistPathAbsolute.c_str( ), shortlistPathAbsolute.size( ) );

    SECTION( "Test scanner using 3-line TLE catalog" )
    {
        rapidjson::Value& catalogPathConfig = config[ "catalog" ];
        const std::string catalogPathAbsolute
            = getRootPath( ) + "/test/" + catalogPathConfig.GetString( );
        catalogPathConfig.SetString(
            catalogPathAbsolute.c_str( ), catalogPathAbsolute.size( ) );

        executeLambertScanner( config );

        SQLite::Database database( databasePathConfig.GetString( ), SQLITE_OPEN_READONLY );
        SQLite::Database databaseExpected( databaseExpectedPath.c_str( ), SQLITE_OPEN_READONLY );

        SQLite::Statement query( database, "SELECT * FROM lambert_scanner_results;" );
        SQLite::Statement queryExpected(
            databaseExpected, "SELECT * FROM lambert_scanner_results;" );

        while ( query.executeStep( ) )
        {
            queryExpected.executeStep( );

            REQUIRE( query.getColumnCount( ) == queryExpected.getColumnCount( ) );

            for ( int i = 0; i < 3; i++ )
            {
                REQUIRE( query.getColumn( i ).getInt( )
                    == queryExpected.getColumn( i ).getInt( ) );
            }

            for ( int i = 3; i < 5; i++ )
            {
                REQUIRE( query.getColumn( i ).getDouble( )
                    == approx( queryExpected.getColumn( i ).getDouble( ) ) );
            }

            for ( int i = 5; i < 7; i++ )
            {
                REQUIRE( query.getColumn( i ).getInt( )
                    == queryExpected.getColumn( i ).getInt( ) );
            }

            for ( int i = 7; i < query.getColumnCount( ); i++ )
            {
                REQUIRE( query.getColumn( i ).getDouble( )
                    == approx( queryExpected.getColumn( i ).getDouble( ) ) );
            }
        }

        REQUIRE_THROWS( query.executeStep( ) );
        REQUIRE_FALSE( queryExpected.executeStep( ) );
        REQUIRE_THROWS( queryExpected.executeStep( ) );

        std::ifstream shortlistExpected( shortlistExpectedPath.c_str( ) );
        std::ifstream shortlist( shortlistConfig[ 1 ].GetString( ) );

        std::string expectedLine;
        std::string line;

        while( std::getline( shortlistExpected, expectedLine ) )
        {
            std::getline( shortlist, line );
            REQUIRE( line == expectedLine );
        }

        REQUIRE_FALSE( std::getline( shortlistExpected, expectedLine ) );
        REQUIRE_FALSE( std::getline( shortlist, line ) );

        shortlistExpected.close( );
        shortlist.close( );

        // Remove temporary database and shortlist files.
        std::remove( databasePathConfig.GetString( ) );
        std::remove( shortlistConfig[ 1 ].GetString( ) );
    }

    SECTION( "Test scanner using 2-line TLE catalog" )
    {
    }

    // Reset cout buffer.
    std::cout.rdbuf( coutBuffer );
}

TEST_CASE( "Test LambertScannerInput struct", "[lambert_scanner],[input-output]" )
{
    const LambertScannerInput lambertScannerInput( catalogPath,
                                                   databasePath,
                                                   departureEpoch,
                                                   timeOfFlightMinimum,
                                                   timeOfFlightMaximum,
                                                   timeOfFlightSteps,
                                                   timeOfFlightStepSize,
                                                   isPrograde,
                                                   revolutionsMaximum,
                                                   shortlistLength,
                                                   shortlistPath );

    REQUIRE( lambertScannerInput.catalogPath            == catalogPath );
    REQUIRE( lambertScannerInput.databasePath           == databasePath );
    REQUIRE( lambertScannerInput.departureEpoch         == departureEpoch );
    REQUIRE( lambertScannerInput.timeOfFlightMinimum    == timeOfFlightMinimum );
    REQUIRE( lambertScannerInput.timeOfFlightMaximum    == timeOfFlightMaximum );
    REQUIRE( lambertScannerInput.timeOfFlightSteps      == timeOfFlightSteps );
    REQUIRE( lambertScannerInput.timeOfFlightStepSize   == timeOfFlightStepSize );
    REQUIRE( lambertScannerInput.isPrograde             == isPrograde );
    REQUIRE( lambertScannerInput.revolutionsMaximum     == revolutionsMaximum );
    REQUIRE( lambertScannerInput.shortlistLength        == shortlistLength );
    REQUIRE( lambertScannerInput.shortlistPath          == shortlistPath );
}

TEST_CASE( "Test function to check input to lambert scanner", "[lambert_scanner],[input-output]" )
{
    // Redirect cout to buffer.
    // http://www.cplusplus.com/reference/ios/ios/rdbuf/
    std::streambuf* coutBuffer;
    std::stringstream outputBuffer;
    coutBuffer = std::cout.rdbuf( );
    std::cout.rdbuf( outputBuffer.rdbuf( ) );

    const std::string filePath = getRootPath( ) + "/test/lambert_scanner_test.json";
    std::ifstream file( filePath.c_str( ) );
    std::ostringstream buffer;
    buffer << file.rdbuf( );
    rapidjson::Document config;
    config.Parse( buffer.str( ).c_str( ) );

    SECTION( "Test successful creation of LambertScannerInput object from JSON file" )
    {
        const LambertScannerInput lambertScannerInput = checkLambertScannerInput( config );

        REQUIRE( lambertScannerInput.catalogPath          == catalogPath );
        REQUIRE( lambertScannerInput.databasePath         == databasePath );
        REQUIRE( lambertScannerInput.departureEpoch       == departureEpoch );
        REQUIRE( lambertScannerInput.timeOfFlightMinimum  == timeOfFlightMinimum );
        REQUIRE( lambertScannerInput.timeOfFlightMaximum  == timeOfFlightMaximum );
        REQUIRE( lambertScannerInput.timeOfFlightSteps    == timeOfFlightSteps );
        REQUIRE( lambertScannerInput.timeOfFlightStepSize == timeOfFlightStepSize );
        REQUIRE( lambertScannerInput.isPrograde           == isPrograde );
        REQUIRE( lambertScannerInput.revolutionsMaximum   == revolutionsMaximum );
        REQUIRE( lambertScannerInput.shortlistLength      == shortlistLength );
        REQUIRE( lambertScannerInput.shortlistPath        == shortlistPath );
    }

    SECTION( "Test missing \"catalog\" in JSON file" )
    {
        config.RemoveMember( "catalog" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test missing \"database\" in JSON file" )
    {
        config.RemoveMember( "database" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test missing \"departure_epoch\" in JSON file" )
    {
        config.RemoveMember( "departure_epoch" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test missing \"time_of_flight_grid\" in JSON file" )
    {
        config.RemoveMember( "time_of_flight_grid" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test missing \"is_prograde\" in JSON file" )
    {
        config.RemoveMember( "is_prograde" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test missing \"revolutions_maximum\" in JSON file" )
    {
        config.RemoveMember( "revolutions_maximum" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test missing \"shortlist\" in JSON file" )
    {
        config.RemoveMember( "shortlist" );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test empty \"departure_epoch\" variable in JSON file" )
    {
        config[ "departure_epoch" ].Clear( );
        const LambertScannerInput lambertScannerInput = checkLambertScannerInput( config );
        REQUIRE( lambertScannerInput.departureEpoch == DateTime( ) );
    }

    SECTION( "Test minimum time-of-flight which is greater than maximum" )
    {
        config.FindMember( "time_of_flight_grid" )->value[ 0 ].SetDouble( 1.0e7 );
        REQUIRE_THROWS( checkLambertScannerInput( config ) );
    }

    SECTION( "Test if shortlist is switched off in JSON file" )
    {
        config.FindMember( "shortlist" )->value[ 0 ].SetInt( 0 );
        const LambertScannerInput lambertScannerInput = checkLambertScannerInput( config );
        REQUIRE( lambertScannerInput.shortlistPath == "" );
    }

    // Reset cout buffer.
    std::cout.rdbuf( coutBuffer );
}

TEST_CASE( "Test creation of lambert_scanner_results table in SQLite database",
           "[lambert_scanner],[input-output]" )
{
    // Set database path, table name, and open database in read/write mode.
    const std::string databasePath = getRootPath( ) + "/test/lambert_scanner_test.db";
    const std::string tableName = "lambert_scanner_results";
    SQLite::Database database( databasePath.c_str( ), SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    // Create table and check if it is created correctly.
    createLambertScannerTable( database );
    REQUIRE( database.tableExists( tableName ) );
    // @todo Add test to ensure that the table schema is as required.
    // @todo Add test to ensure that index has been created correctly on 'transfer_delta_v' column.

    // Remove temporary database.
    std::remove( databasePath.c_str( ) );
}

TEST_CASE( "Test writing transfer shortlist to file", "[lambert_scanner],[input-output]" )
{
    // Set database path, table name, shortlist path, expected shortlist path and open database in
    // read mode.
    const std::string databasePath = getRootPath( ) + "/test/lambert_scanner_expected.db";
    const std::string tableName = "lambert_scanner_results";
    const std::string shortlistAbsolutePath = getRootPath( ) + "/test/" + shortlistPath;
    const std::string shortlistExpectedPath
        = getRootPath( ) + "/test/lambert_scanner_shortlist_expected.csv";
    SQLite::Database database( databasePath.c_str( ), SQLITE_OPEN_READONLY );

    writeTransferShortlist( database, shortlistLength, shortlistAbsolutePath );

    std::ifstream shortlistExpected( shortlistExpectedPath.c_str( ) );
    std::ifstream shortlist( shortlistAbsolutePath.c_str( ) );

    std::string expectedLine;
    std::string line;

    while( std::getline( shortlist, line ) )
    {
        std::getline( shortlistExpected, expectedLine );
        REQUIRE( line == expectedLine );
    }

    REQUIRE_FALSE( std::getline( shortlistExpected, expectedLine ) );
    REQUIRE_FALSE( std::getline( shortlist, line ) );

    shortlistExpected.close( );
    shortlist.close( );

    // Remove temporary shortlist file.
    std::remove( shortlistAbsolutePath.c_str( ) );
}

} // namespace tests
} // namespace d2d
