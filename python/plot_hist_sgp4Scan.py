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

print "Fetching scan data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect(config['database'])

except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)

# Fetch scan data.
scan_data = pd.read_sql( "SELECT    " + config['error'] + "_x_error,                              \
                                    " + config['error'] + "_y_error,                              \
                                    " + config['error'] + "_z_error,                              \
                                    " + config['error'] + "_error                                 \
                          FROM      sgp4_scanner_results                                          \
                          WHERE     success = 1;",                                                \
                          database )

scan_data.columns = [ 'xerror', 'yerror', 'zerror', 'magnitudeError' ]

lambert_scan_data = pd.read_sql( "SELECT        lambert_scanner_results.arrival_position_x,       \
                                                lambert_scanner_results.arrival_position_y,       \
                                                lambert_scanner_results.arrival_position_z,       \
                                                lambert_scanner_results.arrival_velocity_x,       \
                                                lambert_scanner_results.arrival_velocity_y,       \
                                                lambert_scanner_results.arrival_velocity_z        \
                                  FROM          lambert_scanner_results                           \
                                  INNER JOIN    sgp4_scanner_results                              \
                                  ON            lambert_scanner_results.transfer_id =             \
                                                sgp4_scanner_results.lambert_transfer_id          \
                                  AND           sgp4_scanner_results.success = 1;",
                                  database )

lambert_scan_data.columns = [ 'arrivalPositionX',                                                 \
                              'arrivalPositionY',                                                 \
                              'arrivalPositionZ',                                                 \
                              'arrivalVelocityX',                                                 \
                              'arrivalVelocityY',                                                 \
                              'arrivalVelocityZ', ]

print "Fetch successful!"
print ""

# The histogram of the data
xerror = scan_data[ 'xerror' ]
yerror = scan_data[ 'yerror' ]
zerror = scan_data[ 'zerror' ]
magnitudeError = scan_data[ 'magnitudeError' ]

# Calculate mean, variance and standard deviation
mu = sum( magnitudeError ) / len( magnitudeError )
print 'Mean = ' + repr( mu )

sumOfSquareDeviations = sum( ( x - mu )**2 for x in magnitudeError )
variance = sumOfSquareDeviations / len( magnitudeError )
print 'Variance = ' + repr( variance )

sigma = variance**0.5
print 'Standard Deviation = ' + repr( sigma )

# Plot the magnitude of the error
if config['grayscale'] == 'False':
    figureColor = 'green'
else:
    figureColor = '0.40'

n, bins, patches = plt.hist( magnitudeError, bins=50, normed=False, facecolor=figureColor,        \
                             alpha=1, label='Magnitude' )

# Add a line of expected distribution
# y = mlab.normpdf( bins, mu, sigma )
# l = plt.plot( bins, y, 'r--', linewidth=1.5, label='Best fit line' )

# Select appropriate unit and title for the error type
if config['error'] == 'arrival_position':
    errorUnit = "[km]"
    plotTitle = 'Arrival Position Error'
else:
    errorUnit = "[km/s]"
    plotTitle = 'Arrival Velocity Error'

# Figure properties
plt.xlabel( 'Error' + " " + errorUnit )
plt.ylabel( 'Frequency' )

if config[ 'add_title' ] == 'True':
    plt.title( plotTitle + " " + 'Magnitude' )

plt.legend( )
plt.grid( True )

# Save figure to file.
plt.savefig( output_path_prefix + config["histogram_figure"] + "_" + config['error'] + "_error"
             + "_magnitude" + config["figure_format"], dpi=config["figure_dpi"] )
plt.close( )

# Plot the components of the (position/velocity) error vector in a separate figure.
if config['grayscale'] == 'False':
    xcolor = 'black'
    ycolor = 'green'
    zcolor = 'red'
else:
    xcolor = '0'
    ycolor = '0.30'
    zcolor = '0.60'

# ECI to RTN frame transformation.
print ""
print "Performing ECI to RTN frame conversion..."
print ""

def ECI2RTN( eciX, eciY, eciZ, refX, refY, refZ, refVX, refVY, refVZ ):
  "This convertes a state in ECI frame to RTN frame"

  # Radial unit vector:
  unitR = np.zeros( shape=( 1, 3 ), dtype=float )

  # Transverse unit vector:
  unitT = np.zeros( shape=( 1, 3 ), dtype=float )

  # Normal unit vector:
  unitN = np.zeros( shape=( 1, 3 ), dtype=float )


  vectorR = np.array( object=[ refX, refY, refZ ], dtype=float )
  print "vectorR = ", vectorR

  vectorV = np.array( object=[ refVX, refVY, refVZ ], dtype=float )
  print "vectorV = ", vectorV

  arrivalPositionMagnitude = np.linalg.norm( vectorR )

  if arrivalPositionMagnitude != 0:
    unitR = np.divide( vectorR, arrivalPositionMagnitude )
  print "unit R = ", unitR

  angularMomentumVector = np.cross( vectorR, vectorV )
  angularMomentumMagnitude = np.linalg.norm( angularMomentumVector )

  if angularMomentumMagnitude != 0:
    unitN = np.divide( angularMomentumVector, angularMomentumMagnitude )
  print "unit N = ", unitN

  unitT = np.cross( unitN, unitR )
  print "unit T = ", unitT

  # Position expressed in ECI frame
  vectorECI = np.zeros( shape=( 1, 3 ), dtype=float )
  vectorECI[ 0 ][ 0 ] = eciX
  vectorECI[ 0 ][ 1 ] = eciY
  vectorECI[ 0 ][ 2 ] = eciZ
  print "ECI = ", vectorECI

  # Position expressed in RTN frame
  vectorRTN = np.zeros( shape=( 3, 1 ), dtype=float )
  vectorRTN[ 0 ] = np.inner( unitR, vectorECI )
  vectorRTN[ 1 ] = np.inner( unitT, vectorECI )
  vectorRTN[ 2 ] = np.inner( unitN, vectorECI )
  print "RTN: "
  print vectorRTN

  return vectorRTN

# Check for ECI to RTN converter
# Case 1 from http://www.centerforspace.com/downloads/files/pubs/AAS-03-526.pdf
eciX = -605.79221660e3
eciY = -5870.22951108e3
eciZ = 3493.05319896e3
eciVX = -1.56825429e3
eciVY = -3.70234891e3
eciVZ = -6.47948395e3
testVectorRTN = [ 0 for x in range( 0, 3 ) ]
testVectorRTN = ECI2RTN( eciX, eciY, eciZ,                                                        \
                         eciX + 6857.693605, eciY + 0.0, eciZ + 0.0,                              \
                         eciVX + 0.007362813, eciVY + 7.62564531, eciVZ + 0.0 )

# Case 2. circular equitorial orbit
eciX = 10.0e3
eciY = 0.0
eciZ = 0.0
testVectorRTN = ECI2RTN( eciX, eciY, eciZ, 0.0, 10.0e3, 0.0, -10.0, 0.0, 0.0 )
exit( )

arrivalPositionX = lambert_scan_data[ 'arrivalPositionX' ]
arrivalPositionY = lambert_scan_data[ 'arrivalPositionY' ]
arrivalPositionZ = lambert_scan_data[ 'arrivalPositionZ' ]
arrivalVelocityX = lambert_scan_data[ 'arrivalVelocityX' ]
arrivalVelocityY = lambert_scan_data[ 'arrivalVelocityY' ]
arrivalVelocityZ = lambert_scan_data[ 'arrivalVelocityZ' ]

totalCases = len( xerror )
for i in tqdm( range( totalCases ) ):
  vectorR = [ arrivalPositionX[ i ], arrivalPositionY[ i ], arrivalPositionZ[ i ] ]
  vectorV = [ arrivalVelocityX[ i ], arrivalVelocityY[ i ], arrivalVelocityZ[ i ] ]
  arrivalPositionMagnitude = np.linalg.norm( vectorR )
  unitR = vectorR / arrivalPositionMagnitude

  angularMomentumVector = np.cross( vectorR, vectorV )
  angularMomentumMagnitude = np.linalg.norm( angularMomentumVector )
  unitN = angularMomentumVector / angularMomentumMagnitude

  unitT = np.cross( unitN, unitR )

  gamma[ 0 ][ : ] = unitR
  gamma[ 1 ][ : ] = unitT
  gamma[ 2 ][ : ] = unitN

  errorVectorECI = [ xerror[ i ], yerror[ i ], zerror[ i ] ]
  errorVectorRTN = [ 0 for x in range( 0, 3 ) ]
  errorVectorRTN[ 0 ] = np.inner( gamma[ 0 ][ : ], errorVectorECI )
  errorVectorRTN[ 1 ] = np.inner( gamma[ 1 ][ : ], errorVectorECI )
  errorVectorRTN[ 2 ] = np.inner( gamma[ 2 ][ : ], errorVectorECI )

  xerror[ i ] = errorVectorRTN[ 0 ]
  yerror[ i ] = errorVectorRTN[ 1 ]
  zerror[ i ] = errorVectorRTN[ 2 ]

print "ECI to RTN conversion completed!"
print ""

n, bins, patches = plt.hist( xerror, bins=200, histtype='step', normed=False, color=xcolor,       \
                             alpha=1, label='R Axis', log=False )
n, bins, patches = plt.hist( yerror, bins=200, histtype='step', normed=False, color=ycolor,       \
                             alpha=1, label='T Axis', log=False )
n, bins, patches = plt.hist( zerror, bins=200, histtype='step', normed=False, color=zcolor,       \
                             alpha=1, label='N Axis', log=False )
# Figure properties
plt.xlabel( 'Error' + " " + errorUnit )
plt.ylabel( 'Frequency' )

if config[ 'add_title' ] == 'True':
    plt.title( plotTitle + " " + 'Components' )

xAxesLowerLimit = config['set_axes'][0]
xAxesUpperLimit = config['set_axes'][1]
yAxesLowerLimit = config['set_axes'][2]
yAxesUpperLimit = config['set_axes'][3]

if xAxesLowerLimit != 0                                                                           \
  or xAxesUpperLimit != 0                                                                         \
    or yAxesLowerLimit != 0                                                                       \
      or yAxesUpperLimit != 0:
        print "Using user defined axes limits"
        print ""
        plt.axis([xAxesLowerLimit,                                                                \
                  xAxesUpperLimit,                                                                \
                  yAxesLowerLimit,                                                                \
                  yAxesUpperLimit])

xlegend = mlines.Line2D( [], [], color=xcolor, label='R Axis' )
ylegend = mlines.Line2D( [], [], color=ycolor, label='T Axis' )
zlegend = mlines.Line2D( [], [], color=zcolor, label='N Axis' )
lines = [xlegend, ylegend, zlegend]
labels = [line.get_label( ) for line in lines]
plt.legend(lines, labels)

plt.grid( True )

# Save figure to file.
plt.savefig( output_path_prefix + config["histogram_figure"] + "_" + config['error'] + "_error"
             + "_components" + config["figure_format"], dpi=config["figure_dpi"] )
plt.close( )

print "Figures generated successfully!"
print ""

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
