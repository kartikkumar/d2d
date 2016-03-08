/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <rapidjson/document.h>

#include "D2D/catalogPruner.hpp"
// #include "D2D/lambertFetch.hpp"
#include "D2D/lambertScanner.hpp"
// #include "D2D/lambertTransfer.hpp"
// #include "D2D/sgp4Propagator.hpp"

int main( const int numberOfInputs, const char* inputArguments[ ] )
{
    ///////////////////////////////////////////////////////////////////////////

    std::cout << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;
    std::cout << "                               D2D                                " << std::endl;
    std::cout << "                              0.0.3                               " << std::endl;
    std::cout << std::endl;
    std::cout << "       Copyright (c) 2014-2015, K. Kumar (me@kartikkumar.com)     " << std::endl;
    std::cout << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                          Input parameters                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Check that only one input has been provided (a JSON file).
    if ( numberOfInputs - 1 != 1 )
    {
        std::cerr << "ERROR: Number of inputs is wrong. Please only provide a JSON input file!"
                  << std::endl;
        throw;
    }

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    // Read and store JSON input document (filter out comment lines).
    // TODO: Need to make comment-line filtering more robust.
    std::ifstream inputFile( inputArguments[ 1 ] );
    std::stringstream jsonDocumentBuffer;
    std::string inputLine;
    while ( std::getline( inputFile, inputLine ) )
    {
        size_t startPosition = inputLine.find_first_not_of( " \t" );
        if ( std::string::npos != startPosition )
        {
            inputLine = inputLine.substr( startPosition );
        }

        if ( inputLine.substr( 0, 2 ) != "//" )
        {
            jsonDocumentBuffer << inputLine << "\n";
        }
    }

    // Check the application mode requested and redirect to the right branch.
    rapidjson::Document config;
    config.Parse( jsonDocumentBuffer.str( ).c_str( ) );

    rapidjson::Value::MemberIterator modeIterator = config.FindMember( "mode" );
    if ( modeIterator == config.MemberEnd( ) )
    {
        std::cerr << "ERROR: Configuration option \"mode\" could not be found in JSON input!"
                  << std::endl;
        throw;
    }
    std::string mode = modeIterator->value.GetString( );
    std::transform( mode.begin( ), mode.end( ), mode.begin( ), ::tolower );

    if ( mode.compare( "catalog_pruner") == 0 )
    {
        std::cout << "Mode                          " << mode << std::endl;
        d2d::executeCatalogPruner( config );
    }
    // else if ( mode.compare( "lambert_transfer" ) == 0 )
    // {
    //     std::cout << "Mode                          " << mode << std::endl;
    //     d2d::executeLambertTransfer( config );
    // }
    else if ( mode.compare( "lambert_scanner" ) == 0 )
    {
        std::cout << "Mode                          " << mode << std::endl;
        d2d::executeLambertScanner( config );
    }
    // // else if ( mode.compare( "lambert_fetch" ) == 0 )
    // // {
    // //     std::cout << "Mode:                         " << mode << std::endl;
    // //     d2d::fetchLambertTransfer( config );
    // // }
    // // else if ( mode.compare( "sgp4_propagator" ) == 0 )
    // // {
    // //     std::cout << "Mode:                         " << mode << std::endl;
    // //     d2d::executeSGP4Propagator( config );
    // // }
    else
    {
        std::cerr << "ERROR: Requested \"mode\" << mode << is invalid!" << std::endl;
        throw;
    }

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    std::cout << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;
    std::cout << "                         Exited successfully!                     " << std::endl;
    std::cout << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    return EXIT_SUCCESS;

    ///////////////////////////////////////////////////////////////////////////
}
