/*    
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <SML/sml.hpp>
#include <SAM/sam.hpp>

#include "D2D/lambertScanner.hpp"

namespace d2d
{

//! Execute Lambert scanner.
void executeLambertScanner( const rapidjson::Document& jsonInput )
{
	// Verify input parameters.
	const LambertScannerInput input = checkLambertScannerInput( jsonInput );

    // Parse catalog and store TLe objects.
	std::ifstream catalogFile( input.catalogPath.c_str( ) );
	std::string catalogLine;
	typedef std::vector< std::string > TleStrings;
	typedef std::vector< Tle > TleObjects;
	TleObjects tleObjects;

	while ( std::getline( catalogFile, catalogLine ) )
	{
		TleStrings tleStrings;

		tleStrings.push_back( catalogLine );

		std::getline( catalogFile, catalogLine );
		tleStrings.push_back( catalogLine );

		if ( input.tleLines == 3 )
		{
			std::getline( catalogFile, catalogLine );
			tleStrings.push_back( catalogLine );
			tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ], tleStrings[ 2 ] ) );
		}
		else
		{
			tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ] ) );
		}
	}

    catalogFile.close( );
	std::cout << "# TLE objects:                " << tleObjects.size( ) << std::endl;

	// TODO: Check if database already exists.
	// if ( std::ifstream( input.databasePath ).good( ) &&  ) 

	// Open database in read/write mode.
	SQLite::Database database( 
		input.databasePath.c_str( ), SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    // Create table for Lambert scanner results in SQLite database.
	createLambertScannerTable( database );

	// Set gravitational parameter used by Lambert targeter.
	const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter 
    		  << " kg m^3 s_2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                               Output                             " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;    		  

	// Start SQL transaction.
	SQLite::Transaction transaction( database );

	// Setup insert query.
	std::ostringstream lambertScannerTableInsert;
    lambertScannerTableInsert
		<< "INSERT INTO lambert_scanner VALUES ("
		<< "NULL,"
		<< ":departureObjectId,"
		<< ":departureEpoch,"
		<< ":departurePositionX,"
		<< ":departurePositionY,"
		<< ":departurePositionZ,"
		<< ":departureVelocityX,"
		<< ":departureVelocityY,"
		<< ":departureVelocityZ,"
		<< ":departureSemiMajorAxis,"
		<< ":departureEccentricity,"
		<< ":departureInclination,"		
		<< ":departureArgumentOfPeriapsis,"
		<< ":departureLongitudeOfAscendingNode,"
		<< ":departureTrueAnomaly,"
		<< ":departureDeltaVX,"
		<< ":departureDeltaVY,"
		<< ":departureDeltaVZ,"
		<< ":arrivalObjectId,"
		<< ":arrivalPositionX,"
		<< ":arrivalPositionY,"
		<< ":arrivalPositionZ,"
		<< ":arrivalVelocityX,"
		<< ":arrivalVelocityY,"
		<< ":arrivalVelocityZ,"
		<< ":arrivalSemiMajorAxis,"
		<< ":arrivalEccentricity,"
		<< ":arrivalInclination,"		
		<< ":arrivalArgumentOfPeriapsis,"
		<< ":arrivalLongitudeOfAscendingNode,"
		<< ":arrivalTrueAnomaly,"
		<< ":arrivalDeltaVX,"
		<< ":arrivalDeltaVY,"
		<< ":arrivalDeltaVZ,"
		<< ":timeOfFlight,"
		<< ":revolutions,"		
		<< ":transferSemiMajorAxis,"
		<< ":transferEccentricity,"
		<< ":transferInclination,"		
		<< ":transferArgumentOfPeriapsis,"
		<< ":transferLongitudeOfAscendingNode,"
		<< ":transferTrueAnomaly,"
		<< ":transferDeltaV);";

    SQLite::Statement query( database, lambertScannerTableInsert.str( ) );

    std::cout << "Running simulations ... " << std::endl;

	// Loop over TLE objects and compute transfers based on Lambert targeter across time-of-flight
	// grid.
    for ( unsigned int departureObjectCounter = 0; 
    	  departureObjectCounter < tleObjects.size( ); 
    	  departureObjectCounter++ )
    {
    	// Compute departure state.
    	Tle departureObject = tleObjects[ departureObjectCounter ];
    	SGP4 sgp4Departure( departureObject );
		Eci tleDepartureState = sgp4Departure.FindPosition( departureObject.Epoch( ) );

		kep_toolbox::array3D departurePosition;
		departurePosition[ sam::xPositionIndex ] = tleDepartureState.Position( ).x;
		departurePosition[ sam::yPositionIndex ] = tleDepartureState.Position( ).y;
		departurePosition[ sam::zPositionIndex ] = tleDepartureState.Position( ).z;

		std::vector< double > departureState( 6 );
		departureState[ sam::xPositionIndex ] = tleDepartureState.Position( ).x;
		departureState[ sam::yPositionIndex ] = tleDepartureState.Position( ).y;
		departureState[ sam::zPositionIndex ] = tleDepartureState.Position( ).z;
		departureState[ sam::xVelocityIndex ] = tleDepartureState.Velocity( ).x;
		departureState[ sam::yVelocityIndex ] = tleDepartureState.Velocity( ).y;
		departureState[ sam::zVelocityIndex ] = tleDepartureState.Velocity( ).z;

		const std::vector< double > departureStateKepler 
			= sam::convertCartesianToKeplerianElements( 
				departureState, earthGravitationalParameter );

		// Loop over arrival objects.
	    for ( unsigned int arrivalObjectNumber = 0; 
	    	  arrivalObjectNumber < tleObjects.size( ); 
	    	  arrivalObjectNumber++ )
	    {
	    	// Skip the case of the departure and arrival objects being the same.
	    	if ( departureObjectCounter == arrivalObjectNumber )
	    	{
	    		continue;
	    	}

	    	Tle arrivalObject = tleObjects[ arrivalObjectNumber ];
	    	SGP4 sgp4Arrival( arrivalObject );    	

	    	// Loop over time-of-flight grid.
	    	for ( int timeOfFlightCounter = 0; 
	    		  timeOfFlightCounter < input.timeOfFlightSteps; 
	    		  timeOfFlightCounter++ )
			{
		    	const double timeOfFlight 
		    		= input.timeOfFlightMinimum + timeOfFlightCounter * input.timeOfFlightStepSize;

				Eci tleArrivalState = sgp4Arrival.FindPosition( 
					departureObject.Epoch( ) + timeOfFlight );

				kep_toolbox::array3D arrivalPosition;
				arrivalPosition[ sam::xPositionIndex ] = tleArrivalState.Position( ).x;
				arrivalPosition[ sam::yPositionIndex ] = tleArrivalState.Position( ).y;
				arrivalPosition[ sam::zPositionIndex ] = tleArrivalState.Position( ).z;

				std::vector< double > arrivalState( 6 );
				arrivalState[ sam::xPositionIndex ] = tleArrivalState.Position( ).x;
				arrivalState[ sam::yPositionIndex ] = tleArrivalState.Position( ).y;
				arrivalState[ sam::zPositionIndex ] = tleArrivalState.Position( ).z;
				arrivalState[ sam::xVelocityIndex ] = tleArrivalState.Velocity( ).x;
				arrivalState[ sam::yVelocityIndex ] = tleArrivalState.Velocity( ).y;
				arrivalState[ sam::zVelocityIndex ] = tleArrivalState.Velocity( ).z;

				const std::vector< double > arrivalStateKepler 
					= sam::convertCartesianToKeplerianElements( 
						arrivalState, earthGravitationalParameter );

				kep_toolbox::lambert_problem targeter( departurePosition, 
													   arrivalPosition, 
													   timeOfFlight, 
													   earthGravitationalParameter,
													   !input.isPrograde,
													   input.revolutionsMaximum );

int revolution = 0;

 				// for ( int revolution = 0; revolution < targeter.get_Nmax( ) + 1; revolution++ )
				// {
					const kep_toolbox::array3D transferDepartureVelocity 
						= targeter.get_v1( )[ revolution ];
					const kep_toolbox::array3D transferArrivalVelocity 
						= targeter.get_v2( )[ revolution ];

					std::vector< double > transferState( 6 );
					transferState[ sam::xPositionIndex ] = departureState[ sam::xPositionIndex ];
					transferState[ sam::yPositionIndex ] = departureState[ sam::yPositionIndex ];
					transferState[ sam::zPositionIndex ] = departureState[ sam::zPositionIndex ];
					transferState[ sam::xVelocityIndex ] = transferDepartureVelocity[ 0 ];
					transferState[ sam::yVelocityIndex ] = transferDepartureVelocity[ 1 ];
					transferState[ sam::zVelocityIndex ] = transferDepartureVelocity[ 2 ];

					const std::vector< double > transferStateKepler 
						= sam::convertCartesianToKeplerianElements( 
							transferState, earthGravitationalParameter );				

					kep_toolbox::array3D departureDeltaV;
					departureDeltaV[ 0 ] = transferDepartureVelocity[ 0 ] - departureState[ 3 ];
					departureDeltaV[ 1 ] = transferDepartureVelocity[ 1 ] - departureState[ 4 ];
					departureDeltaV[ 2 ] = transferDepartureVelocity[ 2 ] - departureState[ 5 ];

					kep_toolbox::array3D arrivalDeltaV;
					arrivalDeltaV[ 0 ] = arrivalState[ 3 ] - transferArrivalVelocity[ 0 ];
					arrivalDeltaV[ 1 ] = arrivalState[ 4 ] - transferArrivalVelocity[ 1 ];
					arrivalDeltaV[ 2 ] = arrivalState[ 5 ] - transferArrivalVelocity[ 2 ];

					const double transferDeltaV = sml::norm< double >( departureDeltaV ) 
						  						  + sml::norm< double >( arrivalDeltaV );

					// Bind values to SQL insert query.
					query.bind( ":departureObjectId", 
								static_cast< int >( departureObject.NoradNumber( ) ) );
					query.bind( ":departureEpoch", departureObject.Epoch( ).ToJulian( ) );
					query.bind( ":departurePositionX", departureState[ sam::xPositionIndex ] );
					query.bind( ":departurePositionY", departureState[ sam::yPositionIndex ] );
					query.bind( ":departurePositionZ", departureState[ sam::zPositionIndex ] );
					query.bind( ":departureVelocityX", departureState[ sam::xVelocityIndex ] );
					query.bind( ":departureVelocityY", departureState[ sam::yVelocityIndex ] );
					query.bind( ":departureVelocityZ", departureState[ sam::zVelocityIndex ] );
					query.bind( ":departureSemiMajorAxis", 
						departureStateKepler[ sam::semiMajorAxisIndex ] );
					query.bind( ":departureEccentricity", 
						departureStateKepler[ sam::eccentricityIndex ] );
					query.bind( ":departureInclination", 
						departureStateKepler[ sam::inclinationIndex ] );
					query.bind( ":departureArgumentOfPeriapsis", 
						departureStateKepler[ sam::argumentOfPeriapsisIndex ] );
					query.bind( ":departureLongitudeOfAscendingNode", 
						departureStateKepler[ sam::longitudeOfAscendingNodeIndex ] );
					query.bind( ":departureTrueAnomaly", 
						departureStateKepler[ sam::trueAnomalyIndex ] );				
					query.bind( ":departureDeltaVX", departureDeltaV[ 0 ] );
					query.bind( ":departureDeltaVY", departureDeltaV[ 1 ] );
					query.bind( ":departureDeltaVZ", departureDeltaV[ 2 ] );
					query.bind( ":arrivalObjectId", 
						static_cast< int >( arrivalObject.NoradNumber( ) ) );
					query.bind( ":arrivalPositionX", arrivalState[ sam::xPositionIndex ] );
					query.bind( ":arrivalPositionY", arrivalState[ sam::yPositionIndex ] );
					query.bind( ":arrivalPositionZ", arrivalState[ sam::zPositionIndex ] );
					query.bind( ":arrivalVelocityX", arrivalState[ sam::xVelocityIndex ] );
					query.bind( ":arrivalVelocityY", arrivalState[ sam::yVelocityIndex ] );
					query.bind( ":arrivalVelocityZ", arrivalState[ sam::zVelocityIndex ] );
					query.bind( ":arrivalSemiMajorAxis", 
						arrivalStateKepler[ sam::semiMajorAxisIndex ] );
					query.bind( ":arrivalEccentricity", 
						arrivalStateKepler[ sam::eccentricityIndex ] );
					query.bind( ":arrivalInclination", 
						arrivalStateKepler[ sam::inclinationIndex ] );
					query.bind( ":arrivalArgumentOfPeriapsis", 
						arrivalStateKepler[ sam::argumentOfPeriapsisIndex ] );
					query.bind( ":arrivalLongitudeOfAscendingNode",
						arrivalStateKepler[ sam::longitudeOfAscendingNodeIndex ] );
					query.bind( ":arrivalTrueAnomaly", 
						arrivalStateKepler[ sam::trueAnomalyIndex ] );		
					query.bind( ":arrivalDeltaVX", arrivalDeltaV[ 0 ] );
					query.bind( ":arrivalDeltaVY", arrivalDeltaV[ 1 ] );
					query.bind( ":arrivalDeltaVZ", arrivalDeltaV[ 2 ] );
					query.bind( ":timeOfFlight", timeOfFlight );
					query.bind( ":revolutions", revolution );								
					query.bind( ":transferSemiMajorAxis", 
						transferStateKepler[ sam::semiMajorAxisIndex ] );
					query.bind( ":transferEccentricity", 
						transferStateKepler[ sam::eccentricityIndex ] );
					query.bind( ":transferInclination", 
						transferStateKepler[ sam::inclinationIndex ] );
					query.bind( ":transferArgumentOfPeriapsis", 
						transferStateKepler[ sam::argumentOfPeriapsisIndex ] );
					query.bind( ":transferLongitudeOfAscendingNode", 
						transferStateKepler[ sam::longitudeOfAscendingNodeIndex ] );
					query.bind( ":transferTrueAnomaly", 
						transferStateKepler[ sam::trueAnomalyIndex ] );
					query.bind( ":transferDeltaV", transferDeltaV );

					// Execute insert query.
					query.executeStep( );
					
					// Reset SQL insert query.
					query.reset( );					
				// }
			}
	    }    	
    }

    std::cout << "Simulations executed successfully!" << std::endl;
    std::cout << "Populating database ... " << std::endl;

	// Commit transaction.
    transaction.commit( );

    std::cout << "Database populated successfully!" << std::endl;

    // Check if shortlist file should be created; call function to write output.
    if ( input.shortlistNumber > 0 )
    {
    	std::cout << "Writing shortlist to file ... " << std::endl;
    	writeTransferShortlist( database, input.shortlistNumber, input.shortlistPath );
    	std::cout << "Shortlist file created successfully!" << std::endl;
    }
}

//! Create Lambert scanner table.
void createLambertScannerTable( SQLite::Database& database )
{
	// Set up SQL command to create table to store results from Lambert scanner.
	std::ostringstream lambertScannerTableCreate;
	lambertScannerTableCreate
		<< "CREATE TABLE IF NOT EXISTS lambert_scanner (" 
		<< "\"transferId\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
		<< "\"departureObjectId\" INTEGER NOT NULL,"		
		<< "\"departureEpoch\" REAL NOT NULL,"
		<< "\"departurePositionX\" REAL NOT NULL,"
		<< "\"departurePositionY\" REAL NOT NULL,"
		<< "\"departurePositionZ\" REAL NOT NULL,"
		<< "\"departureVelocityX\" REAL NOT NULL,"
		<< "\"departureVelocityY\" REAL NOT NULL,"
		<< "\"departureVelocityZ\" REAL NOT NULL,"	
		<< "\"departureSemiMajorAxis\" REAL NOT NULL,"
		<< "\"departureEccentricity\" REAL NOT NULL,"
		<< "\"departureInclination\" REAL NOT NULL,"
		<< "\"departureArgumentOfPeriapsis\" REAL NOT NULL,"
		<< "\"departureLongitudeOfAscendingNode\" REAL NOT NULL,"
		<< "\"departureTrueAnomaly\" REAL NOT NULL,"
		<< "\"departureDeltaVX\" REAL NOT NULL,"
		<< "\"departureDeltaVY\" REAL NOT NULL,"
		<< "\"departureDeltaVZ\" REAL NOT NULL,"	
		<< "\"arrivalObjectId\" INTEGER NOT NULL,"
		<< "\"arrivalPositionX\" REAL NOT NULL,"
		<< "\"arrivalPositionY\" REAL NOT NULL,"
		<< "\"arrivalPositionZ\" REAL NOT NULL,"
		<< "\"arrivalVelocityX\" REAL NOT NULL,"
		<< "\"arrivalVelocityY\" REAL NOT NULL,"
		<< "\"arrivalVelocityZ\" REAL NOT NULL,"
		<< "\"arrivalSemiMajorAxis\" REAL NOT NULL,"
		<< "\"arrivalEccentricity\" REAL NOT NULL,"
		<< "\"arrivalInclination\" REAL NOT NULL,"
		<< "\"arrivalArgumentOfPeriapsis\" REAL NOT NULL,"
		<< "\"arrivalLongitudeOfAscendingNode\" REAL NOT NULL,"
		<< "\"arrivalTrueAnomaly\" REAL NOT NULL,"		
		<< "\"arrivalDeltaVX\" REAL NOT NULL,"
		<< "\"arrivalDeltaVY\" REAL NOT NULL,"
		<< "\"arrivalDeltaVZ\" REAL NOT NULL,"
		<< "\"timeOfFlight\" REAL NOT NULL,"
		<< "\"revolutions\" REAL NOT NULL,"		
		<< "\"transferSemiMajorAxis\" REAL NOT NULL,"
		<< "\"transferEccentricity\" REAL NOT NULL,"
		<< "\"transferInclination\" REAL NOT NULL,"
		<< "\"transferArgumentOfPeriapsis\" REAL NOT NULL,"
		<< "\"transferLongitudeOfAscendingNode\" REAL NOT NULL,"
		<< "\"transferTrueAnomaly\" REAL NOT NULL,"		
		<< "\"transferDeltaV\");";

	// Execute command to create table.
    database.exec( lambertScannerTableCreate.str( ).c_str( ) );

    // Execute commant to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE UNIQUE INDEX IF NOT EXISTS \"transferDeltaV\" on "
    						  << "lambert_scanner (transferDeltaV ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

	if ( !database.tableExists( "lambert_scanner" ) )
    {
        std::cerr << "ERROR: Creating table 'lambert_scanner' failed!" << std::endl;
        throw;
    }
}

//! Write transfer shortlist to file.
void writeTransferShortlist( 
	SQLite::Database& database, const int shortlistNumber, const std::string shortlistPath )
{
	// Fetch transfers to include in shortlist. Database table is sorted by transferDeltaV, in
	// ascending order.
	std::ostringstream shortlistSelect;
	shortlistSelect << "SELECT * FROM lambert_scanner ORDER BY transferDeltaV ASC LIMIT "
					<< shortlistNumber;
	SQLite::Statement query( database, shortlistSelect.str( ) );
	
	std::ofstream shortlistFile( shortlistPath.c_str( ) );

	// Print file header.
	shortlistFile << "transferId,"
				  << "departureObjectId,"		
				  << "departureEpoch,"
				  << "departurePositionX,"
				  << "departurePositionY,"
				  << "departurePositionZ,"
				  << "departureVelocityX,"
				  << "departureVelocityY,"
				  << "departureVelocityZ,"	
				  << "departureSemiMajorAxis,"
				  << "departureEccentricity,"
				  << "departureInclination,"
				  << "departureArgumentOfPeriapsis,"
				  << "departureLongitudeOfAscendingNode,"
				  << "departureTrueAnomaly,"
				  << "departureDeltaVX,"
				  << "departureDeltaVY,"
				  << "departureDeltaVZ,"	
				  << "arrivalObjectId,"
				  << "arrivalPositionX,"
				  << "arrivalPositionY,"
				  << "arrivalPositionZ,"
				  << "arrivalVelocityX,"
				  << "arrivalVelocityY,"
				  << "arrivalVelocityZ,"
				  << "arrivalSemiMajorAxis,"
				  << "arrivalEccentricity,"
				  << "arrivalInclination,"
				  << "arrivalArgumentOfPeriapsis,"
				  << "arrivalLongitudeOfAscendingNode,"
				  << "arrivalTrueAnomaly,"		
				  << "arrivalDeltaVX,"
				  << "arrivalDeltaVY,"
				  << "arrivalDeltaVZ,"
				  << "timeOfFlight,"
				  << "revolutions,"		
				  << "transferSemiMajorAxis,"
				  << "transferEccentricity,"
				  << "transferInclination,"
				  << "transferArgumentOfPeriapsis,"
				  << "transferLongitudeOfAscendingNode,"
				  << "transferTrueAnomaly,"		
				  << "transferDeltaV"
				  << std::endl;

	// Loop through data retrieved from database and write to file.
	while( query.executeStep( ) )
	{
		const int 	 transferId 						= query.getColumn( 0 );
		const int 	 departureObjectId 					= query.getColumn( 1 );
		const double departureEpoch 					= query.getColumn( 2 );
		const double departurePositionX 				= query.getColumn( 3 );
		const double departurePositionY 				= query.getColumn( 4 );
		const double departurePositionZ 				= query.getColumn( 5 );
		const double departureVelocityX 				= query.getColumn( 6 );
		const double departureVelocityY 				= query.getColumn( 7 );
		const double departureVelocityZ 				= query.getColumn( 8 );
		const double departureSemiMajorAxis 			= query.getColumn( 9 );
		const double departureEccentricity 				= query.getColumn( 10 );
		const double departureInclination 				= query.getColumn( 11 );
		const double departureArgumentOfPeriapsis 		= query.getColumn( 12 );
		const double departureLongitudeOfAscendingNode 	= query.getColumn( 13 );
		const double departureTrueAnomaly 				= query.getColumn( 14 );
		const double departureDeltaVX 					= query.getColumn( 15 );
		const double departureDeltaVY 					= query.getColumn( 16 );
		const double departureDeltaVZ 					= query.getColumn( 17 );
		const int 	 arrivalObjectId 					= query.getColumn( 18 );
		const double arrivalPositionX 					= query.getColumn( 19 );
		const double arrivalPositionY 					= query.getColumn( 20 );
		const double arrivalPositionZ 					= query.getColumn( 21 );
		const double arrivalVelocityX 					= query.getColumn( 22 );
		const double arrivalVelocityY 					= query.getColumn( 23 );
		const double arrivalVelocityZ 					= query.getColumn( 24 );
		const double arrivalSemiMajorAxis 				= query.getColumn( 25 );
		const double arrivalEccentricity 				= query.getColumn( 26 );
		const double arrivalInclination 				= query.getColumn( 27 );
		const double arrivalArgumentOfPeriapsis 		= query.getColumn( 28 );
		const double arrivalLongitudeOfAscendingNode 	= query.getColumn( 29 );
		const double arrivalTrueAnomaly 				= query.getColumn( 30 );
		const double arrivalDeltaVX 					= query.getColumn( 31 );
		const double arrivalDeltaVY 					= query.getColumn( 32 );
		const double arrivalDeltaVZ 					= query.getColumn( 33 );		
		const double timeOfFlight 						= query.getColumn( 34 );
		const int 	 revolutions 						= query.getColumn( 35 );		
		const double transferSemiMajorAxis 				= query.getColumn( 36 );
		const double transferEccentricity 				= query.getColumn( 37 );
		const double transferInclination 				= query.getColumn( 38 );
		const double transferArgumentOfPeriapsis 		= query.getColumn( 39 );
		const double transferLongitudeOfAscendingNode 	= query.getColumn( 40 );
		const double transferTrueAnomaly 				= query.getColumn( 41 );
		const double transferDeltaV 					= query.getColumn( 42 );
	

		shortlistFile << transferId << ","
					  << departureObjectId << ","
					  << departureEpoch << ","
					  << departurePositionX << ","
					  << departurePositionY << ","
					  << departurePositionZ << ","
					  << departureVelocityX << ","
					  << departureVelocityY << ","
					  << departureVelocityZ << ","
					  << departureSemiMajorAxis << ","
					  << departureEccentricity << ","
					  << departureInclination << ","
					  << departureArgumentOfPeriapsis << ","
					  << departureLongitudeOfAscendingNode << ","
					  << departureTrueAnomaly << ","
					  << departureDeltaVX << ","
					  << departureDeltaVY << ","
					  << departureDeltaVZ << ","
					  << arrivalObjectId << ","
					  << arrivalPositionX << ","
					  << arrivalPositionY << ","
					  << arrivalPositionZ << ","
					  << arrivalVelocityX << ","
					  << arrivalVelocityY << ","
					  << arrivalVelocityZ << ","
					  << arrivalSemiMajorAxis << ","
					  << arrivalEccentricity << ","
					  << arrivalInclination << ","
					  << arrivalArgumentOfPeriapsis << ","
					  << arrivalLongitudeOfAscendingNode << ","
					  << arrivalTrueAnomaly << ","
					  << arrivalDeltaVX << ","
					  << arrivalDeltaVY << ","
					  << arrivalDeltaVZ << ","
					  << timeOfFlight << ","
					  << revolutions << ","	
					  << transferSemiMajorAxis << ","
					  << transferEccentricity << ","
					  << transferInclination << ","
					  << transferArgumentOfPeriapsis << ","
					  << transferLongitudeOfAscendingNode << ","
					  << transferTrueAnomaly << ","
					  << transferDeltaV
					  << std::endl;
	}

	shortlistFile.close( );
}

//! Check Lambert scanner input parameters.
LambertScannerInput checkLambertScannerInput( const rapidjson::Document& input )
{
	// Fetch path to TLE catalog file.
	rapidjson::Value::ConstMemberIterator catalogPathIterator = input.FindMember( "catalog" );
    if ( catalogPathIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"catalog\" could not be found in config file!" << std::endl;
        throw;        
    }
    const std::string catalogPath = catalogPathIterator->value.GetString( );    
    std::cout << "Catalog:                      " << catalogPath << std::endl;

    // Fetch number of lines for TLE entries in catalog.
   	rapidjson::Value::ConstMemberIterator tleLinesIterator = input.FindMember( "tle_lines" );
	int tleLines = 3;

    if ( catalogPathIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"tle_lines\" could not be found in config file!" << std::endl;
        throw;        
    }
    else
    {    	
    	tleLines = tleLinesIterator->value.GetInt( );
	    if ( tleLines < 2 || tleLines > 3 )
	    {
	        std::cerr << "ERROR: \"tle_lines\" can only be set to 2 or 3!" << std::endl;
	        throw;        
	    }    	
    }

	// Fetch database path.
	rapidjson::Value::ConstMemberIterator databasePathIterator = input.FindMember( "database" );
    if ( databasePathIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"database\" could not be found in config file!" << std::endl;
        throw;        
    }
    const std::string databasePath = databasePathIterator->value.GetString( );
    std::cout << "Database:                     " << databasePath << std::endl;

	// Fetch grid to sample time-of-flight for transfer.
	rapidjson::Value::ConstMemberIterator timeOfFlightGridIterator 
		= input.FindMember( "time_of_flight_grid" );
    if ( catalogPathIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"time_of_flight_grid\" could not be found in config file!" 
        		  << std::endl;
        throw;        
    }
    const double timeOfFlightMinimum = timeOfFlightGridIterator->value[ 0 ].GetDouble( );    
    std::cout << "Time-of-flight (min):         " << timeOfFlightMinimum << " s" << std::endl;	
    const double timeOfFlightMaximum = timeOfFlightGridIterator->value[ 1 ].GetDouble( );    
    std::cout << "Time-of-flight (max):         " << timeOfFlightMaximum << " s" << std::endl;	
    const int timeOfFlightSteps = timeOfFlightGridIterator->value[ 2 ].GetInt( );    
    std::cout << "Time-of-flight (# steps):     " << timeOfFlightSteps << std::endl;	
	const double timeOfFlightStepSize 
		= static_cast< int >( ( timeOfFlightMaximum - timeOfFlightMinimum ) / timeOfFlightSteps );
    std::cout << "Time-of-flight (step size):   " << timeOfFlightStepSize << " s" << std::endl;  

	// Fetch flag indicating if prograde transfers should be computed. If set to false, retrograde
	// will be computed.
	rapidjson::Value::ConstMemberIterator isProgradeIterator = input.FindMember( "is_prograde" );
    if ( isProgradeIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"is_prograde\" could not be found in config file!" << std::endl;
        throw;        
    }
    const bool isPrograde = isProgradeIterator->value.GetInt( );    
    std::cout << "Prograde?:                    ";
    if ( !isPrograde )
    {
    	std::cout << "true" << std::endl;
    }
    else
    {
    	std::cout << "false" << std::endl;    	
    }

	// Fetch maximum number of revolutions.
	rapidjson::Value::ConstMemberIterator revolutionsMaximumIterator 
		= input.FindMember( "revolutions_maximum" );
    if ( revolutionsMaximumIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"revolutions_maximum\" could not be found in config file!" 
        		  << std::endl;
        throw;        
    }
    const int revolutionsMaximum = revolutionsMaximumIterator->value.GetInt( );    
    std::cout << "Revolutions (max):            " << revolutionsMaximum << std::endl;

    // Fetch number of transfers to include in shortlist.
	// Check if "shortlist_number" is defined in input file, not set to 0, and not 
	// negative (error).
	rapidjson::Value::ConstMemberIterator shortListNumberIterator 
		= input.FindMember( "shortlist_number" );
	int shortlistNumber = 0;		
    if ( shortListNumberIterator != input.MemberEnd( ) )
    {
    	shortlistNumber = shortListNumberIterator->value.GetInt( );  
		if ( shortlistNumber != 0 )
		{
			if ( shortlistNumber < 0 )
			{
		        std::cerr << "ERROR: \"shortlist_number\" cannot be negative!" << std::endl;
		        throw;      				
			}
			else
			{
			    std::cout << "Shortlist #:   		      " << shortlistNumber << std::endl;
			}
		}
    }

    // Fetch shortlist filename.
	rapidjson::Value::ConstMemberIterator shortlistIterator = input.FindMember( "shortlist" );
    if ( shortlistIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"shortlist\" could not be found in config file!" << std::endl;
        throw;        
    }
    const std::string shortlistPath = shortlistIterator->value.GetString( );
    if ( shortlistPath.empty( ) )
    {
    	std::cerr << "ERROR: \"shortlist\" cannot be empty!" << std::endl;
    	throw;
    } 
    std::cout << "Shortlist:                    " << shortlistPath << std::endl;

    // Return data struct with verified parameters.
    return LambertScannerInput( catalogPath,
    							tleLines,
    							databasePath,
    							timeOfFlightMinimum,
    							timeOfFlightMaximum,
    							timeOfFlightSteps,
    							timeOfFlightStepSize,
    							isPrograde,
    							revolutionsMaximum,
    							shortlistNumber,
    							shortlistPath );
}

//! Fetch details of specific debris-to-debris Lambert transfer.
void fetchLambertTransfer( const rapidjson::Document& input )
{
	// Fetch database path.
	rapidjson::Value::ConstMemberIterator databasePathIterator = input.FindMember( "database" );
    if ( databasePathIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"database\" could not be found in config file!" << std::endl;
        throw;        
    }
    const std::string databasePath = databasePathIterator->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    // Fetch path to output file.
   	rapidjson::Value::ConstMemberIterator outputFileIterator = input.FindMember( "output_file" );
    if ( outputFileIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"output_file\" could not be found in config file!" << std::endl;
        throw;        
    }
    const std::string outputFilePath = outputFileIterator->value.GetString( );
    std::cout << "Output                        " << outputFilePath << std::endl;

	// Fetch transfer ID.
	rapidjson::Value::ConstMemberIterator transferIdIterator = input.FindMember( "transfer_id" );
    if ( transferIdIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"transfer_id\" could not be found in config file!" << std::endl;
        throw;        
    }
    const int transferId = transferIdIterator->value.GetInt( );
    std::cout << "Transfer ID                   " << transferId << std::endl;

	// Fetch time step.
	rapidjson::Value::ConstMemberIterator timeStepIterator = input.FindMember( "time_step" );
    if ( timeStepIterator == input.MemberEnd( ) )
    {
        std::cerr << "ERROR: \"time_step\" could not be found in config file!" << std::endl;
        throw;        
    }
    const int timeStep = timeStepIterator->value.GetDouble( );
    std::cout << "Time step                     " << timeStep << std::endl;

	// Set gravitational parameter used by Lambert targeter.
	const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter 
    		  << " kg m^3 s_2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                               Output                             " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;    		  

	// Open database in read/write mode.
	SQLite::Database database( databasePath.c_str( ), SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

	// Retrieve data for specified transfer ID.
	std::ostringstream transferSelect;
	transferSelect << "SELECT * FROM lambert_scanner WHERE \"transferId\" = " << transferId << ";";
	SQLite::Statement query( database, transferSelect.str( ) );	
	query.executeStep( );

	const int 	 departureObjectId 					= query.getColumn( 1 );
	const double departureEpoch 					= query.getColumn( 2 );
	const double departurePositionX 				= query.getColumn( 3 );
	const double departurePositionY 				= query.getColumn( 4 );
	const double departurePositionZ 				= query.getColumn( 5 );
	const double departureVelocityX 				= query.getColumn( 6 );
	const double departureVelocityY 				= query.getColumn( 7 );
	const double departureVelocityZ 				= query.getColumn( 8 );
	const double departureSemiMajorAxis 			= query.getColumn( 9 );
	const double departureEccentricity 				= query.getColumn( 10 );
	const double departureInclination 				= query.getColumn( 11 );
	const double departureArgumentOfPeriapsis 		= query.getColumn( 12 );
	const double departureLongitudeOfAscendingNode 	= query.getColumn( 13 );
	const double departureTrueAnomaly 				= query.getColumn( 14 );
	const double departureDeltaVX 					= query.getColumn( 15 );
	const double departureDeltaVY 					= query.getColumn( 16 );
	const double departureDeltaVZ 					= query.getColumn( 17 );
	const int 	 arrivalObjectId 					= query.getColumn( 18 );
	const double arrivalPositionX 					= query.getColumn( 19 );
	const double arrivalPositionY 					= query.getColumn( 20 );
	const double arrivalPositionZ 					= query.getColumn( 21 );
	const double arrivalVelocityX 					= query.getColumn( 22 );
	const double arrivalVelocityY 					= query.getColumn( 23 );
	const double arrivalVelocityZ 					= query.getColumn( 24 );
	const double arrivalSemiMajorAxis 				= query.getColumn( 25 );
	const double arrivalEccentricity 				= query.getColumn( 26 );
	const double arrivalInclination 				= query.getColumn( 27 );
	const double arrivalArgumentOfPeriapsis 		= query.getColumn( 28 );
	const double arrivalLongitudeOfAscendingNode 	= query.getColumn( 29 );
	const double arrivalTrueAnomaly 				= query.getColumn( 30 );
	const double arrivalDeltaVX 					= query.getColumn( 31 );
	const double arrivalDeltaVY 					= query.getColumn( 32 );
	const double arrivalDeltaVZ 					= query.getColumn( 33 );		
	const double timeOfFlight 						= query.getColumn( 34 );
	const int 	 revolutions 						= query.getColumn( 35 );		
	const double transferSemiMajorAxis 				= query.getColumn( 36 );
	const double transferEccentricity 				= query.getColumn( 37 );
	const double transferInclination 				= query.getColumn( 38 );
	const double transferArgumentOfPeriapsis 		= query.getColumn( 39 );
	const double transferLongitudeOfAscendingNode 	= query.getColumn( 40 );
	const double transferTrueAnomaly 				= query.getColumn( 41 );
	const double transferDeltaV 					= query.getColumn( 42 );

	// Compute and store positions and velocities by propagating conic section (Kepler orbit).
	kep_toolbox::array3D position, velocity;

	position[ 0 ] = departurePositionX;
	position[ 1 ] = departurePositionY;
	position[ 2 ] = departurePositionZ;

	velocity[ 0 ] = departureVelocityX + departureDeltaVX;
	velocity[ 1 ] = departureVelocityY + departureDeltaVY;
	velocity[ 2 ] = departureVelocityZ + departureDeltaVZ;

	typedef std::map< double, kep_toolbox::array6D > StateHistory;
    StateHistory stateHistory;

    kep_toolbox::array6D state;
    state[ 0 ] = position[ 0 ];
    state[ 1 ] = position[ 1 ];
    state[ 2 ] = position[ 2 ];
    state[ 3 ] = velocity[ 0 ];
    state[ 4 ] = velocity[ 1 ];
    state[ 5 ] = velocity[ 2 ];

    stateHistory[ departureEpoch ] = state;      

    const int numberOfSteps = std::floor( timeOfFlight / timeStep );

    for ( int i = 0; i < numberOfSteps; i++ )
    {
	    kep_toolbox::propagate_lagrangian( 
	    	position, velocity, ( i + 1 ) * timeStep, earthGravitationalParameter );

	    state[ 0 ] = position[ 0 ];
	    state[ 1 ] = position[ 1 ];
	    state[ 2 ] = position[ 2 ];
	    state[ 3 ] = velocity[ 0 ];
	    state[ 4 ] = velocity[ 1 ];
	    state[ 5 ] = velocity[ 2 ];

	    stateHistory[ departureEpoch + ( i + 1 ) * timeStep ] = state;	    
    }

    if ( timeOfFlight - numberOfSteps * timeStep > std::numeric_limits< double >::epsilon( ) )
    {
		kep_toolbox::propagate_lagrangian( 
			position, 
			velocity, 
			timeOfFlight - numberOfSteps * timeStep, 
			earthGravitationalParameter );

		state[ 0 ] = position[ 0 ];
		state[ 1 ] = position[ 1 ];
		state[ 2 ] = position[ 2 ];
		state[ 3 ] = velocity[ 0 ];
		state[ 4 ] = velocity[ 1 ];
		state[ 5 ] = velocity[ 2 ];

		stateHistory[ departureEpoch + timeOfFlight ] = state;   	
    }

    // Write state history to output file.
	std::ofstream outputFile( outputFilePath.c_str( ) );
	outputFile << "epoch,xPosition [km],yPosition [km],zPosition [km],"
			   << "xVelocity [km],yVelocity [km],zVelocity" << std::endl;

	for ( StateHistory::iterator iteratorStateHistory = stateHistory.begin( );
		  iteratorStateHistory != stateHistory.end( );
		  iteratorStateHistory++ )
	{
		outputFile << iteratorStateHistory->first << ","
				   << iteratorStateHistory->second[ 0 ] << ","
				   << iteratorStateHistory->second[ 1 ] << ","
				   << iteratorStateHistory->second[ 2 ] << ","
				   << iteratorStateHistory->second[ 3 ] << ","
				   << iteratorStateHistory->second[ 4 ] << ","
				   << iteratorStateHistory->second[ 5 ] << std::endl;				   
	}

	outputFile.close( );
}

} // namespace d2d
