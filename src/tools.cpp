/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <keplerian_toolbox.h>

#include "D2D/tools.hpp"

namespace d2d
{

//! Sample Kepler orbit.
StateHistory sampleKeplerOrbit( const Vector6& initialState,
                                const double propagationTime,
                                const int numberOfSamples,
                                const double gravitationalParameter,
                                const double initialEpoch )
{
    // Initialize state vectors.
    Vector6 state = initialState;
    Vector3 position;
    std::copy( state.begin( ), state.begin( ) + 3, position.begin( ) );
    Vector3 velocity;
    std::copy( state.begin( ) + 3, state.begin( ) + 6, velocity.begin( ) );

    // Compute size of propagation time steps.
    const double timeStep = propagationTime / static_cast< double >( numberOfSamples );

    StateHistory stateHistory;
    stateHistory[ initialEpoch ] = initialState;

    // Loop over all samples and store propagated state.
    for ( int i = 0; i < numberOfSamples; i++ )
    {
        kep_toolbox::propagate_lagrangian( position, velocity, timeStep, gravitationalParameter );
        std::copy( position.begin( ), position.begin( ) + 3, state.begin( ) );
        std::copy( velocity.begin( ), velocity.begin( ) + 3, state.begin( ) + 3 );
        const double epoch = ( ( i + 1 ) * timeStep ) / ( 24 * 3600.0 ) + initialEpoch;
        stateHistory[ epoch ] = state;
    }

    return stateHistory;
}

//! Convert SGP4 ECI object to state vector.
Vector6 getStateVector( const Eci state )
{
    Vector6 result;
    result[ astro::xPositionIndex ] = state.Position( ).x;
    result[ astro::yPositionIndex ] = state.Position( ).y;
    result[ astro::zPositionIndex ] = state.Position( ).z;
    result[ astro::xVelocityIndex ] = state.Velocity( ).x;
    result[ astro::yVelocityIndex ] = state.Velocity( ).y;
    result[ astro::zVelocityIndex ] = state.Velocity( ).z;
    return result;
}

//! Print state history to stream.
void print( std::ostream& stream,
            const StateHistory stateHistory,
            const std::string& streamHeader,
            const int precision )
{
        stream << streamHeader << std::endl;

        for ( StateHistory::const_iterator iteratorState = stateHistory.begin( );
              iteratorState != stateHistory.end( );
              iteratorState++ )
        {
            stream << std::setprecision( precision )
                   << iteratorState->first       << ","
                   << iteratorState->second[ 0 ] << ","
                   << iteratorState->second[ 1 ] << ","
                   << iteratorState->second[ 2 ] << ","
                   << iteratorState->second[ 3 ] << ","
                   << iteratorState->second[ 4 ] << ","
                   << iteratorState->second[ 5 ] << std::endl;
        }
}

//! Find parameter.
ConfigIterator find( const rapidjson::Document& config, const std::string& parameterName )
{
    const ConfigIterator iterator = config.FindMember( parameterName.c_str( ) );
    if ( iterator == config.MemberEnd( ) )
    {
        std::ostringstream error;
        error << "ERROR: \"" << parameterName << "\" missing from config file!";
        throw std::runtime_error( error.str( ) );
    }
    return iterator;
}

//! Remove newline characters from string.
void removeNewline( std::string& string )
{
    string.erase( std::remove( string.begin( ), string.end( ), '\r' ), string.end( ) );
    string.erase( std::remove( string.begin( ), string.end( ), '\n' ), string.end( ) );
}

//! Get TLE catalog type.
int getTleCatalogType( const std::string& catalogFirstLine )
{
    int tleLines = 0;
    if ( catalogFirstLine.substr( 0, 1 ) == "1" )
    {
        tleLines = 2;
    }
    else if ( catalogFirstLine.substr( 0, 1 ) == "0" )
    {
        tleLines = 3;
    }
    else
    {
        throw std::runtime_error( "ERROR: Catalog malformed!" );
    }

    return tleLines;
}

} // namespace d2d
