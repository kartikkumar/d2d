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
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

// #include <libsgp4/Eci.h>
// #include <libsgp4/Globals.h>
// #include <libsgp4/SGP4.h>
// #include <libsgp4/Tle.h>

#include "D2D/lambertScanner.hpp"

namespace d2d
{

//! Execute Lambert scanner.
void executeLambertScanner( const rapidjson::Document& configuration )
{
	// Fetch TLE catalog.
	rapidjson::Value::ConstMemberIterator catalogPathIterator 
		= configuration.FindMember( "catalog" );
    if ( catalogPathIterator == configuration.MemberEnd( ) )
    {
        std::cerr << "ERROR: Configuration option \"catalog\" could not be found in JSON input!" 
                  << std::endl;
        throw;        
    }
    const std::string catalogPath = catalogPathIterator->value.GetString( );    
    std::cout << "Catalog:       " << catalogPath << std::endl;

   	rapidjson::Value::ConstMemberIterator tleNumberOfLinesIterator 
		= configuration.FindMember( "tle_lines" );
	int tleNumberOfLines = 3;

    if ( catalogPathIterator != configuration.MemberEnd( ) )
    {
    	tleNumberOfLines = tleNumberOfLinesIterator->value.GetInt( );
    }

    if ( tleNumberOfLines < 2 || tleNumberOfLines > 3 )
    {
        std::cerr << "ERROR: Configuration option \"tle_lines\" can only be set to 2 or 3!" 
                  << std::endl;
        throw;        
    }

	std::ifstream catalogFile( catalogPath.c_str( ) );
	std::string catalogLine;
	typedef std::vector< std::string > TleObjectStrings;
	typedef std::vector< Tle > TleObjects;
	TleObjects tleObjects;

	while ( std::getline( catalogFile, catalogLine ) )
	{
		TleObjectStrings tleObjectStrings;

		if ( tleNumberOfLines == 3 )
		{
			tleObjectStrings.push_back( catalogLine );	
		}

		for ( unsigned int i = 0; i < 2; i++ )
		{
			if ( std::getline( catalogFile, catalogLine ) )
			{
				tleObjectStrings.push_back( catalogLine );
			}

			else
			{
		        std::cerr 
		        	<< "ERROR: TLE object " 
		        	<< tleObjectStrings[ 0 ].substr( 2, 68 ) << " is incomplete in catalog!" 
		            << std::endl;
		        throw;        				
			}
		}

		tleObjects.push_back( 
			Tle( tleObjectStrings[ 0 ], tleObjectStrings[ 1 ], tleObjectStrings[ 2 ] ) );
	}

    catalogFile.close( );

	// Fetch database path.
	rapidjson::Value::ConstMemberIterator databasePathIterator 
		= configuration.FindMember( "database" );
    if ( databasePathIterator == configuration.MemberEnd( ) )
    {
        std::cerr << "ERROR: Configuration option \"database\" could not be found in JSON input!" 
                  << std::endl;
        throw;        
    }
    const std::string databasePath = databasePathIterator->value.GetString( );
    std::cout << "Database:      " << databasePath << std::endl;

	// Open database in read/write mode.
	SQLite::Database database( databasePath.c_str( ), SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    // Create table for Lambert scanner results in SQLite database.
	createLambertScannerTable( database );

	// Set grid to sample time-of-flight for transfer.
	const double timeOfFlightMinimum = 1000.0;	
	const double timeOfFlightMaximum = 10000.0;	
	const double timeOfFlightStep    = 100.0;
	const int timeOfFlightSteps 
		= static_cast< int >( ( timeOfFlightMaximum - timeOfFlightMinimum ) / timeOfFlightStep );

	// Set gravitational parameter used by Lambert targeter.
	const double earthGravitationalParameter = kMU;

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
		<< ":departureDeltaVX,"
		<< ":departureDeltaVY,"
		<< ":departureDeltaVZ,"
		<< ":arrivalObjectId,"
		<< ":timeOfFlight,"
		<< ":arrivalPositionX,"
		<< ":arrivalPositionY,"
		<< ":arrivalPositionZ,"
		<< ":arrivalVelocityX,"
		<< ":arrivalVelocityY,"
		<< ":arrivalVelocityZ,"
		<< ":arrivalDeltaVX,"
		<< ":arrivalDeltaVY,"
		<< ":arrivalDeltaVZ,"
		<< ":transferDeltaV);";

    SQLite::Statement query( database, lambertScannerTableInsert.str( ) );	

	// Loop over TLE objects and compute transfers based on Lambert targeter across time-of-flight
	// grid.
    for ( unsigned int departureObjectNumber = 0; 
    	  departureObjectNumber < tleObjects.size( ); 
    	  departureObjectNumber++ )
    {
    	Tle departureObject = tleObjects[ departureObjectNumber ];
    	SGP4 sgp4Departure( departureObject );
		Eci departureState = sgp4Departure.FindPosition( departureObject.Epoch( ) );

		kep_toolbox::array3D departurePosition;
		departurePosition[ 0 ] = departureState.Position( ).x;
		departurePosition[ 1 ] = departureState.Position( ).y;
		departurePosition[ 2 ] = departureState.Position( ).z;

	    for ( unsigned int arrivalObjectNumber = 0; 
	    	  arrivalObjectNumber < tleObjects.size( ); 
	    	  arrivalObjectNumber++ )
	    {
	    	// Skip the case of the departure and arrival objects being the same.
	    	if ( departureObjectNumber == arrivalObjectNumber )
	    	{
	    		continue;
	    	}

	    	Tle arrivalObject = tleObjects[ arrivalObjectNumber ];
	    	SGP4 sgp4Arrival( arrivalObject );    	

	    	for ( unsigned int timeOfFlightStepNumber = 0; 
	    		  timeOfFlightStepNumber < timeOfFlightSteps; 
	    		  timeOfFlightStepNumber++ )
			{
		    	const double timeOfFlight 
		    		= timeOfFlightMinimum + timeOfFlightStepNumber * timeOfFlightStep;

				Eci arrivalState = sgp4Arrival.FindPosition( 
					departureObject.Epoch( ) + timeOfFlight );

				kep_toolbox::array3D arrivalPosition;
				arrivalPosition[ 0 ] = arrivalState.Position( ).x;
				arrivalPosition[ 1 ] = arrivalState.Position( ).y;
				arrivalPosition[ 2 ] = arrivalState.Position( ).z;

				kep_toolbox::lambert_problem targeter( departurePosition, 
													   arrivalPosition, 
													   timeOfFlight, 
													   earthGravitationalParameter );

				kep_toolbox::array3D departureVelocity;
				departureVelocity = targeter.get_v1( )[ 0 ];

				kep_toolbox::array3D arrivalVelocity;
				arrivalVelocity = targeter.get_v2( )[ 0 ];

				kep_toolbox::array3D departureDeltaV;
				departureDeltaV[ 0 ] = departureVelocity[ 0 ] - departureState.Velocity( ).x;
				departureDeltaV[ 1 ] = departureVelocity[ 1 ] - departureState.Velocity( ).y;
				departureDeltaV[ 2 ] = departureVelocity[ 2 ] - departureState.Velocity( ).z;

				kep_toolbox::array3D arrivalDeltaV;
				arrivalDeltaV[ 0 ] = arrivalVelocity[ 0 ] - arrivalState.Velocity( ).x;
				arrivalDeltaV[ 1 ] = arrivalVelocity[ 1 ] - arrivalState.Velocity( ).y;
				arrivalDeltaV[ 2 ] = arrivalVelocity[ 2 ] - arrivalState.Velocity( ).z;

				const double transferDeltaV 
				 = std::sqrt( departureDeltaV[ 0 ] * departureDeltaV[ 0 ]
				 			  + departureDeltaV[ 1 ] * departureDeltaV[ 1 ]
				 			  + departureDeltaV[ 2 ] * departureDeltaV[ 2 ] )
				   + std::sqrt( arrivalDeltaV[ 0 ] * arrivalDeltaV[ 0 ]
				 			  + arrivalDeltaV[ 1 ] * arrivalDeltaV[ 1 ]
				 			  + arrivalDeltaV[ 2 ] * arrivalDeltaV[ 2 ] );

				// Bind values to SQL insert query.
				query.bind( ":departureObjectId", 
							static_cast< int >( departureObject.NoradNumber( ) ) );
				query.bind( ":departureEpoch", departureObject.Epoch( ).ToJulian( ) );
				query.bind( ":departurePositionX", departurePosition[ 0 ] );
				query.bind( ":departurePositionY", departurePosition[ 1 ] );
				query.bind( ":departurePositionZ", departurePosition[ 2 ] );
				query.bind( ":departureVelocityX", departureVelocity[ 0 ] );
				query.bind( ":departureVelocityY", departureVelocity[ 1 ] );
				query.bind( ":departureVelocityZ", departureVelocity[ 2 ] );
				query.bind( ":departureDeltaVX", departureDeltaV[ 0 ] );
				query.bind( ":departureDeltaVY", departureDeltaV[ 1 ] );
				query.bind( ":departureDeltaVZ", departureDeltaV[ 2 ] );
				query.bind( ":arrivalObjectId", 
						    static_cast< int >( departureObject.NoradNumber( ) ) );
				query.bind( ":timeOfFlight", timeOfFlight );
				query.bind( ":arrivalPositionX", arrivalPosition[ 0 ] );
				query.bind( ":arrivalPositionY", arrivalPosition[ 1 ] );
				query.bind( ":arrivalPositionZ", arrivalPosition[ 2 ] );
				query.bind( ":arrivalVelocityX", arrivalVelocity[ 0 ] );
				query.bind( ":arrivalVelocityY", arrivalVelocity[ 1 ] );
				query.bind( ":arrivalVelocityZ", arrivalVelocity[ 2 ] );
				query.bind( ":arrivalDeltaVX", arrivalDeltaV[ 0 ] );
				query.bind( ":arrivalDeltaVY", arrivalDeltaV[ 1 ] );
				query.bind( ":arrivalDeltaVZ", arrivalDeltaV[ 2 ] );
				query.bind( ":transferDeltaV", transferDeltaV );

				// Execute insert query.
				query.executeStep( );

				// Reset SQL insert query.
				query.reset( );
			}
	    }    	
    }

	// Commit transaction.
    transaction.commit( );    
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
		<< "\"departureDeltaVX\" REAL NOT NULL,"
		<< "\"departureDeltaVY\" REAL NOT NULL,"
		<< "\"departureDeltaVZ\" REAL NOT NULL,"	
		<< "\"arrivalObjectId\" INTEGER NOT NULL,"
		<< "\"timeOfFlight\" REAL NOT NULL,"
		<< "\"arrivalPositionX\" REAL NOT NULL,"
		<< "\"arrivalPositionY\" REAL NOT NULL,"
		<< "\"arrivalPositionZ\" REAL NOT NULL,"
		<< "\"arrivalVelocityX\" REAL NOT NULL,"
		<< "\"arrivalVelocityY\" REAL NOT NULL,"
		<< "\"arrivalVelocityZ\" REAL NOT NULL,"	
		<< "\"arrivalDeltaVX\" REAL NOT NULL,"
		<< "\"arrivalDeltaVY\" REAL NOT NULL,"
		<< "\"arrivalDeltaVZ\" REAL NOT NULL,"	
		<< "\"transferDeltaV\");";

	// Execute command to create table.
    database.exec( lambertScannerTableCreate.str( ).c_str( ) );

	if ( database.tableExists( "lambert_scanner" ) )
    {
//        cout << "Table '" << testParticleCaseTableName << "' successfully created!" << endl; 
    }

    else
    {
        // ostringstream tableCreateError;
        // tableCreateError << "ERROR: Creating table '" << testParticleCaseTableName 
        //                  << "'' failed!";
        throw;
    }
}

} // namespace d2d
