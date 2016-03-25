/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/progress.hpp>

#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/TimeSpan.h>
#include <libsgp4/Tle.h>
#include <libsgp4/Vector.h>

#include <Atom/atom.hpp>
#include <Atom/convertCartesianStateToTwoLineElements.hpp>

#include <Astro/astro.hpp>
#include <Astro/constants.hpp>

#include "D2D/sgp4Scanner.hpp"
#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

namespace d2d
{

//! Execute sgp4_scanner.
void executeSGP4Scanner( const rapidjson::Document& config )
{
    std::cout.precision( 15 );
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const sgp4ScannerInput input = checkSGP4ScannerInput( config );

    // Set gravitational parameter used [km^3 s^-2].
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " km^3 s^-2" << std::endl;

    // Set mean radius used [km].
    const double earthMeanRadius = kXKMPER;
    std::cout << "Earth mean radius             " << earthMeanRadius << " km" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Open database in read/write mode.
    // N.B.: Database must already exist and contain a populated table called
    //       "lambert_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create sgp4_scanner_results table in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createSGP4ScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Fetch number of rows in lambert_scanner_results table.
    std::ostringstream lambertScannerTableSizeSelect;
    lambertScannerTableSizeSelect << "SELECT COUNT(*) FROM lambert_scanner_results;";
    const int lambertScannertTableSize
        = database.execAndGet( lambertScannerTableSizeSelect.str( ) );

    // Setup select query to fetch data from lambert_scanner_results table.
    std::ostringstream lambertScannerTableSelect;
    lambertScannerTableSelect << "SELECT * FROM lambert_scanner_results;";

    // Setup insert query to insert data into sgp4_scanner_results table.
    std::ostringstream sgp4ScannerTableInsert;
    sgp4ScannerTableInsert << "INSERT INTO sgp4_scanner_results VALUES ("
        << "NULL,"
        << ":lambert_transfer_id,"
        << ":departure_object_id,"         
        << ":arrival_object_id,"           
        << ":departure_epoch,"             
        << ":departure_semi_major_axis,"               
        << ":departure_eccentricity,"                  
        << ":departure_inclination,"                   
        << ":departure_argument_of_periapsis,"         
        << ":departure_longitude_of_ascending_node,"   
        << ":departure_true_anomaly,"                  
        << ":arrival_position_x,"
        << ":arrival_position_y,"
        << ":arrival_position_z,"
        << ":arrival_velocity_x,"
        << ":arrival_velocity_y,"
        << ":arrival_velocity_z,"
        << ":arrival_position_x_error,"
        << ":arrival_position_y_error,"
        << ":arrival_position_z_error,"
        << ":arrival_position_error,"
        << ":arrival_velocity_x_error,"
        << ":arrival_velocity_y_error,"
        << ":arrival_velocity_z_error,"
        << ":arrival_velocity_error,"
        << ":success"
        << ");";
    
    SQLite::Statement lambertQuery( database, lambertScannerTableSelect.str( ) );

    SQLite::Statement SGP4Query( database, sgp4ScannerTableInsert.str( ) );

    std::cout << "Propagating Lambert transfers using SGP4 and populating database ... "
              << std::endl;

    // Loop over rows in lambert_scanner_results table and propagate Lambert transfers using SGP4.
    boost::progress_display showProgress( lambertScannertTableSize );

    // Counters for different fail cases
    int virtualTleFailCounter = 0;
    int arrivalEpochPropagationFailCounter = 0;

    while ( lambertQuery.executeStep( ) )
    {
        const int      lambertTransferId           = lambertQuery.getColumn( 0 );
        const int      departureObjectId           = lambertQuery.getColumn( 1 );
        const int      arrivalObjectId             = lambertQuery.getColumn( 2 );

        const double   departureEpochJulian        = lambertQuery.getColumn( 3 );
        const double   timeOfFlight                = lambertQuery.getColumn( 4 );

        const double   departureSMA                = lambertQuery.getColumn( 13 );
        const double   departureEccentricity       = lambertQuery.getColumn( 14 );
        const double   departureInclination        = lambertQuery.getColumn( 15 );
        const double   departureAOP                = lambertQuery.getColumn( 16 );
        const double   departureRAAN               = lambertQuery.getColumn( 17 );
        const double   departureTA                 = lambertQuery.getColumn( 18 );

        const double   departurePositionX          = lambertQuery.getColumn( 7 );
        const double   departurePositionY          = lambertQuery.getColumn( 8 );
        const double   departurePositionZ          = lambertQuery.getColumn( 9 );
        const double   departureVelocityX          = lambertQuery.getColumn( 10 );
        const double   departureVelocityY          = lambertQuery.getColumn( 11 );
        const double   departureVelocityZ          = lambertQuery.getColumn( 12 );
        const double   departureDeltaVX            = lambertQuery.getColumn( 37 );
        const double   departureDeltaVY            = lambertQuery.getColumn( 38 );
        const double   departureDeltaVZ            = lambertQuery.getColumn( 39 );

        const double   lambertArrivalPositionX     = lambertQuery.getColumn( 19 );
        const double   lambertArrivalPositionY     = lambertQuery.getColumn( 20 );
        const double   lambertArrivalPositionZ     = lambertQuery.getColumn( 21 );
        const double   lambertArrivalVelocityX     = lambertQuery.getColumn( 22 );
        const double   lambertArrivalVelocityY     = lambertQuery.getColumn( 23 );
        const double   lambertArrivalVelocityZ     = lambertQuery.getColumn( 24 );
        const double   lambertArrivalDeltaVX       = lambertQuery.getColumn( 40 );
        const double   lambertArrivalDeltaVY       = lambertQuery.getColumn( 41 );
        const double   lambertArrivalDeltaVZ       = lambertQuery.getColumn( 42 );

        const double   lambertTotalDeltaV          = lambertQuery.getColumn( 43 );

        // Set up DateTime object for departure epoch using Julian date.
        // NB: 1721425.5 corresponds to the Gregorian epoch: 0001 Jan 01 00:00:00.0
        DateTime departureEpoch( ( departureEpochJulian - astro::ASTRO_GREGORIAN_EPOCH_IN_JULIAN_DAYS ) * TicksPerDay );

        // Get departure State for the transfer object
        Tle transferObjectTle;
        std::vector< double > transferObjectDepartureState( 6 );
        transferObjectDepartureState[ 0 ] = departurePositionX;
        transferObjectDepartureState[ 1 ] = departurePositionY;
        transferObjectDepartureState[ 2 ] = departurePositionZ;
        transferObjectDepartureState[ 3 ] = departureVelocityX + departureDeltaVX;
        transferObjectDepartureState[ 4 ] = departureVelocityY + departureDeltaVY;
        transferObjectDepartureState[ 5 ] = departureVelocityZ + departureDeltaVZ;

        // Calculate transfer object's velocity magnitude at the departure point
        std::vector< double > transferObjectDepartureVelocityVector( 3 );
        for ( int i = 0; i < 3; i++ )
        {
            transferObjectDepartureVelocityVector[ i ] = transferObjectDepartureState[ i + 3 ];
        }
        double transferObjectDepartureVelocityMagnitude = sml::norm< double >( transferObjectDepartureVelocityVector );

        // Filter out cases using velocity cut off given through input file
        if ( lambertTotalDeltaV > input.velocityCutOff )
        {
            // Bind zeroes to SQL insert sgp4Query
            SGP4Query.bind( ":lambert_transfer_id",                             lambertTransferId );
            SGP4Query.bind( ":departure_object_id",                             departureObjectId );
            SGP4Query.bind( ":arrival_object_id",                               arrivalObjectId );
            SGP4Query.bind( ":departure_epoch",                                 departureEpochJulian );

            SGP4Query.bind( ":departure_semi_major_axis",                       departureSMA );             
            SGP4Query.bind( ":departure_eccentricity",                          departureEccentricity );
            SGP4Query.bind( ":departure_inclination",                           departureInclination );
            SGP4Query.bind( ":departure_argument_of_periapsis",                 departureAOP );
            SGP4Query.bind( ":departure_longitude_of_ascending_node",           departureRAAN );              
            SGP4Query.bind( ":departure_true_anomaly",                          departureTA );
            
            SGP4Query.bind( ":arrival_position_x",                              0 );
            SGP4Query.bind( ":arrival_position_y",                              0 );
            SGP4Query.bind( ":arrival_position_z",                              0 );
            SGP4Query.bind( ":arrival_velocity_x",                              0 );
            SGP4Query.bind( ":arrival_velocity_y",                              0 );
            SGP4Query.bind( ":arrival_velocity_z",                              0 );
            SGP4Query.bind( ":arrival_position_x_error",                        0 );
            SGP4Query.bind( ":arrival_position_y_error",                        0 );
            SGP4Query.bind( ":arrival_position_z_error",                        0 );
            SGP4Query.bind( ":arrival_position_error",                          0 );
            SGP4Query.bind( ":arrival_velocity_x_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_y_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_z_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_error",                          0 );
            SGP4Query.bind( ":success",                                         0 );

            // execute insert SGP4Query
            SGP4Query.executeStep( );

            // Reset SQL insert SGP4Query
            SGP4Query.reset( );

            continue;
        }

        // Create virtual TLE for the transfer object's orbit from its departure state. 
        // This TLE will be propagated using the SGP4 Transfer.
        try
        {
            transferObjectTle = atom::convertCartesianStateToTwoLineElements< double, std::vector< double > >( transferObjectDepartureState, 
                                                                                                               departureEpoch );
        }
        catch( std::exception& virtualTleError )
        {
            // do nothing
        }

        // Check if transferObjectTle is correct or not
        const SGP4 sgp4Check( transferObjectTle );
        double absoluteTolerance = 1.0e-10;
        double relativeTolerance = 1.0e-8;
        bool testPassed = false;

        Eci propagatedDepartureStateEci = sgp4Check.FindPosition( 0.0 );
        Vector6 propagatedDepartureState;
        propagatedDepartureState[ 0 ] = propagatedDepartureStateEci.Position( ).x;
        propagatedDepartureState[ 1 ] = propagatedDepartureStateEci.Position( ).y;
        propagatedDepartureState[ 2 ] = propagatedDepartureStateEci.Position( ).z;
        propagatedDepartureState[ 3 ] = propagatedDepartureStateEci.Velocity( ).x;
        propagatedDepartureState[ 4 ] = propagatedDepartureStateEci.Velocity( ).y;
        propagatedDepartureState[ 5 ] = propagatedDepartureStateEci.Velocity( ).z;
        
        Vector6 trueDepartureState;
        for ( int i = 0; i < 6; i++ )
            trueDepartureState[ i ] = transferObjectDepartureState[ i ];

        testPassed = executeVirtualTleConvergenceTest( propagatedDepartureState,
                                                       trueDepartureState,
                                                       relativeTolerance,
                                                       absoluteTolerance );

        if ( testPassed == false )
        {
            // Bind values to SQL insert sgp4Query
            SGP4Query.bind( ":lambert_transfer_id",                             lambertTransferId );
            SGP4Query.bind( ":departure_object_id",                             departureObjectId );
            SGP4Query.bind( ":arrival_object_id",                               arrivalObjectId );
            SGP4Query.bind( ":departure_epoch",                                 departureEpochJulian );

            SGP4Query.bind( ":departure_semi_major_axis",                       departureSMA );             
            SGP4Query.bind( ":departure_eccentricity",                          departureEccentricity );
            SGP4Query.bind( ":departure_inclination",                           departureInclination );
            SGP4Query.bind( ":departure_argument_of_periapsis",                 departureAOP );
            SGP4Query.bind( ":departure_longitude_of_ascending_node",           departureRAAN );              
            SGP4Query.bind( ":departure_true_anomaly",                          departureTA );
            
            SGP4Query.bind( ":arrival_position_x",                              0 );
            SGP4Query.bind( ":arrival_position_y",                              0 );
            SGP4Query.bind( ":arrival_position_z",                              0 );
            SGP4Query.bind( ":arrival_velocity_x",                              0 );
            SGP4Query.bind( ":arrival_velocity_y",                              0 );
            SGP4Query.bind( ":arrival_velocity_z",                              0 );
            SGP4Query.bind( ":arrival_position_x_error",                        0 );
            SGP4Query.bind( ":arrival_position_y_error",                        0 );
            SGP4Query.bind( ":arrival_position_z_error",                        0 );
            SGP4Query.bind( ":arrival_position_error",                          0 );
            SGP4Query.bind( ":arrival_velocity_x_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_y_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_z_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_error",                          0 );
            SGP4Query.bind( ":success",                                         0 );

            // execute insert SGP4Query
            SGP4Query.executeStep( );

            // Reset SQL insert SGP4Query
            SGP4Query.reset( );
            
            ++virtualTleFailCounter;
            continue;
        }
            
        // Propagate transfer object using the SGP4 propagator
        const SGP4 sgp4( transferObjectTle );
        DateTime sgp4ArrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
        Vector eciPosition = Vector( );
        Vector eciVelocity = Vector( );
        Eci sgp4ArrivalStateEci( sgp4ArrivalEpoch, eciPosition, eciVelocity );
        try
        {
            sgp4ArrivalStateEci = sgp4.FindPosition( sgp4ArrivalEpoch );
        }
        catch( std::exception& sgp4PropagationError )
        {
            // Bind values to SQL insert sgp4Query
            SGP4Query.bind( ":lambert_transfer_id",                             lambertTransferId );
            SGP4Query.bind( ":departure_object_id",                             departureObjectId );
            SGP4Query.bind( ":arrival_object_id",                               arrivalObjectId );
            SGP4Query.bind( ":departure_epoch",                                 departureEpochJulian );

            SGP4Query.bind( ":departure_semi_major_axis",                       departureSMA );             
            SGP4Query.bind( ":departure_eccentricity",                          departureEccentricity );
            SGP4Query.bind( ":departure_inclination",                           departureInclination );
            SGP4Query.bind( ":departure_argument_of_periapsis",                 departureAOP );
            SGP4Query.bind( ":departure_longitude_of_ascending_node",           departureRAAN );              
            SGP4Query.bind( ":departure_true_anomaly",                          departureTA );
            
            SGP4Query.bind( ":arrival_position_x",                              0 );
            SGP4Query.bind( ":arrival_position_y",                              0 );
            SGP4Query.bind( ":arrival_position_z",                              0 );
            SGP4Query.bind( ":arrival_velocity_x",                              0 );
            SGP4Query.bind( ":arrival_velocity_y",                              0 );
            SGP4Query.bind( ":arrival_velocity_z",                              0 );
            SGP4Query.bind( ":arrival_position_x_error",                        0 );
            SGP4Query.bind( ":arrival_position_y_error",                        0 );
            SGP4Query.bind( ":arrival_position_z_error",                        0 );
            SGP4Query.bind( ":arrival_position_error",                          0 );
            SGP4Query.bind( ":arrival_velocity_x_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_y_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_z_error",                        0 );
            SGP4Query.bind( ":arrival_velocity_error",                          0 );
            SGP4Query.bind( ":success",                                         0 );

            // execute insert SGP4Query
            SGP4Query.executeStep( );

            // Reset SQL insert SGP4Query
            SGP4Query.reset( );
            
            ++arrivalEpochPropagationFailCounter;
            continue;
        }
                
        const Vector6 sgp4ArrivalState = getStateVector( sgp4ArrivalStateEci );

        //compute the required results
        double sgp4ArrivalPositionX = sgp4ArrivalState[ astro::xPositionIndex ];
        double sgp4ArrivalPositionY = sgp4ArrivalState[ astro::yPositionIndex ];
        double sgp4ArrivalPositionZ = sgp4ArrivalState[ astro::zPositionIndex ];

        double sgp4ArrivalVelocityX = sgp4ArrivalState[ astro::xVelocityIndex ];
        double sgp4ArrivalVelocityY = sgp4ArrivalState[ astro::yVelocityIndex ];
        double sgp4ArrivalVelocityZ = sgp4ArrivalState[ astro::zVelocityIndex ];

        double arrivalPositionErrorX = sgp4ArrivalPositionX - lambertArrivalPositionX;
        double arrivalPositionErrorY = sgp4ArrivalPositionY - lambertArrivalPositionY;
        double arrivalPositionErrorZ = sgp4ArrivalPositionZ - lambertArrivalPositionZ;
            
        std::vector< double > positionErrorVector( 3 );
        positionErrorVector[ 0 ] = arrivalPositionErrorX;
        positionErrorVector[ 1 ] = arrivalPositionErrorY;
        positionErrorVector[ 2 ] = arrivalPositionErrorZ;
        double arrivalPositionErrorMagnitude = sml::norm< double >( positionErrorVector );

        double arrivalVelocityErrorX = sgp4ArrivalVelocityX - ( lambertArrivalVelocityX - lambertArrivalDeltaVX );
        double arrivalVelocityErrorY = sgp4ArrivalVelocityY - ( lambertArrivalVelocityY - lambertArrivalDeltaVY );
        double arrivalVelocityErrorZ = sgp4ArrivalVelocityZ - ( lambertArrivalVelocityZ - lambertArrivalDeltaVZ );
            
        std::vector< double > velocityErrorVector( 3 );
        velocityErrorVector[ 0 ] = arrivalVelocityErrorX;
        velocityErrorVector[ 1 ] = arrivalVelocityErrorY;
        velocityErrorVector[ 2 ] = arrivalVelocityErrorZ;
        double arrivalVelocityErrorMagnitude = sml::norm< double >( velocityErrorVector );

        // Bind values to SQL insert sgp4Query
        SGP4Query.bind( ":lambert_transfer_id",                             lambertTransferId );
        SGP4Query.bind( ":departure_object_id",                             departureObjectId );
        SGP4Query.bind( ":arrival_object_id",                               arrivalObjectId );
        SGP4Query.bind( ":departure_epoch",                                 departureEpochJulian );

        SGP4Query.bind( ":departure_semi_major_axis",                       departureSMA );             
        SGP4Query.bind( ":departure_eccentricity",                          departureEccentricity );
        SGP4Query.bind( ":departure_inclination",                           departureInclination );
        SGP4Query.bind( ":departure_argument_of_periapsis",                 departureAOP );
        SGP4Query.bind( ":departure_longitude_of_ascending_node",           departureRAAN );              
        SGP4Query.bind( ":departure_true_anomaly",                          departureTA );
        
        SGP4Query.bind( ":arrival_position_x",                              sgp4ArrivalPositionX );
        SGP4Query.bind( ":arrival_position_y",                              sgp4ArrivalPositionY );
        SGP4Query.bind( ":arrival_position_z",                              sgp4ArrivalPositionZ );
        SGP4Query.bind( ":arrival_velocity_x",                              sgp4ArrivalVelocityX );
        SGP4Query.bind( ":arrival_velocity_y",                              sgp4ArrivalVelocityY );
        SGP4Query.bind( ":arrival_velocity_z",                              sgp4ArrivalVelocityZ );
        SGP4Query.bind( ":arrival_position_x_error",                        arrivalPositionErrorX );
        SGP4Query.bind( ":arrival_position_y_error",                        arrivalPositionErrorY );
        SGP4Query.bind( ":arrival_position_z_error",                        arrivalPositionErrorZ );
        SGP4Query.bind( ":arrival_position_error",                          arrivalPositionErrorMagnitude );
        SGP4Query.bind( ":arrival_velocity_x_error",                        arrivalVelocityErrorX );
        SGP4Query.bind( ":arrival_velocity_y_error",                        arrivalVelocityErrorY );
        SGP4Query.bind( ":arrival_velocity_z_error",                        arrivalVelocityErrorZ );
        SGP4Query.bind( ":arrival_velocity_error",                          arrivalVelocityErrorMagnitude );
        SGP4Query.bind( ":success",                                         1 );

        // execute insert SGP4Query
        SGP4Query.executeStep( );

        // Reset SQL insert SGP4Query
        SGP4Query.reset( );

        ++showProgress;
    }

    // Fetch number of rows in sgp4_scanner_results table.
    std::ostringstream sgp4ScannerTableSizeSelect;
    sgp4ScannerTableSizeSelect << "SELECT COUNT(*) FROM sgp4_scanner_results;";
    const int sgp4ScannertTableSize
        = database.execAndGet( sgp4ScannerTableSizeSelect.str( ) );

    std::ostringstream totalLambertCasesConsideredSelect;
    totalLambertCasesConsideredSelect << "SELECT COUNT(*) FROM lambert_scanner_results WHERE transfer_delta_v <= " << input.velocityCutOff << ";";
    const int totalLambertCasesConsidered = database.execAndGet( totalLambertCasesConsideredSelect.str( ) );

    std::cout << std::endl;
    std::cout << "Total lambert cases = " << lambertScannertTableSize << std::endl;
    std::cout << "Total sgp4 cases = " << sgp4ScannertTableSize << std::endl;
    std::cout << std::endl;
    std::cout << "Number of lambert cases considered with the velocity cut off = " << totalLambertCasesConsidered << std::endl;
    std::cout << "Number of virtual TLE convergence fail cases = " << virtualTleFailCounter << std::endl;
    std::cout << "Number of arrival epoch propagation fail cases = " << arrivalEpochPropagationFailCounter << std::endl;

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;
}

//! Check sgp4_scanner input parameters.
sgp4ScannerInput checkSGP4ScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const double velocityCutOff = find( config, "velocityCutOff" )->value.GetDouble( );
    std::cout << "Velocity Cut-Off              " << velocityCutOff << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                 " << shortlistPath << std::endl;
    }

    return sgp4ScannerInput( catalogPath,
                             velocityCutOff,
                             databasePath,
                             shortlistLength,
                             shortlistPath );
}

//! Create sgp4_scanner table.
void createSGP4ScannerTable( SQLite::Database& database )
{
    // Check that lambert_scanner_results table exists.
    if ( !database.tableExists( "lambert_scanner_results" ) )
    {
        throw std::runtime_error(
            "ERROR: \"lambert_scanner_results\" must exist and be populated!" );
    }

    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS sgp4_scanner_results;" );

    // Set up SQL command to create table to store SGP4 Scanner results.
    std::ostringstream sgp4ScannerTableCreate;
    sgp4ScannerTableCreate
        << "CREATE TABLE sgp4_scanner_results ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"                     INTEGER,"
        << "\"departure_object_id\"                     INTEGER,"
        << "\"arrival_object_id\"                       INTEGER,"
        << "\"departure_epoch\"                         REAL,"
        << "\"departure_semi_major_axis\"               REAL,"
        << "\"departure_eccentricity\"                  REAL,"
        << "\"departure_inclination\"                   REAL,"
        << "\"departure_argument_of_periapsis\"         REAL,"
        << "\"departure_longitude_of_ascending_node\"   REAL,"
        << "\"departure_true_anomaly\"                  REAL,"
        << "\"arrival_position_x\"                      REAL,"
        << "\"arrival_position_y\"                      REAL,"
        << "\"arrival_position_z\"                      REAL,"
        << "\"arrival_velocity_x\"                      REAL,"
        << "\"arrival_velocity_y\"                      REAL,"
        << "\"arrival_velocity_z\"                      REAL,"
        << "\"arrival_position_x_error\"                REAL,"
        << "\"arrival_position_y_error\"                REAL,"
        << "\"arrival_position_z_error\"                REAL,"
        << "\"arrival_position_error\"                  REAL,"
        << "\"arrival_velocity_x_error\"                REAL,"
        << "\"arrival_velocity_y_error\"                REAL,"
        << "\"arrival_velocity_z_error\"                REAL,"
        << "\"arrival_velocity_error\"                  REAL,"
        << "\"success\"                                 INTEGER"
        <<                                              ");";

    // Execute command to create table.
    database.exec( sgp4ScannerTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_position_error column.
    std::ostringstream arrivalPositionErrorIndexCreate;
    arrivalPositionErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_position_error\" on "
                                    << "sgp4_scanner_results (arrival_position_error ASC);";
    database.exec( arrivalPositionErrorIndexCreate.str( ).c_str( ) );

    // Execute command to create index on transfer arrival_velocity_error column.
    std::ostringstream arrivalVelocityErrorIndexCreate;
    arrivalVelocityErrorIndexCreate << "CREATE INDEX IF NOT EXISTS \"arrival_velocity_error\" on "
                                    << "sgp4_scanner_results (arrival_velocity_error ASC);";
    database.exec( arrivalVelocityErrorIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "sgp4_scanner_results" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'sgp4_scanner_results' failed! in routine SGP4Scanner.cpp" );
    }
}

} // namespace d2d
