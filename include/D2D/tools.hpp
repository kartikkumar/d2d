/*
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include <boost/array.hpp>

#include <rapidjson/document.h>

namespace d2d
{

//! 3-Vector.
typedef boost::array< double, 3 > Vector3;

//! 6-Vector.
typedef boost::array< double, 6 > Vector6;

//! State history.
typedef std::map< double, Vector6 > StateHistory;

//! JSON config iterator.
typedef rapidjson::Value::ConstMemberIterator ConfigIterator;

//! Sample Kepler orbit.
/*!
 * Samples a Kepler orbit and generates a state-history stored in a STL map (key=epoch). The
 * Kepler orbit is sampled by using propagate_lagrangian() provided with PyKep (Izzo, 2012).
 *
 * @param[in]  initialState           Initial Cartesian state [km; km/s]
 * @param[in]  propagationTime        Total propagation time [s]
 * @param[in]  numberOfSamples        Number of samples, distributed evenly over propagation time
 * @param[in]  gravitationalParameter Gravitational parameter of central body [km^3 s^-2]
 * @param[in]  initialEpoch           Epoch corresponding to initial Cartesian state
 *                                    (default = 0.0) [Julian date]
 * @return                            State-history of sampled Kepler orbit
 */
StateHistory sampleKeplerOrbit( const Vector6& initialState,
                                const double propagationTime,
                                const int numberOfSamples,
                                const double gravitationalParameter,
                                const double initialEpoch = 0.0 );

//! Print datum to stream.
/*!
 * Prints a specified datum to stream provided, given a specified width and a filler character.
 *
 * @tparam     DataType  Type for specified datum
 * @param[out] stream    Output stream
 * @param[in]  datum     Specified datum to print
 * @param[in]  width     Width of datum printed to stream, in terms of number of characters
 *                       (default = 25)
 * @param[in]  filler    Character used to fill fixed-width (default = ' ')
 */
template< typename DataType >
inline void print( std::ostream& stream,
                   const DataType datum,
                   const int width = 25,
                   const char filler = ' ' )
{
    stream << std::left << std::setw( width ) << std::setfill( filler ) << datum;
}

template< typename DataType >
inline void printMetaParameter( std::ostream& stream,
                                const std::string& parameterName,
                                const DataType datum,
                                const std::string& unit,
                                const char delimiter = ',' )
{
    print( stream, parameterName );
    stream << delimiter;
    print( stream, datum );
    stream << delimiter;
    print( stream, unit );
    stream << std::endl;
}

inline ConfigIterator findParameter( const rapidjson::Document& config,
                                     const std::string& parameterName )
{
    const ConfigIterator iterator = config.FindMember( parameterName.c_str( ) );
    if ( iterator == config.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"" << parameterName << "\" missing from config file!" << std::endl;
        throw;
    }
    return iterator;
}

inline void printStateHistory( std::ostream& stream,
                          const StateHistory stateHistory,
                          const std::string& fileHeader,
                          const int precision = std::numeric_limits< double >::digits10 )
{
        stream << fileHeader << std::endl;

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

} // namespace d2d
