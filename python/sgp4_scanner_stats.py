'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology
                                          (abhishek.agrawal@protonmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# I/O
import csv
import commentjson
import json
from pprint import pprint

# SQL database
import sqlite3

# Numerical
import numpy as np
import pandas as pd

# System
import sys
import time
from os import path
from tqdm import tqdm

print ""
print "---------------------------------------------------------------------------------"
print "                                 D2D                                             "
print "                                                                                 "
print "         Copyright (c) 2016, K. Kumar (me@kartikkumar.com)                       "
print "         Copyright (c) 2016, A. Agrawal (abhishek.agrawal@protonmail.com)        "
print "---------------------------------------------------------------------------------"
print ""

# Start timer.
start_time = time.time( )

print ""
print "******************************************************************"
print "                          Input parameters                        "
print "******************************************************************"
print ""

# Parse JSON configuration file
# Raise exception if wrong number of inputs are provided to script
if len(sys.argv) != 2:
    raise Exception("Only provide a JSON config file as input!")

json_data = open(sys.argv[1])
config = commentjson.load(json_data)
json_data.close()
pprint(config)

print ""
print "******************************************************************"
print "                            Operations                            "
print "******************************************************************"
print ""

print "Fetching scan data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect(config['database'])

except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)

# Fetch scan data.
scan_data = pd.read_sql( "SELECT    arrival_position_x_error,                                     \
                                    arrival_position_y_error,                                     \
                                    arrival_position_z_error,                                     \
                                    arrival_position_error,                                       \
                                    arrival_velocity_x_error,                                     \
                                    arrival_velocity_y_error,                                     \
                                    arrival_velocity_z_error,                                     \
                                    arrival_velocity_error                                        \
                          FROM      sgp4_scanner_results                                          \
                          WHERE     success = 1;",                                                \
                          database )

scan_data.columns = [ 'positionErrorX',                                                           \
                      'positionErrorY',                                                           \
                      'positionErrorZ',                                                           \
                      'positionErrorMagnitude',                                                   \
                      'velocityErrorX',                                                           \
                      'velocityErrorY',                                                           \
                      'velocityErrorZ',                                                           \
                      'velocityErrorMagnitude' ]

print "Fetch successful!"
print ""

magnitudeTypes = [ 'positionErrorMagnitude', 'velocityErrorMagnitude' ]
errorTypes = [ 'position error', 'velocity error' ]
for loopIndex in range( 2 ):
  errorType = errorTypes[ loopIndex ]
  print ""
  print "Stats for " + errorType + ":"
  magnitudeError = scan_data[ magnitudeTypes[ loopIndex ] ]

  # Calculate mean, variance and standard deviation
  mu = sum( magnitudeError ) / len( magnitudeError )
  print 'Mean = ' + repr( mu )

  sumOfSquareDeviations = sum( ( x - mu )**2 for x in magnitudeError )
  variance = sumOfSquareDeviations / len( magnitudeError )
  print 'Variance = ' + repr( variance )

  sigma = variance**0.5
  print 'Standard Deviation = ' + repr( sigma )

  # Calculate median
  median = np.median( magnitudeError )
  print 'median = ' + repr( median )

  # Get maximum and minimum error values
  maxError = max( magnitudeError )
  print 'max error = ' + repr( maxError )
  minError = min( magnitudeError )
  print 'min error = ' + repr( minError )

  # Get interquartile range
  q75, q25 = np.percentile( magnitudeError, [ 75, 25 ] )
  interquartileRange = q75 - q25
  print 'IQR = ' + repr( interquartileRange )

  # Print data to a csv file
  orbitType = config['orbit_type']
  file_path = path.relpath( config['output'] )
  with open( file_path, 'ab' ) as csvfile:
    fieldnames = [ 'Orbit Type', 'Error Type', 'Mean', 'Variance', 'Standard Deviation',          \
                   'median', 'Max. Error', 'Min. Error', 'Interquartile Range' ]
    dataWriter = csv.DictWriter( csvfile, fieldnames=fieldnames, extrasaction='raise' )

    dataWriter.writeheader()
    dataWriter.writerow( { 'Orbit Type'           : orbitType,                                    \
                           'Error Type'           : errorType,                                    \
                           'Mean'                 : mu,                                           \
                           'Variance'             : variance,                                     \
                           'Standard Deviation'   : sigma,                                        \
                           'median'               : median,                                       \
                           'Max. Error'           : maxError,                                     \
                           'Min. Error'           : minError,                                     \
                           'Interquartile Range'  : interquartileRange } )

# Close SQLite database if it's still open.
if database:
    database.close( )

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""
