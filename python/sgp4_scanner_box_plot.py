'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology
                                          (abhishek.agrawal@protonmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# I/O
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

# Get plotting packages
import matplotlib

# If user's computer does not have a GUI/display then the TKAgg will not be used
if config['display'] == 'True':
    matplotlib.use('TkAgg')
else:
    matplotlib.use('Agg')

import matplotlib.colors
import matplotlib.axes
import matplotlib.lines as mlines
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d

print ""
print "******************************************************************"
print "                            Operations                            "
print "******************************************************************"
print ""

print "Input data files being read ..."

output_path_prefix = config["output_directory"] + '/'

print "Fetching scan data from the databases ..."

# Connect to SQLite databases.
try:
    leo_database = sqlite3.connect( config['leo_database'] )
    heo_database = sqlite3.connect( config['heo_database'] )
    geo_database = sqlite3.connect( config['geo_database'] )

except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)


errorType = [ "arrival_position", "arrival_velocity" ]

# Fetch scan data.
leo_scan_data = pd.read_sql( "SELECT    arrival_position_error,                                   \
                                        arrival_velocity_error                                    \
                              FROM      sgp4_scanner_results                                      \
                              WHERE     success = 1;",                                            \
                              leo_database )

leo_scan_data.columns = [ 'positionErrorMagnitude',                                               \
                          'velocityErrorMagnitude' ]

heo_scan_data = pd.read_sql( "SELECT    arrival_position_error,                                   \
                                        arrival_velocity_error                                    \
                              FROM      sgp4_scanner_results                                      \
                              WHERE     success = 1;",                                            \
                              heo_database )

heo_scan_data.columns = [ 'positionErrorMagnitude',                                               \
                          'velocityErrorMagnitude' ]

geo_scan_data = pd.read_sql( "SELECT    arrival_position_error,                                   \
                                        arrival_velocity_error                                    \
                              FROM      sgp4_scanner_results                                      \
                              WHERE     success = 1;",                                            \
                              geo_database )

geo_scan_data.columns = [ 'positionErrorMagnitude',                                               \
                          'velocityErrorMagnitude' ]

print "Fetch successful!"
print ""

for errorTypeIndex in range( len( errorType ) ):
  if errorType[ errorTypeIndex ] == "arrival_position":
    yLabel = 'Position error [km]'
    figureNameLabel = 'position_error'
    annotation_xOffset = 0.18
    annotation_yOffset = 0
    print "Plotting position error box plot ..."
    print ""
    leo_magnitudeError = np.array( leo_scan_data[ 'positionErrorMagnitude' ], dtype=float )
    heo_magnitudeError = np.array( heo_scan_data[ 'positionErrorMagnitude' ], dtype=float )
    geo_magnitudeError = np.array( geo_scan_data[ 'positionErrorMagnitude' ], dtype=float )
    magnitudeError = []
    magnitudeError.append( leo_magnitudeError )
    magnitudeError.append( heo_magnitudeError )
    magnitudeError.append( geo_magnitudeError )
  else:
    yLabel = 'Velocity error [km/s]'
    figureNameLabel = 'velocity_error'
    annotation_xOffset = 0.12
    annotation_yOffset = 0.003
    print ""
    print "Plotting velocity error box plot ..."
    print ""
    leo_magnitudeError = np.array( leo_scan_data[ 'velocityErrorMagnitude' ], dtype=float )
    heo_magnitudeError = np.array( heo_scan_data[ 'velocityErrorMagnitude' ], dtype=float )
    geo_magnitudeError = np.array( geo_scan_data[ 'velocityErrorMagnitude' ], dtype=float )
    magnitudeError = []
    magnitudeError.append( leo_magnitudeError )
    magnitudeError.append( heo_magnitudeError )
    magnitudeError.append( geo_magnitudeError )

  fig = plt.figure( )
  axes = fig.gca( )

  plotOut = plt.boxplot( magnitudeError, sym='+' )
  # Use logarithmic scale for the Y axis
  plt.yscale( 'log' )

  plt.setp( plotOut['boxes'], color='black' )
  plt.setp( plotOut['whiskers'], color='black' )
  plt.setp( plotOut['medians'], color='black' )
  plt.setp( plotOut['fliers'], color='black' )
  plt.xticks( [1, 2, 3], ['LEO', 'GTO', 'GEO'] )
  plt.xlabel( 'Orbital regime' )
  plt.ylabel( yLabel )

  # # Setting labels for the medians
  # for line in plotOut['medians']:
  #   # Get position data for the median line (from matplotlib.lines)
  #   medianX, medianY = line.get_xydata( )[ 1 ]

  #   # Overlay median values
  #   axes.annotate( '%1.2f' % medianY,                                                             \
  #                  xy=( medianX + annotation_xOffset, medianY + annotation_yOffset ),             \
  #                  xycoords='data',                                                               \
  #                  horizontalalignment='center',                                                  \
  #                  verticalalignment='center' )

  # Save figure to file.
  plt.savefig( output_path_prefix + config['figure_name'] + ' ' + figureNameLabel
               + config["figure_format"], dpi=300 )
  plt.close( )

print "All figures generated successfully!"
print ""

# Close SQLite databases if they're still open.
if leo_database:
  leo_database.close( )

if heo_database:
  heo_database.close( )

if geo_database:
  geo_database.close( )

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""
