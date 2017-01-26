'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology (abhishek.agrawal@protonmail.com)
Copyright (c) 2014-2016 Enne Hekma, Delft University of Technology (ennehekma@gmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d
import matplotlib.mlab as mlab

# I/O
import commentjson
import json
from pprint import pprint

# SQL database
import sqlite3

# Numerical
import numpy as np
import pandas as pd
from scipy.stats import norm

# System
import sys
import time

print ""
print "---------------------------------------------------------------------------------"
print "                               D2D                                               "
print "                              0.0.2                                              "
print "         Copyright (c) 2016, K. Kumar (me@kartikkumar.com)                       "
print "         Copyright (c) 2016, A. Agrawal (abhishek.agrawal@protonmail.com)        "
print "         Copyright (c) 2016, E.J. Hekma (ennehekma@gmail.com)                    "
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
# scan_data = pd.read_sql( "SELECT transfer_delta_v FROM lambert_scanner_results WHERE transfer_delta_v < 30;",
#                               database )
scan_data = pd.read_sql( "SELECT    sgp4_scanner_results.arrival_position_error,        \
									sgp4_scanner_results.arrival_velocity_error,         \
									lambert_scanner_results.revolutions \
						  FROM          lambert_scanner_results                           \
						  INNER JOIN    sgp4_scanner_results                              \
						  ON            lambert_scanner_results.transfer_id =             \
										sgp4_scanner_results.lambert_transfer_id          \
					      AND           sgp4_scanner_results.success = 1;",
						  database )


scan_data.columns = [ 	'position_error',
						'velocity_error',
						'revolutions' ]

scan_0 = scan_data[scan_data['revolutions'] == 0]["position_error"]
scan_1 = scan_data[scan_data['revolutions'] == 1]["position_error"]
scan_2 = scan_data[scan_data['revolutions'] == 2]["position_error"]
scan_3 = scan_data[scan_data['revolutions'] == 3]["position_error"]
scan_4 = scan_data[scan_data['revolutions'] == 4]["position_error"]
scan_5 = scan_data[scan_data['revolutions'] == 5]["position_error"]

print config['set_axes_position_magnitude'][1]
plt.figure()
result = plt.hist([scan_0,scan_1,scan_2,scan_3,scan_4,scan_5], 50, stacked=True,  range=( 0,config['set_axes_position_magnitude'][1]))


# hatches = ['/', '+', '*', '\\', 'x', '.', '-', 'o']

# # print result
# plist1 = result[2][0]
# plist2 = result[2][1]
# for h, p1, p2 in zip(hatches, plist1, plist2):
#     p1.set_hatch(h)
# 	# print p1
#     p2.set_hatch(h)
# plt.title("GEO")



plt.legend(("0 revolutions","1 revolutions","2 revolutions","3 revolutions","4 revolutions","5 revolutions"),loc=1)
plt.ylabel( 'Frequency' )
plt.xlabel('Arrival position error [km]')

# Save figure to file.
plt.savefig(config["output_directory"] + "/" + config["histogram_figure"] +   "_position_stacked" + 	\
                       ".png", dpi=config["figure_dpi"])
plt.close()



scan_0 = scan_data[scan_data['revolutions'] == 0]["velocity_error"]
scan_1 = scan_data[scan_data['revolutions'] == 1]["velocity_error"]
scan_2 = scan_data[scan_data['revolutions'] == 2]["velocity_error"]
scan_3 = scan_data[scan_data['revolutions'] == 3]["velocity_error"]
scan_4 = scan_data[scan_data['revolutions'] == 4]["velocity_error"]
scan_5 = scan_data[scan_data['revolutions'] == 5]["velocity_error"]

plt.figure()
plt.hist([scan_0,scan_1,scan_2,scan_3,scan_4,scan_5], 50, stacked=True,  range=( 0,config['set_axes_velocity_magnitude'][1]))
# plt.title("GEO")

plt.legend(("0 revolutions","1 revolutions","2 revolutions","3 revolutions","4 revolutions","5 revolutions"),loc=1)
plt.ylabel( 'Frequency' )
plt.xlabel('Arrival velocity error [km/s]')

# Save figure to file.
plt.savefig(config["output_directory"] + "/" + config["histogram_figure"] +   "_velocity_stacked" + 	\
                       ".png", dpi=config["figure_dpi"])
plt.close()


print "Figure generated successfully!"
print ""

# Close SQLite database if it's still open.
if database:
    database.close()

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""
