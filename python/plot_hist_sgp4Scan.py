'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology
                                          (abhishek.agrawal@protonmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d

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
scan_data = pd.read_sql( "SELECT    " + config['error'] + "_x_error, \
                                    " + config['error'] + "_y_error, \
                                    " + config['error'] + "_z_error, \
                                    " + config['error'] + "_error    \
                          FROM      sgp4_scanner_results             \
                          WHERE     success = 1;",
                          database )

scan_data.columns = [ 'xerror', 'yerror', 'zerror', 'magnitudeError' ]

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
n, bins, patches = plt.hist( magnitudeError, bins=100, normed=True, facecolor='green', alpha=0.5,
                             label='Magnitude' )

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
plt.ylabel( 'Normed Frequency' )
plt.title( plotTitle + " " + 'Magnitude' )
# plt.axis([40, 160, 0, 0.03])
plt.legend( )
plt.grid( True )

# Save figure to file.
plt.savefig( output_path_prefix + config["histogram_figure"] + "_" + config['error'] + "_error"
             + "_magnitude" + ".png", dpi=config["figure_dpi"] )
plt.close( )

# Plot the components of the error in a separate figure.
n, bins, patches = plt.hist( xerror, bins=200, histtype='step', normed=False, facecolor='red',
                             alpha=1, label='X Axis' )
n, bins, patches = plt.hist( yerror, bins=200, histtype='step', normed=False, facecolor='blue',
                             alpha=1, label='Y Axis' )
n, bins, patches = plt.hist( zerror, bins=200, histtype='step', normed=False, facecolor='orange',
                             alpha=1, label='Z Axis' )
# Figure properties
plt.xlabel( 'Error' + " " + errorUnit )
plt.ylabel( 'Frequency' )
plt.title( plotTitle + " " + 'Components' )
# plt.axis([40, 160, 0, 0.03])
plt.legend( )
plt.grid( True )

# Save figure to file.
plt.savefig( output_path_prefix + config["histogram_figure"] + "_" + config['error'] + "_error"
             + "_components" + ".png", dpi=config["figure_dpi"] )
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
