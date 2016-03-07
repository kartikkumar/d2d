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

#include <rapidjson/document.h>

#include "D2D/catalogPruner.hpp"
#include "D2D/tools.hpp"

namespace d2d
{
namespace tests
{

const static std::string   catalogPath             = "catalog_pruner_tle_3line_catalog_full.txt";
const static double        semiMajorAxisMinimum    = 200.0;
const static double        semiMajorAxisMaximum    = 2000.0;
const static double        eccentricityMinimum     = 0.0;
const static double        eccentricityMaximum     = 0.1;
const static double        inclinationMinimum      = 95.0;
const static double        inclinationMaximum      = 100.0;
const static std::string   nameRegex               = "(ARIANE)";
const static int           catalogCutoff           = 0;
const static std::string   prunedCatalogPath       = "catalog_pruner_tle_3line_pruned_catalog.txt";

TEST_CASE( "Test execution of catalog_pruner application mode", "[catalog-pruner]" )
{
    // Redirect cout to buffer.
    // http://www.cplusplus.com/reference/ios/ios/rdbuf/
    std::streambuf* coutBuffer;
    std::stringstream outputBuffer;
    coutBuffer = std::cout.rdbuf( );
    std::cout.rdbuf( outputBuffer.rdbuf( ) );

    SECTION( "Test pruning using 3-line TLE catalog" )
    {
        const std::string expectedPrunedCatalogPath
            = getRootPath( ) + "/test/catalog_pruner_tle_3line_pruned_catalog_expected.txt";

        const std::string filePath = getRootPath( ) + "/test/catalog_pruner_3line_test.json";
        std::ifstream file( filePath.c_str( ) );
        std::ostringstream buffer;
        buffer << file.rdbuf( );
        rapidjson::Document config;
        config.Parse( buffer.str( ).c_str( ) );

        // Change paths to catalog files to absolute.
        rapidjson::Value& catalogPathConfig = config[ "catalog" ];
        const std::string catalogPathAbsolute
            = getRootPath( ) + "/test/" + catalogPathConfig.GetString( );
        catalogPathConfig.SetString( catalogPathAbsolute.c_str( ), catalogPathAbsolute.size( ) );

        rapidjson::Value& prunedCatalogPathConfig = config[ "catalog_pruned" ];
        const std::string prunedCatalogPathAbsolute
            = getRootPath( ) + "/test/" + prunedCatalogPathConfig.GetString( );
        prunedCatalogPathConfig.SetString(
            prunedCatalogPathAbsolute.c_str( ), prunedCatalogPathAbsolute.size( ) );

        executeCatalogPruner( config );

        std::ifstream prunedCatalogFile( prunedCatalogPathConfig.GetString( ) );
        std::ifstream expectedPrunedCatalogFile( expectedPrunedCatalogPath.c_str( ) );

        std::string prunedLine;
        std::string expectedPrunedLine;

        while( std::getline( prunedCatalogFile, prunedLine ) )
        {
            std::getline( expectedPrunedCatalogFile, expectedPrunedLine );
            REQUIRE( prunedLine.compare( expectedPrunedLine ) == 0 );
        }

        // Extra getline() is required so eofbit is set for expected pruned catalog file too.
        std::getline( expectedPrunedCatalogFile, expectedPrunedLine );

        REQUIRE( prunedCatalogFile.eof( ) );
        REQUIRE( expectedPrunedCatalogFile.eof( ) );

        expectedPrunedCatalogFile.close( );
        prunedCatalogFile.close( );

        // Remove tempoarary pruned catalog file.
        std::remove( prunedCatalogPathConfig.GetString( ) );
    }

    SECTION( "Test pruning using 2-line TLE catalog" )
    {
        const std::string expectedPrunedCatalogPath
            = getRootPath( ) + "/test/catalog_pruner_tle_2line_pruned_catalog_expected.txt";

        const std::string filePath = getRootPath( ) + "/test/catalog_pruner_2line_test.json";
        std::ifstream file( filePath.c_str( ) );
        std::ostringstream buffer;
        buffer << file.rdbuf( );
        rapidjson::Document config;
        config.Parse( buffer.str( ).c_str( ) );

        // Change paths to catalog files to absolute.
        rapidjson::Value& catalogPathConfig = config[ "catalog" ];
        std::string catalogPathAbsolute
            = getRootPath( ) + "/test/" + catalogPathConfig.GetString( );
        catalogPathConfig.SetString( catalogPathAbsolute.c_str( ), catalogPathAbsolute.size( ) );

        rapidjson::Value& prunedCatalogPathConfig = config[ "catalog_pruned" ];
        std::string prunedCatalogPathAbsolute
            = getRootPath( ) + "/test/" + prunedCatalogPathConfig.GetString( );
        prunedCatalogPathConfig.SetString(
            prunedCatalogPathAbsolute.c_str( ), prunedCatalogPathAbsolute.size( ) );

        executeCatalogPruner( config );

        std::ifstream prunedCatalogFile( prunedCatalogPathConfig.GetString( ) );
        std::ifstream expectedPrunedCatalogFile( expectedPrunedCatalogPath.c_str( ) );

        std::string prunedLine;
        std::string expectedPrunedLine;

        while( std::getline( prunedCatalogFile, prunedLine ) )
        {
            std::getline( expectedPrunedCatalogFile, expectedPrunedLine );
            REQUIRE( prunedLine.compare( expectedPrunedLine ) == 0 );
        }

        // Extra getline() is required so eofbit is set for expected pruned catalog file too.
        std::getline( expectedPrunedCatalogFile, expectedPrunedLine );

        REQUIRE( prunedCatalogFile.eof( ) );
        REQUIRE( expectedPrunedCatalogFile.eof( ) );

        expectedPrunedCatalogFile.close( );
        prunedCatalogFile.close( );

        // Remove temporary pruned catalog file.
        std::remove( prunedCatalogPathConfig.GetString( ) );
    }

    // Reset cout buffer.
    std::cout.rdbuf( coutBuffer );
}

TEST_CASE( "Test CatalogPrunerInput struct", "[catalog-pruner],[input-output]" )
{
    const CatalogPrunerInput catalogPrunerInput( catalogPath,
                                                 semiMajorAxisMinimum,
                                                 semiMajorAxisMaximum,
                                                 eccentricityMinimum,
                                                 eccentricityMaximum,
                                                 inclinationMinimum,
                                                 inclinationMaximum,
                                                 nameRegex,
                                                 catalogCutoff,
                                                 prunedCatalogPath );

    REQUIRE( catalogPrunerInput.catalogPath == catalogPath );
    REQUIRE( catalogPrunerInput.semiMajorAxisMinimum == semiMajorAxisMinimum );
    REQUIRE( catalogPrunerInput.semiMajorAxisMaximum == semiMajorAxisMaximum );
    REQUIRE( catalogPrunerInput.eccentricityMinimum == eccentricityMinimum );
    REQUIRE( catalogPrunerInput.eccentricityMaximum == eccentricityMaximum );
    REQUIRE( catalogPrunerInput.inclinationMinimum == inclinationMinimum );
    REQUIRE( catalogPrunerInput.inclinationMaximum == inclinationMaximum );
    REQUIRE( catalogPrunerInput.nameRegex == nameRegex );
    REQUIRE( catalogPrunerInput.catalogCutoff == catalogCutoff );
    REQUIRE( catalogPrunerInput.prunedCatalogPath == prunedCatalogPath );
}

TEST_CASE( "Test function to check input to catalog pruner", "[catalog-pruner],[input-output]" )
{
    // Redirect cout to buffer.
    // http://www.cplusplus.com/reference/ios/ios/rdbuf/
    std::streambuf* coutBuffer;
    std::stringstream outputBuffer;
    coutBuffer = std::cout.rdbuf( );
    std::cout.rdbuf( outputBuffer.rdbuf( ) );

    const std::string filePath = getRootPath( ) + "/test/catalog_pruner_3line_test.json";
    std::ifstream file( filePath.c_str( ) );
    std::ostringstream buffer;
    buffer << file.rdbuf( );
    rapidjson::Document config;
    config.Parse( buffer.str( ).c_str( ) );

    SECTION( "Test successful creation of CatalogPrunerInput object from JSON file" )
    {
        const CatalogPrunerInput catalogPrunerInput = checkCatalogPrunerInput( config );

        REQUIRE( catalogPrunerInput.catalogPath == catalogPath );
        REQUIRE( catalogPrunerInput.semiMajorAxisMinimum == semiMajorAxisMinimum );
        REQUIRE( catalogPrunerInput.semiMajorAxisMaximum == semiMajorAxisMaximum );
        REQUIRE( catalogPrunerInput.eccentricityMinimum == eccentricityMinimum );
        REQUIRE( catalogPrunerInput.eccentricityMaximum == eccentricityMaximum );
        REQUIRE( catalogPrunerInput.inclinationMinimum == inclinationMinimum );
        REQUIRE( catalogPrunerInput.inclinationMaximum == inclinationMaximum );
        REQUIRE( catalogPrunerInput.nameRegex == nameRegex );
        REQUIRE( catalogPrunerInput.catalogCutoff == catalogCutoff );
        REQUIRE( catalogPrunerInput.prunedCatalogPath == prunedCatalogPath );
    }

    SECTION( "Test missing \"catalog\" in JSON file" )
    {
        config.RemoveMember( "catalog" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    SECTION( "Test missing \"semi_major_axis_filter\" in JSON file" )
    {
        config.RemoveMember( "semi_major_axis_filter" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    SECTION( "Test missing \"eccentricity_filter\" in JSON file" )
    {
        config.RemoveMember( "eccentricity_filter" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    SECTION( "Test missing \"inclination_filter\" in JSON file" )
    {
        config.RemoveMember( "inclination_filter" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    SECTION( "Test missing \"name_regex\" in JSON file" )
    {
        config.RemoveMember( "name_regex" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    SECTION( "Test missing \"catalog_cutoff\" in JSON file" )
    {
        config.RemoveMember( "catalog_cutoff" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    SECTION( "Test missing \"catalog_pruned\" in JSON file" )
    {
        config.RemoveMember( "catalog_pruned" );
        REQUIRE_THROWS( checkCatalogPrunerInput( config ) );
    }

    // Reset cout buffer.
    std::cout.rdbuf( coutBuffer );
}

} // namespace tests
} // namespace d2d
