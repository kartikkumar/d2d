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

print "Fetching scan data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect(config['database'])

except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)

# Function to calculate the transformation matrix for ECI to RTN frame conversion
# @param[in]  refX    X position of the RTN frame's origin w.r.t the ECI frame's origin
# @param[in]  refY    Y position of the RTN frame's origin w.r.t the ECI frame's origin
# @param[in]  refZ    Z position of the RTN frame's origin w.r.t the ECI frame's origin
# @param[in]  refVX   X velocity of the RTN frame's origin w.r.t the ECI frame's origin
# @param[in]  refVY   Y velocity of the RTN frame's origin w.r.t the ECI frame's origin
# @param[in]  refVZ   Z velocity of the RTN frame's origin w.r.t the ECI frame's origin
def gammaECI2RTN( refX, refY, refZ, refVX, refVY, refVZ ):

  # Radial unit vector:
  unitR = np.zeros( shape=( 1, 3 ), dtype=float )

  # Transverse unit vector:
  unitT = np.zeros( shape=( 1, 3 ), dtype=float )

  # Normal unit vector:
  unitN = np.zeros( shape=( 1, 3 ), dtype=float )

  vectorR = np.array( object=[ refX, refY, refZ ], dtype=float )

  vectorV = np.array( object=[ refVX, refVY, refVZ ], dtype=float )

  arrivalPositionMagnitude = np.linalg.norm( vectorR )

  if arrivalPositionMagnitude != 0:
    unitR = np.divide( vectorR, arrivalPositionMagnitude )

  angularMomentumVector = np.cross( vectorR, vectorV )
  angularMomentumMagnitude = np.linalg.norm( angularMomentumVector )

  if angularMomentumMagnitude != 0:
    unitN = np.divide( angularMomentumVector, angularMomentumMagnitude )

  unitT = np.cross( unitN, unitR )

  gamma = np.zeros( shape=( 3, 3 ), dtype=float )
  gamma[ 0 ][ : ] = unitR
  gamma[ 1 ][ : ] = unitT
  gamma[ 2 ][ : ] = unitN

  return gamma

# Sanity check for the gammaECI2RTN function:
# errorVectorECI = [ 10, 10, 0 ]
# errorVectorRTN = np.zeros( shape=( 3, 1 ), dtype=float )
# gamma = np.zeros( shape=( 3, 3 ), dtype=float )
# gamma = gammaECI2RTN( 0, 10, 0, -30, 0, 0 )
# errorVectorRTN[ 0 ] = np.inner( gamma[ 0 ][ : ], errorVectorECI )
# errorVectorRTN[ 1 ] = np.inner( gamma[ 1 ][ : ], errorVectorECI )
# errorVectorRTN[ 2 ] = np.inner( gamma[ 2 ][ : ], errorVectorECI )
# print "Error Vector RTN: "
# print errorVectorRTN
# expectedErrorVectorRTN = [ 10, -10, 0 ]
# print "Expected Error Vector RTN: "
# print expectedErrorVectorRTN
# exit( )

# Function to plot the components of position/velocity error vector
# @param[in]  errorX        x component
# @param[in]  errorY        y component
# @param[in]  errorZ        z component
# @param[in]  xcolor        x component color in the plot
# @param[in]  ycolor        y component color in the plot
# @param[in]  zcolor        z component color in the plot
# @param[in]  xlegend       legend for the x component curve in the plot
# @param[in]  ylegend       legend for the y component curve in the plot
# @param[in]  zlegend       legend for the z component curve in the plot
# @param[in]  xAxisLabel    Label for the X axis
# @param[in]  yAxisLabel    Label for the Y axis
# @param[in]  zAxisLabel    Label for the Z axis
# @param[in]  flag          Set True if position error vector is being plotted, else False
def plotComponents( errorX, errorY, errorZ,                                                       \
                    xcolor, ycolor, zcolor,                                                       \
                    xlegend, ylegend, zlegend,                                                    \
                    xAxisLabel, yAxisLabel, plotTitle,                                            \
                    flag ):

  n, bins, patches = plt.hist( errorX, bins=200, histtype='step', normed=False,                 \
                                 color=xcolor, alpha=1, label=xlegend, log=False )
  n, bins, patches = plt.hist( errorY, bins=200, histtype='step', normed=False,                 \
                                 color=ycolor, alpha=1, label=ylegend, log=False )
  n, bins, patches = plt.hist( errorZ, bins=200, histtype='step', normed=False,                 \
                                 color=zcolor, alpha=1, label=zlegend, log=False )

  # Figure properties
  plt.xlabel( xAxisLabel )
  plt.ylabel( yAxisLabel )

  if config[ 'add_title' ] == 'True':
      plt.title( plotTitle )

  if flag == True:
    xAxesLowerLimit = config['set_axes_position'][ 0 ]
    xAxesUpperLimit = config['set_axes_position'][ 1 ]
    yAxesLowerLimit = config['set_axes_position'][ 2 ]
    yAxesUpperLimit = config['set_axes_position'][ 3 ]
  else:
    xAxesLowerLimit = config['set_axes_velocity'][ 0 ]
    xAxesUpperLimit = config['set_axes_velocity'][ 1 ]
    yAxesLowerLimit = config['set_axes_velocity'][ 2 ]
    yAxesUpperLimit = config['set_axes_velocity'][ 3 ]

  if xAxesLowerLimit != 0                                                                         \
    or xAxesUpperLimit != 0                                                                       \
      or yAxesLowerLimit != 0                                                                     \
        or yAxesUpperLimit != 0:
          print "Using user defined axes limits"
          print ""
          plt.axis([xAxesLowerLimit,                                                              \
                    xAxesUpperLimit,                                                              \
                    yAxesLowerLimit,                                                              \
                    yAxesUpperLimit])

  xLegend = mlines.Line2D( [], [], color=xcolor, label=xlegend )
  yLegend = mlines.Line2D( [], [], color=ycolor, label=ylegend )
  zLegend = mlines.Line2D( [], [], color=zcolor, label=zlegend )
  lines = [xLegend, yLegend, zLegend]
  labels = [line.get_label( ) for line in lines]
  plt.legend(lines, labels)

  plt.grid( True )

# Function to plot the components of position/velocity error vector using line and markers
# @param[in]  errorX        x component
# @param[in]  errorY        y component
# @param[in]  errorZ        z component
# @param[in]  xcolor        x component color in the plot
# @param[in]  ycolor        y component color in the plot
# @param[in]  zcolor        z component color in the plot
# @param[in]  xlegend       legend for the x component curve in the plot
# @param[in]  ylegend       legend for the y component curve in the plot
# @param[in]  zlegend       legend for the z component curve in the plot
# @param[in]  xAxisLabel    Label for the X axis
# @param[in]  yAxisLabel    Label for the Y axis
# @param[in]  zAxisLabel    Label for the Z axis
# @param[in]  flag          Set True if position error vector is being plotted, else False
def plotComponentsMarkers( errorX, errorY, errorZ,                                                \
                           xcolor, ycolor, zcolor,                                                \
                           xlegend, ylegend, zlegend,                                             \
                           xAxisLabel, yAxisLabel, plotTitle,                                     \
                           flag ):

  xDataHist, xBinEdges = np.histogram( errorX, bins=50, normed=False )
  xBinCentre = ( xBinEdges[:-1] + xBinEdges[1:] ) / 2

  yDataHist, yBinEdges = np.histogram( errorY, bins=50, normed=False )
  yBinCentre = ( yBinEdges[:-1] + yBinEdges[1:] ) / 2

  zDataHist, zBinEdges = np.histogram( errorZ, bins=50, normed=False )
  zBinCentre = ( zBinEdges[:-1] + zBinEdges[1:] ) / 2

  xMarkerLine = mlines.Line2D( xBinCentre, xDataHist,                                             \
                               linestyle='solid', linewidth=3, color=xcolor, label=xlegend,       \
                               marker='s', markersize=6, markerfacecolor=xcolor )

  yMarkerLine = mlines.Line2D( yBinCentre, yDataHist,                                             \
                               linestyle='solid', linewidth=3, color=ycolor, label=ylegend,       \
                               marker='v', markersize=6, markerfacecolor=ycolor )

  zMarkerLine = mlines.Line2D( zBinCentre, zDataHist,                                             \
                               linestyle='solid', linewidth=3, color=zcolor, label=zlegend,       \
                               marker='*', markersize=6, markerfacecolor=zcolor )

  markerFigure = plt.figure( )
  ax = markerFigure.add_subplot( 1, 1, 1 )
  ax.add_line( xMarkerLine )
  ax.add_line( yMarkerLine )
  ax.add_line( zMarkerLine )

  # Figure properties
  plt.xlabel( xAxisLabel )
  plt.ylabel( yAxisLabel )

  if config[ 'add_title' ] == 'True':
      plt.title( plotTitle )

  if flag == True:
    xAxesLowerLimit = config['set_axes_position'][ 0 ]
    xAxesUpperLimit = config['set_axes_position'][ 1 ]
    yAxesLowerLimit = config['set_axes_position'][ 2 ]
    yAxesUpperLimit = config['set_axes_position'][ 3 ]
  else:
    xAxesLowerLimit = config['set_axes_velocity'][ 0 ]
    xAxesUpperLimit = config['set_axes_velocity'][ 1 ]
    yAxesLowerLimit = config['set_axes_velocity'][ 2 ]
    yAxesUpperLimit = config['set_axes_velocity'][ 3 ]

  if xAxesLowerLimit != 0                                                                         \
    or xAxesUpperLimit != 0                                                                       \
      or yAxesLowerLimit != 0                                                                     \
        or yAxesUpperLimit != 0:
          print "Using user defined axes limits"
          print ""
          ax.set_xlim( xAxesLowerLimit, xAxesUpperLimit )
          ax.set_ylim( yAxesLowerLimit, yAxesUpperLimit )

  xmin, xmax, ymin, ymax = ax.axis('auto')
  ax.set_xlim( xmin, xmax )
  ax.set_ylim( ymin, ymax )

  plt.legend( )
  plt.grid( True )


errorType = [ "arrival_position", "arrival_velocity" ]

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

if config['add_j2'] == "True":
  j2_analysis_data = pd.read_sql( "SELECT     arrival_position_x_error,                           \
                                              arrival_position_y_error,                           \
                                              arrival_position_z_error,                           \
                                              arrival_position_error,                             \
                                              arrival_velocity_x_error,                           \
                                              arrival_velocity_y_error,                           \
                                              arrival_velocity_z_error,                           \
                                              arrival_velocity_error                              \
                                  FROM        j2_analysis_results;",                              \
                                  database )

  j2_analysis_data.columns = [ 'positionErrorX',                                                  \
                               'positionErrorY',                                                  \
                               'positionErrorZ',                                                  \
                               'positionErrorMagnitude',                                          \
                               'velocityErrorX',                                                  \
                               'velocityErrorY',                                                  \
                               'velocityErrorZ',                                                  \
                               'velocityErrorMagnitude' ]


print "Fetch successful!"
print ""

for errorTypeIndex in range( len( errorType ) ):
  if errorType[ errorTypeIndex ] == "arrival_position":
    print "Plotting position error magnitude histogram ..."
    print ""
    magnitudeError = scan_data[ 'positionErrorMagnitude' ]
    if config['add_j2'] == "True":
      j2magnitudeError = j2_analysis_data[ 'positionErrorMagnitude' ]
  else:
    print ""
    print "Plotting velocity error magnitude histogram ..."
    print ""
    magnitudeError = scan_data[ 'velocityErrorMagnitude' ]
    if config['add_j2'] == "True":
      j2magnitudeError = j2_analysis_data[ 'velocityErrorMagnitude' ]

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
      j2CurveColor = 'red'
  else:
      figureColor = '0.40'
      j2CurveColor = '1'

  if config['add_j2'] == "True":
    n, bins, patches = plt.hist( magnitudeError, bins=50, normed=False, facecolor=figureColor,    \
                                 alpha=1, label='Magnitude' )
    n, bins, patches = plt.hist( j2magnitudeError, bins=50, histtype='step', normed=False,        \
                                 color=j2CurveColor, alpha=1, label='J2' )

    j2Legend = mlines.Line2D( [], [], color=j2CurveColor, label='J2' )
    magnitudeLegend = mpatches.Patch( color=figureColor, label='Magnitude' )
    lines = [ magnitudeLegend, j2Legend ]
    labels = [ line.get_label( ) for line in lines ]
    plt.legend( lines, labels )
  else:
    n, bins, patches = plt.hist( magnitudeError, bins=50, normed=False, facecolor=figureColor,    \
                                 alpha=1, label='Magnitude' )
    # plt.legend( )

  # Select appropriate unit and title for the error type
  if errorType[ errorTypeIndex ] == 'arrival_position':
      plotTitle  = 'Arrival Position Error'
      xAxisLabel = 'Arrival position error [km]'
  else:
      plotTitle  = 'Arrival Velocity Error'
      xAxisLabel = 'Arrival velocity error [km/s]'

  # Figure properties
  plt.xlabel( xAxisLabel )
  plt.ylabel( 'Frequency' )

  if config[ 'add_title' ] == 'True':
      plt.title( plotTitle + " " + 'Magnitude' )

  plt.grid( True )

  # Save figure to file.
  if config['add_j2'] == "True":
    plt.savefig( output_path_prefix + "J2_" + config["histogram_figure"] + "_"
                 + errorType[ errorTypeIndex ] + "_error" + "_magnitude"
                 + config["figure_format"], dpi=config["figure_dpi"] )
    plt.close( )
  else:
    plt.savefig( output_path_prefix + config["histogram_figure"] + "_" + errorType[ errorTypeIndex ]
                 + "_error" + "_magnitude" + config["figure_format"], dpi=config["figure_dpi"] )
    plt.close( )

# Plot the components of the (position/velocity) error vector in a separate figure.
if config['grayscale'] == 'False':
    xcolor = 'red'
    ycolor = 'blue'
    zcolor = 'black'
else:
    xcolor = '0'
    ycolor = '0.30'
    zcolor = '0.60'

positionErrorX = scan_data[ 'positionErrorX' ]
positionErrorY = scan_data[ 'positionErrorY' ]
positionErrorZ = scan_data[ 'positionErrorZ' ]
velocityErrorX = scan_data[ 'velocityErrorX' ]
velocityErrorY = scan_data[ 'velocityErrorY' ]
velocityErrorZ = scan_data[ 'velocityErrorZ' ]

if config['frame'] == "RTN":
  # ECI to RTN frame transformation.
  print ""
  print "Performing ECI to RTN frame conversion for the position and velocity error vectors ..."
  print ""

  arrivalPositionX = lambert_scan_data[ 'arrivalPositionX' ]
  arrivalPositionY = lambert_scan_data[ 'arrivalPositionY' ]
  arrivalPositionZ = lambert_scan_data[ 'arrivalPositionZ' ]
  arrivalVelocityX = lambert_scan_data[ 'arrivalVelocityX' ]
  arrivalVelocityY = lambert_scan_data[ 'arrivalVelocityY' ]
  arrivalVelocityZ = lambert_scan_data[ 'arrivalVelocityZ' ]

  totalCases = len( arrivalPositionX )
  gamma = np.zeros( shape=( 3, 3 ), dtype=float )

  for i in tqdm( range( totalCases ) ):
    gamma = gammaECI2RTN( arrivalPositionX[ i ],                                                  \
                          arrivalPositionY[ i ],                                                  \
                          arrivalPositionZ[ i ],                                                  \
                          arrivalVelocityX[ i ],                                                  \
                          arrivalVelocityY[ i ],                                                  \
                          arrivalVelocityZ[ i ] )

    positionErrorVectorECI = [ positionErrorX[ i ], positionErrorY[ i ], positionErrorZ[ i ] ]
    velocityErrorVectorECI = [ velocityErrorX[ i ], velocityErrorY[ i ], velocityErrorZ[ i ] ]

    positionErrorX[ i ] = np.inner( gamma[ 0 ][ : ], positionErrorVectorECI )
    positionErrorY[ i ] = np.inner( gamma[ 1 ][ : ], positionErrorVectorECI )
    positionErrorZ[ i ] = np.inner( gamma[ 2 ][ : ], positionErrorVectorECI )

    velocityErrorX[ i ] = np.inner( gamma[ 0 ][ : ], velocityErrorVectorECI )
    velocityErrorY[ i ] = np.inner( gamma[ 1 ][ : ], velocityErrorVectorECI )
    velocityErrorZ[ i ] = np.inner( gamma[ 2 ][ : ], velocityErrorVectorECI )

  print "Plotting position error components defined in RTN frame"

  if config['component_marker'] == "False":
    plotComponents( positionErrorX, positionErrorY, positionErrorZ,                               \
                    xcolor, ycolor, zcolor,                                                       \
                    "Radial", "Transverse", "Normal",                                             \
                    "Arrival position error [km]", "Frequency", "Position Error Components",      \
                    True )
  else:
    plotComponentsMarkers( positionErrorX, positionErrorY, positionErrorZ,                        \
                           xcolor, ycolor, zcolor,                                                \
                           "Radial", "Transverse", "Normal",                                      \
                           "Arrival position error [km]", "Frequency",
                           "Position Error Components", True )

  # Save figure to file.
  plt.savefig( output_path_prefix + config["histogram_figure"] + "_arrival_position_error"
               + "_components_" + config["frame"] + config["figure_format"],                      \
               dpi=config["figure_dpi"] )
  plt.close( )

  print "Plotting velocity error components defined in RTN frame"

  if config['component_marker'] == "False":
    plotComponents( velocityErrorX, velocityErrorY, velocityErrorZ,                               \
                    xcolor, ycolor, zcolor,                                                       \
                    "Radial", "Transverse", "Normal",                                             \
                    "Arrival velocity error [km/s]", "Frequency", "Velocity Error Components",    \
                    False )
  else:
    plotComponentsMarkers( velocityErrorX, velocityErrorY, velocityErrorZ,                        \
                           xcolor, ycolor, zcolor,                                                \
                           "Radial", "Transverse", "Normal",                                      \
                           "Arrival velocity error [km/s]", "Frequency",
                           "Velocity Error Components", False )

  # Save figure to file.
  plt.savefig( output_path_prefix + config["histogram_figure"] + "_arrival_velocity_error"
               + "_components_" + config["frame"] + config["figure_format"],                      \
               dpi=config["figure_dpi"] )
  plt.close( )

else:
  print "Plotting position error components defined in ECI frame"

  if config['component_marker'] == "False":
    plotComponents( positionErrorX, positionErrorY, positionErrorZ,                               \
                    xcolor, ycolor, zcolor,                                                       \
                    "X Axis", "Y Axis", "Z Axis",                                                 \
                    "Arrival position error [km]", "Frequency", "Position Error Components",      \
                    True )
  else:
    plotComponentsMarkers( positionErrorX, positionErrorY, positionErrorZ,                        \
                           xcolor, ycolor, zcolor,                                                \
                           "X Axis", "Y Axis", "Z Axis",                                          \
                           "Arrival position error [km]", "Frequency",
                           "Position Error Components", True )

  # Save figure to file.
  plt.savefig( output_path_prefix + config["histogram_figure"] + "_arrival_position_error"
               + "_components_" + config["frame"] + config["figure_format"],                      \
               dpi=config["figure_dpi"] )
  plt.close( )

  print "Plotting velocity error components defined in ECI frame"

  if config['component_marker'] == "False":
    plotComponents( velocityErrorX, velocityErrorY, velocityErrorZ,                               \
                    xcolor, ycolor, zcolor,                                                       \
                    "X Axis", "Y Axis", "Z Axis",                                                 \
                    "Arrival velocity error [km/s]", "Frequency", "Velocity Error Components",    \
                    False )
  else:
    plotComponentsMarkers( velocityErrorX, velocityErrorY, velocityErrorZ,                        \
                           xcolor, ycolor, zcolor,                                                \
                           "X Axis", "Y Axis", "Z Axis",                                          \
                           "Arrival velocity error [km/s]", "Frequency",
                           "Velocity Error Components", False )

  # Save figure to file.
  plt.savefig( output_path_prefix + config["histogram_figure"] + "_arrival_velocity_error"
               + "_components_" + config["frame"] + config["figure_format"],                      \
               dpi=config["figure_dpi"] )
  plt.close( )

print "All figures generated successfully!"
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
