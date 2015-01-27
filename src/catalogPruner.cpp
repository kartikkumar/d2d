/*
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <fstream>
#include <iostream>

#include <boost/xpressive/xpressive.hpp>

#include <libsgp4/Globals.h>
#include <libsgp4/OrbitalElements.h>
#include <libsgp4/Tle.h>

#include "D2D/catalogPruner.hpp"
#include "D2D/tools.hpp"

namespace d2d
{

//! Execute TLE catalog pruner.
void executeCatalogPruner( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const CatalogPrunerInput input = checkCatalogPrunerInput( config );

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                              Parser                              " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Parse catalog and store TLE objects.
    std::ifstream catalogFile( input.catalogPath.c_str( ) );
    std::string catalogLine;

    // Check if catalog is 2-line or 3-line version.
    int tleLines = 3;
    std::getline( catalogFile, catalogLine );
    if ( catalogLine.substr( 0, 1 ) != "0" )
    {
        tleLines = 2;
        std::cout << "2-line catalog detected ... " << std::endl;
        std::cerr << "WARNING: regex name filter will be skipped!" << std::endl;
    }
    else
    {
        std::cout << "3-line catalog detected ..." << std::endl;
    }

    // Reset file stream to start of file.
    catalogFile.seekg( 0, std::ios::beg );

    // Loop over file and apply filters to generate pruned catalog.
    if ( tleLines == 3 )
    {
        std::ofstream prunedCatalogFile( input.prunedCatalogPath.c_str( ) );

        while ( std::getline( catalogFile, catalogLine ) )
        {
            if ( catalogLine.substr( 0, 1 ) != "0" )
            {
                // TODO: print catalog line in error message.
                std::cerr << "ERROR: Catalog malformed!" << std::endl;
                throw;
            }
            const std::string line0 = catalogLine;

            // Check regex name filter.
            // If regex can't be match, continue.
            boost::xpressive::sregex line0RegexFilter
                = boost::xpressive::sregex::compile( input.nameRegex.c_str( ) );
            if ( !boost::xpressive::regex_search( line0, line0RegexFilter ) )
            {
                std::getline( catalogFile, catalogLine );
                std::getline( catalogFile, catalogLine );
                continue;
            }

            std::getline( catalogFile, catalogLine );
            if ( catalogLine.substr( 0, 1 ) != "1" )
            {
                // Print catalog line in error message.
                std::cerr << "ERROR: Catalog malformed!" << std::endl;
                throw;
            }
            removeNewline( catalogLine );
            const std::string line1 = catalogLine;

            std::getline( catalogFile, catalogLine );
            if ( catalogLine.substr( 0, 1 ) != "2" )
            {
                // Print catalog line in error message.
                std::cerr << "ERROR: Catalog malformed!" << std::endl;
                throw;
            }
            removeNewline( catalogLine );
            const std::string line2 = catalogLine;

            // Create TLE object from catalog lines.
            const Tle tle( line0, line1, line2 );

            // Apply filters.
            const OrbitalElements orbitalElements( tle );

            // Apply semi-major axis filter.
            const double semiMajorAxis = orbitalElements.RecoveredSemiMajorAxis( ) * kXKMPER;
            if ( ( semiMajorAxis < input.semiMajorAxisMinimum + kXKMPER )
                 || ( semiMajorAxis > input.semiMajorAxisMaximum + kXKMPER ) )
            {
                continue;
            }

            // Apply eccentricity filter.
            const double eccentricity = orbitalElements.Eccentricity( );
            if ( ( eccentricity < input.eccentricityMinimum )
                 || ( eccentricity > input.eccentricityMaximum ) )
            {
                continue;
            }

            // Apply inclination filter.
            const double inclination = orbitalElements.Inclination( ) / kPI * 180.0;
            if ( ( inclination < input.inclinationMinimum )
                 || ( inclination > input.inclinationMaximum ) )
            {
                continue;
            }

            // This point is reached if TLE is not filtered: write catalog lines to pruned catalog.
            prunedCatalogFile << line0 << std::endl;
            prunedCatalogFile << line1 << std::endl;
            prunedCatalogFile << line2 << std::endl;
        }

        prunedCatalogFile.close( );
    }
    else if ( tleLines == 2 )
    {
        std::ofstream prunedCatalogFile( input.prunedCatalogPath.c_str( ) );

        while ( std::getline( catalogFile, catalogLine ) )
        {
            std::getline( catalogFile, catalogLine );
            if ( catalogLine.substr( 0, 1 ) != "1" )
            {
                // Print catalog line in error message.
                std::cerr << "ERROR: Catalog malformed!" << std::endl;
                throw;
            }
            removeNewline( catalogLine );
            const std::string line1 = catalogLine;

            std::getline( catalogFile, catalogLine );
            if ( catalogLine.substr( 0, 1 ) != "2" )
            {
                // Print catalog line in error message.
                std::cerr << "ERROR: Catalog malformed!" << std::endl;
                throw;
            }
            removeNewline( catalogLine );
            const std::string line2 = catalogLine;

            // Create TLE object from catalog lines.
            const Tle tle( line1, line2 );

            // Apply filters.
            const OrbitalElements orbitalElements( tle );

            // Apply semi-major axis filter.
            const double semiMajorAxis = orbitalElements.RecoveredSemiMajorAxis( ) * kXKMPER;
            if ( ( semiMajorAxis < input.semiMajorAxisMinimum + kXKMPER )
                 || ( semiMajorAxis > input.semiMajorAxisMaximum + kXKMPER ) )
            {
                continue;
            }

            // Apply eccentricity filter.
            const double eccentricity = orbitalElements.Eccentricity( );
            if ( ( eccentricity < input.eccentricityMinimum )
                 || ( eccentricity > input.eccentricityMaximum ) )
            {
                continue;
            }

            // Apply inclination filter.
            const double inclination = orbitalElements.Inclination( ) / kPI * 180.0;
            if ( ( inclination < input.inclinationMinimum )
                 || ( inclination > input.inclinationMaximum ) )
            {
                continue;
            }

            // This point is reached if TLE is not filtered: write catalog lines to pruned catalog.
            prunedCatalogFile << line1 << std::endl;
            prunedCatalogFile << line2 << std::endl;
        }

        prunedCatalogFile.close( );
    }
    else
    {
        std::cerr << "ERROR: # of lines per TLE must be 2 or 3!" << std::endl;
        throw;
    }

    catalogFile.close( );
}

//! Check catalog_pruner input parameters.
CatalogPrunerInput checkCatalogPrunerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const double semiMajorAxisMinimum
        = find( config, "semi_major_axis_filter" )->value[ 0 ].GetDouble( );
    std::cout << "Minimum semi-major axis [km]  " << semiMajorAxisMinimum << std::endl;

    const double semiMajorAxisMaximum
        = find( config, "semi_major_axis_filter" )->value[ 1 ].GetDouble( );
    std::cout << "Maximum semi-major axis [km]  " << semiMajorAxisMaximum << std::endl;

    if ( semiMajorAxisMinimum > semiMajorAxisMaximum )
    {
        std::cerr << "ERROR: Minimum altitude filter is greater the maximum!" << std::endl;
        throw;
    }

    const double eccentricityMinimum
        = find( config, "eccentricity_filter" )->value[ 0 ].GetDouble( );
    std::cout << "Minimum eccentricity [-]      " << eccentricityMinimum << std::endl;

    if ( eccentricityMinimum < 0.0 )
    {
        std::cerr << "ERROR: Minimum eccentricity is less than 0.0!" << std::endl;
        throw;
    }

    const double eccentricityMaximum
        = find( config, "eccentricity_filter" )->value[ 1 ].GetDouble( );
    std::cout << "Maximum eccentricity [-]      " << eccentricityMaximum << std::endl;

    if ( eccentricityMaximum > 1.0 )
    {
        std::cerr << "ERROR: Maximum eccentricity is greater than 1.0!" << std::endl;
        throw;
    }

    if ( eccentricityMinimum > eccentricityMaximum )
    {
        std::cerr << "ERROR: Minimum eccentricity filter is greater the maximum!" << std::endl;
        throw;
    }

    const double inclinationMinimum
        = find( config, "inclination_filter" )->value[ 0 ].GetDouble( );
    std::cout << "Minimum inclination [deg]     " << inclinationMinimum << std::endl;

    const double inclinationMaximum
        = find( config, "inclination_filter" )->value[ 1 ].GetDouble( );
    std::cout << "Maximum inclination [deg]     " << inclinationMaximum << std::endl;

    if ( inclinationMinimum > inclinationMaximum )
    {
        std::cerr << "ERROR: Minimum inclination filter is greater the maximum!" << std::endl;
        throw;
    }

    const std::string nameRegex = find( config, "name_regex" )->value.GetString( );
    std::cout << "Name regex                    " << nameRegex << std::endl;

    const std::string prunedCatalogPath = find( config, "catalog_pruned" )->value.GetString( );
    std::cout << "Pruned catalog                " << prunedCatalogPath << std::endl;

    return CatalogPrunerInput( catalogPath,
                               semiMajorAxisMinimum,
                               semiMajorAxisMaximum,
                               eccentricityMinimum,
                               eccentricityMaximum,
                               inclinationMinimum,
                               inclinationMaximum,
                               nameRegex,
                               prunedCatalogPath );
}

} // namespace d2d
