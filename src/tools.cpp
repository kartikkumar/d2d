/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/TimeSpan.h>

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

//! Sample SGP4 orbit
StateHistory sampleSGP4Orbit( const Tle& tle,
                              const double propagationTime,
                              const int numberOfSamples,
                              const double initialEpochJulian )
{
    SGP4 sgp4( tle );
    Vector6 state;
    DateTime initialEpoch( ( initialEpochJulian - astro::ASTRO_GREGORIAN_EPOCH_IN_JULIAN_DAYS ) * TicksPerDay );
    Eci initialStateECI = sgp4.FindPosition( initialEpoch );
    Vector6 initialState = getStateVector( initialStateECI );
    state = initialState;

    // compute size of propagation time step
    const double timeStep = propagationTime / static_cast< double >( numberOfSamples );

    StateHistory stateHistory;
    stateHistory[ initialEpochJulian ] = initialState;

    // Loop over all samples and store the propagated state
    DateTime epoch = initialEpoch;
    for ( int i = 0; i < numberOfSamples; i++ )
    {
        epoch = epoch.AddSeconds( timeStep );
        Eci stateECI = sgp4.FindPosition( epoch );
        state = getStateVector( stateECI );
        const double epochJulian = ( ( i + 1 ) * timeStep ) / ( 24.0 * 3600.0 ) + initialEpochJulian;
        stateHistory[ epochJulian ] = state;
    }
    return stateHistory;
}

//! Execute convergence test for a virtual TLE.
bool executeVirtualTleConvergenceTest( const Vector6& propagatedCartesianState,
                                       const Vector6& trueCartesianState,
                                       const double relativeTolerance,
                                       const double absoluteTolerance )
{
    // Check for NAN values.
    for ( int i = 0; i < 6; i++ )
    {
        bool elementIsNan = std::isnan( propagatedCartesianState[ i ] );
        if ( elementIsNan == true )
        {
            // If a NaN value is detected, the convergence test has failed.
            return false;
        }
    }

    // Check if error between target and propagated Cartesian states is within specified
    // tolerances.
    Vector6 absoluteDifference = propagatedCartesianState;
    Vector6 relativeDifference = propagatedCartesianState;
    for ( int i = 0; i < 6; i++ )
    {
        absoluteDifference[ i ]
            = std::fabs( propagatedCartesianState[ i ] - trueCartesianState[ i ] );
        relativeDifference[ i ] = absoluteDifference[ i ] / std::fabs( trueCartesianState[ i ] );
        if ( relativeDifference[ i ] > relativeTolerance )
        {
            if ( absoluteDifference[ i ] > absoluteTolerance )
            {
                // If the relative and absolute difference for the ith element exceeds the
                // specified tolerances, the convergence test has failed.
                return false;
            }
        }
    }

    // Reaching this point means that there are no NaN values and either the relative or absolute
    // difference for each element is within specified tolerance.
    return true;
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

//! Recurse leg-by-leg to generate list of TLE sequences.
void recurseSequences( const int            currentSequencePosition,
                       const TleObjects&    tleObjects,
                       Sequence&            sequence,
                       ListOfSequences&     listOfSequences )
{
    // If current leg has reached the length of the sequence, then the sequence is complete.
    if ( currentSequencePosition == static_cast< int >( sequence.size( ) ) )
    {
        return;
    }

    // Loop through pool of TLE objects and store IDs in sequence.
    for ( unsigned int i = 0; i < tleObjects.size( ); i++ )
    {
        // Store ith TLE object in sequence at position of current leg.
        sequence[ currentSequencePosition ] = tleObjects[ i ];

        // Create a local copy of the list of TLE objects and erase the object selected for the
        // current leg.
        TleObjects tleObjectsLocal = tleObjects;
        tleObjectsLocal.erase( tleObjectsLocal.begin( ) + i );

        // Proceed to the next leg in the sequence through recursion.
        recurseSequences( currentSequencePosition + 1, tleObjectsLocal, sequence, listOfSequences );

        // Write the sequence to the list.
        if ( currentSequencePosition == static_cast< int >( sequence.size( ) ) - 1 )
        {
            listOfSequences.push_back( sequence );
        }
    }
}

//! Compute departure-arrival epoch pairs for all pork-chop plots.
AllEpochs computeAllPorkChopPlotEpochs( const int       sequenceLength,
                                        const double    stayTime,
                                        const DateTime& departureEpochInitial,
                                        const int       departureEpochSteps,
                                        const double    departureEpochStepSize,
                                        const double    timeOfFlightMinimum,
                                        const int       timeOfFlightSteps,
                                        const double    timeOfFlightStepSize )
{
    // Create container of unique departure epochs that are used per leg to compute the pork-chop
    // data set.
    std::vector< DateTime > uniqueDepartureEpochs;
    uniqueDepartureEpochs.push_back( departureEpochInitial );
    for ( int i = 1; i < departureEpochSteps + 1; ++i )
    {
        DateTime departureEpoch = departureEpochInitial;
        departureEpoch = departureEpoch.AddSeconds( departureEpochStepSize * i );
        uniqueDepartureEpochs.push_back( departureEpoch );
    }

    AllEpochs allEpochs;

    // Loop over each leg and generate the departure-arrival epoch pairs.
    for ( int i = 0; i < sequenceLength - 1; ++i )
    {
        ListOfEpochs listOfEpochs;

        // Loop over unique departure epochs.
        for ( unsigned int j = 0; j < uniqueDepartureEpochs.size( ); ++j )
        {
            // Loop over time-of-flight grid.
            for ( int k = 0; k < timeOfFlightSteps + 1; k++ )
            {
                const double timeOfFlight = timeOfFlightMinimum + k * timeOfFlightStepSize;
                const DateTime departureEpoch = uniqueDepartureEpochs[ j ];
                const DateTime arrivalEpoch = uniqueDepartureEpochs[ j ].AddSeconds( timeOfFlight );

                // Store pair of departure and arrival epochs in the list.
                listOfEpochs.push_back( std::make_pair< DateTime, DateTime >( departureEpoch,
                                                                              arrivalEpoch ) );
            }
        }

        // Store list of all departure and arrival epoch combinations for the current leg in the
        // map.
        allEpochs[ i + 1 ] = listOfEpochs;

        // Extract all arrival epochs from list of epochs.
        std::vector< DateTime > listOfArrivalEpochs;
        for ( unsigned int j = 0; j < listOfEpochs.size( ); ++j )
        {
            listOfArrivalEpochs.push_back( listOfEpochs[ j ].second );
        }

        // Sort arrival epochs and only keep unique entries. The unique arrival epochs are saved as
        // the departure epochs for the next leg after adding a fixed stay time.
        std::sort( listOfArrivalEpochs.begin( ), listOfArrivalEpochs.end( ) );
        std::vector< DateTime >::iterator arrivalEpochIterator
            = std::unique( listOfArrivalEpochs.begin( ), listOfArrivalEpochs.end( ) );
        listOfArrivalEpochs.resize( std::distance( listOfArrivalEpochs.begin( ),
                                                   arrivalEpochIterator ) );
        for ( unsigned int j = 0; j < listOfArrivalEpochs.size( ); ++j )
        {
            listOfArrivalEpochs[ j ] = listOfArrivalEpochs[ j ].AddSeconds( stayTime );
        }
        uniqueDepartureEpochs = listOfArrivalEpochs;
    }

    return allEpochs;
}

//! Overload ==-operator to compare PorkChopPlotId objects.
bool operator==( const PorkChopPlotId& id1, const PorkChopPlotId& id2 )
{
    bool isEqual = false;
    if ( id1.legId == id2.legId )
    {
        if ( id1.departureObjectId == id2.departureObjectId )
        {
            if ( id1.arrivalObjectId == id2.arrivalObjectId )
            {
                isEqual = true;
            }
        }
    }

    return isEqual;
}

//! Overload !=-operator to compare PorkChopPlotId objects.
bool operator!=( const PorkChopPlotId& id1, const PorkChopPlotId& id2 )
{
    return !(id1 == id2);
}

//! Overload <-operator to compare PorkChopPlotId objects.
bool operator<( const PorkChopPlotId& id1, const PorkChopPlotId& id2 )
{
    bool isLessThan = false;
    if ( id1.legId == id2.legId )
    {
        if ( id1.departureObjectId == id2.departureObjectId )
        {
            if ( id1.arrivalObjectId < id2.arrivalObjectId )
            {
                isLessThan = true;
            }
        }
        else if ( id1.departureObjectId < id2.departureObjectId )
        {
            isLessThan = true;
        }
    }
    else if ( id1.legId < id2.legId )
    {
        isLessThan = true;
    }

    return isLessThan;
}

//! Overload >=-operator to compare PorkChopPlotId objects.
bool operator<=( const PorkChopPlotId& id1, const PorkChopPlotId& id2 )
{
    bool isLessThanOrEqual = false;
    if ( id1 < id2 )
    {
        isLessThanOrEqual = true;
    }
    else if ( id1 == id2 )
    {
        isLessThanOrEqual = true;
    }
    return isLessThanOrEqual;
}

//! Overload >-operator to compare PorkChopPlotId objects.
bool operator>( const PorkChopPlotId& id1, const PorkChopPlotId& id2 )
{
    return !( id1 <= id2 );
}

//! Overload >=-operator to compare PorkChopPlotId objects.
bool operator>=( const PorkChopPlotId& id1, const PorkChopPlotId& id2 )
{
    bool isGreaterThanOrEqual = false;
    if ( id1 > id2 )
    {
        isGreaterThanOrEqual = true;
    }
    else if ( id1 == id2 )
    {
        isGreaterThanOrEqual = true;
    }
    return isGreaterThanOrEqual;
}

} // namespace d2d
