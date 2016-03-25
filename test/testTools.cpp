/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <cmath>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <catch.hpp>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>
#include <libsgp4/Vector.h>

#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

namespace d2d
{
namespace tests
{

TEST_CASE( "Test root-path function", "[input-output]" )
{
    // Need to figure out how to test getRootPath().
}

TEST_CASE( "Test sampling of Kepler orbit", "[kepler]" )
{
    Vector3 position;
    position[ 0 ] = 7806.3;
    position[ 1 ] = 8214.5;
    position[ 2 ] = -445.8;

    Vector3 velocity;
    velocity[ 0 ] = -7.9;
    velocity[ 1 ] = 7.7;
    velocity[ 2 ] = 0.4;

    Vector6 initialState;
    std::copy( position.begin( ), position.end( ), initialState.begin( ) );
    std::copy( velocity.begin( ), velocity.end( ), initialState.begin( ) + 3 );

    const double propagationTime = 1000.0;
    const int numberOfSamples = 50;
    const double gravitationalParameter = 398600.4418;
    const double initialEpoch = 2457077.24167; // Julian date when test was written

    const double timeStep = static_cast< double >( propagationTime / numberOfSamples );

    StateHistory expectedStateHistory;
    const std::string dataFilePath = getRootPath( ) + "/test/tools_sampled_kepler_orbit.csv";
    std::ifstream file( dataFilePath.c_str( ) );
    std::string line;
    std::getline( file, line ); // skip header line
    typedef boost::tokenizer< boost::escaped_list_separator< char > > Tokenizer;

    while ( std::getline( file, line ) )
    {
        Tokenizer tokenizer( line );
        const double epoch = boost::lexical_cast< double >( *tokenizer.begin( ) );
        Tokenizer::iterator iteratorToken = tokenizer.begin( );
        std::advance( iteratorToken, 1 );
        Vector6 state;
        int counter = 0;
        for( ; iteratorToken != tokenizer.end( ); ++iteratorToken )
        {
            state[ counter ] = boost::lexical_cast< double >( *iteratorToken );
            ++counter;
        }
        expectedStateHistory[ epoch ] = state;
    }

    file.close( );

    StateHistory stateHistory = sampleKeplerOrbit(
        initialState, propagationTime, numberOfSamples, gravitationalParameter, initialEpoch );

    StateHistory::iterator iteratorExpectedStateHistory = expectedStateHistory.begin( );
    for ( StateHistory::iterator iteratorStateHistory = stateHistory.begin( );
          iteratorStateHistory != stateHistory.end( );
          ++iteratorStateHistory )
    {
        REQUIRE( iteratorStateHistory->first == approx( iteratorExpectedStateHistory->first ) );

        for ( unsigned int i = 0; i < 6; ++i )
        {
            REQUIRE( iteratorStateHistory->second[ i ]
                == approx( iteratorExpectedStateHistory->second[ i ] ) );
        }

        std::advance( iteratorExpectedStateHistory, 1 );
    }
}

TEST_CASE( "Test execution of virtual TLE convergence test", "[Virtual TLE],[Convergence]" )
{
    Vector6 trueState;
    trueState[ 0 ] = 7806.3;
    trueState[ 1 ] = 8214.5;
    trueState[ 2 ] = -445.8;
    trueState[ 3 ] = -7.9;
    trueState[ 4 ] = 7.7;
    trueState[ 5 ] = 0.4;

    const double relativeTolerance = 1.0e-8;
    const double absoluteTolerance = 1.0e-10;

    SECTION( "Test to check for NAN values" )
    {
        Vector6 propagatedState;
        propagatedState[ 0 ] = trueState[ 0 ];
        propagatedState[ 1 ] = nan( "1" );
        propagatedState[ 2 ] = trueState[ 2 ];
        propagatedState[ 3 ] = trueState[ 3 ];
        propagatedState[ 4 ] = trueState[ 4 ];
        propagatedState[ 5 ] = nan( "5" );

        bool checkFlag = executeVirtualTleConvergenceTest( propagatedState,
                                                           trueState,
                                                           relativeTolerance,
                                                           absoluteTolerance );

        REQUIRE( checkFlag == false );
    }

    SECTION( "Test to check if relative error between target and propagated Cartesian states is within specified tolerance" )
    {
        Vector6 propagatedState;
        propagatedState[ 0 ] = trueState[ 0 ] + ( 1.0e-9 * std::fabs( trueState[ 0 ] ) );
        propagatedState[ 1 ] = trueState[ 1 ] + ( 1.0e-9 * std::fabs( trueState[ 1 ] ) );
        propagatedState[ 2 ] = trueState[ 2 ] + ( 1.0e-9 * std::fabs( trueState[ 2 ] ) );
        propagatedState[ 3 ] = trueState[ 3 ] + ( 1.0e-9 * std::fabs( trueState[ 3 ] ) );
        propagatedState[ 4 ] = trueState[ 4 ] + ( 1.0e-9 * std::fabs( trueState[ 4 ] ) );
        propagatedState[ 5 ] = trueState[ 5 ] + ( 1.0e-9 * std::fabs( trueState[ 5 ] ) );

        bool checkFlag = executeVirtualTleConvergenceTest( propagatedState,
                                                           trueState,
                                                           relativeTolerance,
                                                           absoluteTolerance );

        REQUIRE( checkFlag == true );
    }

    SECTION( "Test to check if absolute error between target and propagated Cartesian states is within specified tolerance" )
    {
        Vector6 propagatedState;
        propagatedState[ 0 ] = trueState[ 0 ] + ( 1.0e-9 * std::fabs( trueState[ 0 ] ) );
        propagatedState[ 1 ] = trueState[ 1 ] + ( 1.0e-11 );
        propagatedState[ 2 ] = trueState[ 2 ] + ( 1.0e-11 );
        propagatedState[ 3 ] = trueState[ 3 ] + ( 1.0e-11 );
        propagatedState[ 4 ] = trueState[ 4 ] + ( 1.0e-11 );
        propagatedState[ 5 ] = trueState[ 5 ] + ( 1.0e-11 );

        bool checkFlag = executeVirtualTleConvergenceTest( propagatedState,
                                                           trueState,
                                                           relativeTolerance,
                                                           absoluteTolerance );

        REQUIRE( checkFlag == true );

        propagatedState[ 0 ] = 10.0 * trueState[ 0 ];
        propagatedState[ 1 ] = 10.0 * trueState[ 1 ];
        propagatedState[ 2 ] = 10.0 * trueState[ 2 ];
        propagatedState[ 3 ] = 10.0 * trueState[ 3 ];
        propagatedState[ 4 ] = 10.0 * trueState[ 4 ];
        propagatedState[ 5 ] = 10.0 * trueState[ 5 ];

        checkFlag = executeVirtualTleConvergenceTest( propagatedState,
                                                      trueState,
                                                      relativeTolerance,
                                                      absoluteTolerance );

        REQUIRE( checkFlag == false );
    }
}

TEST_CASE( "Test conversion of Eci object in SGP4 library to Vector6", "[conversion]" )
{
    const Vector position( 1.2, 3.5, -6.2 );
    const Vector velocity( -5.9, 1.9, -6.8 );

    Vector6 expectedState;
    expectedState[ 0 ] = position.x;
    expectedState[ 1 ] = position.y;
    expectedState[ 2 ] = position.z;
    expectedState[ 3 ] = velocity.x;
    expectedState[ 4 ] = velocity.y;
    expectedState[ 5 ] = velocity.z;

    Eci tleTestState( DateTime( ), position, velocity );
    const Vector6 state = getStateVector( tleTestState );

    REQUIRE( state == expectedState );
}

TEST_CASE( "Test print functions", "[print],[input-output]" )
{
    SECTION( "Test printing a floating-point number to a formatted stream" )
    {
        const double value = 1.2345;
        const int stringWidth = 10;
        const char stringFiller = ' ';

        const std::string expectedString = "1.2345    ";

        std::ostringstream buffer;
        print( buffer, value, stringWidth, stringFiller );

        REQUIRE( buffer.str( ) == expectedString );
    }

    SECTION( "Test printing a parameter name, value and units to a formatted stream" )
    {
        const std::string parameterName = "Test parameter";
        const double parameterValue = 45.6789;
        const std::string parameterUnits = "[km^3 s^-2]";
        const char delimiter = ',';
        const int width = 15;
        const char filler = ' ';

        const std::string expectedString = "Test parameter ,45.6789        ,[km^3 s^-2]    ";
        std::ostringstream buffer;
        print( buffer, parameterName, parameterValue, parameterUnits, delimiter, width, filler );

        REQUIRE( buffer.str( ) == expectedString );
    }

    SECTION( "Test printing a StateHistory object to a formatted stream" )
    {
        StateHistory stateHistory;

        Vector6 state1;
        state1[ 0 ] = 1.546;
        state1[ 1 ] = -0.894;
        state1[ 2 ] = 3.576;
        state1[ 3 ] = 10.771;
        state1[ 4 ] = -88.344;
        state1[ 5 ] = 73.639;
        stateHistory[ 1.200 ] = state1;

        Vector6 state2;
        state2[ 0 ] = 4.436;
        state2[ 1 ] = 1.599;
        state2[ 2 ] = -84.367;
        state2[ 3 ] = -7.584;
        state2[ 4 ] = -43.665;
        state2[ 5 ] = 12.748;
        stateHistory[ 2.367 ] = state2;

        Vector6 state3;
        state3[ 0 ] = -9.977;
        state3[ 1 ] = -7.336;
        state3[ 2 ] = 35.778;
        state3[ 3 ] = 22.731;
        state3[ 4 ] = -6.664;
        state3[ 5 ] = 9.610;
        stateHistory[ 4.592 ] = state3;

        const std::string header = "T,x,y,z,xdot,ydot,zdot";

        std::ostringstream expectedBuffer;
        expectedBuffer << "T,x,y,z,xdot,ydot,zdot" << std::endl;
        expectedBuffer << "1.2,1.546,-0.894,3.576,10.771,-88.344,73.639" << std::endl;
        expectedBuffer << "2.367,4.436,1.599,-84.367,-7.584,-43.665,12.748" << std::endl;
        expectedBuffer << "4.592,-9.977,-7.336,35.778,22.731,-6.664,9.61" << std::endl;

        std::ostringstream buffer;
        print( buffer, stateHistory, header, 10 );
        REQUIRE( buffer.str( ) == expectedBuffer.str( ) );
    }
}

TEST_CASE( "Test function to find parameter in JSON input file", "[JSON],[input-output]" )
{
    SECTION( "Test search for existing parameter" )
    {
        const char json[] = "{\"hello\" : \"world\"}";
        const std::string expectedParameterValue = "world";

        rapidjson::Document test;
        test.Parse( json );
        REQUIRE_NOTHROW( find( test, "hello" ) );

        rapidjson::Value::ConstMemberIterator parameter = find( test, "hello" );
        REQUIRE( parameter->value.GetString( ) == expectedParameterValue );
    }

    SECTION( "Test search for non-existent parameter" )
    {
        const char json[] = "{\"hello\" : \"world\"}";

        rapidjson::Document test;
        test.Parse( json );
        REQUIRE_THROWS( find( test, "iDontExist" ) );
    }
}

TEST_CASE( "Test parsing functions", "[parser][input-output]" )
{
    SECTION( "Test removal of newline characters from string" )
    {
        std::string testString = "This is a test string with newline characters\r\n";
        const std::string expectedString = "This is a test string with newline characters";

        REQUIRE_FALSE( testString == expectedString );
        removeNewline( testString );
        REQUIRE( testString == expectedString );
    }
}

TEST_CASE( "Test function to determine TLE catalog type", "[TLE][input-output]" )
{
    SECTION( "Test detection of 2-line catalog" )
    {
        const std::string line =
            "1 00005U 58002B   15025.82041458  .00000183  00000-0  24786-3 0  3216";
        REQUIRE( getTleCatalogType( line ) == 2 );
    }

    SECTION( "Test detection of 3-line catalog" )
    {
        const std::string line = "0 VANGUARD 1";
        REQUIRE( getTleCatalogType( line ) == 3 );
    }

    SECTION( "Test detection of malformed catalog" )
    {
        REQUIRE_THROWS( getTleCatalogType( "Malformed TLE line" ) );
    }
}

} // namespace tests
} // namespace d2d
