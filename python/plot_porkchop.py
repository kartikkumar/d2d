'''
Copyright (c) 2014-2016, Kartik Kumar (me@kartikkumar.com)
Copyright (c) 2016, Enne Hekma (ennehekma@gmail.com)
All rights reserved.
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib import gridspec
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d
from nlcmap import nlcmap
from matplotlib import rcParams
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
import math

# I/O
import commentjson
import json
from pprint import pprint
import sqlite3

# Numerical
import numpy as np
import pandas as pd

# System
import sys
import time

print ""
print "------------------------------------------------------------------"
print "                               D2D                                "
print "                              0.0.2                               "
print "      Copyright (c) 2015-2016, K. Kumar (me@kartikkumar.com)      "
print "      Copyright (c) 2016, E.J. Hekma (ennehekma@gmail.com)        "
print "------------------------------------------------------------------"
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

print "Fetching porkchop plot data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect(config['database'])

except sqlite3.Error, e:

    print "Error %s:" % e.args[0]
    sys.exit(1)

if config['objects']==[]:
	best = pd.read_sql("SELECT DISTINCT(departure_object_id), arrival_object_id, time_of_flight,  \
										departure_epoch, transfer_delta_v  						  \
	                    FROM lambert_scanner_results 											  \
	 					ORDER BY transfer_delta_v ASC 											  \
	 					LIMIT 10 ",																  \
	                    database )

	a = (best['departure_object_id'][0])
	b = (best['arrival_object_id'][0])
else:
	a = config['objects'][0]
	b = config['objects'][1]

print "Porkchop plot figure being generated for transfer between TLE objects", a, "and", b, "..."

times_of_flight = pd.read_sql_query("	SELECT DISTINCT time_of_flight 							  \
										FROM lambert_scanner_results",							  \
										database)
departure_epochs = pd.read_sql_query("	SELECT DISTINCT departure_epoch 						  \
										FROM lambert_scanner_results",							  \
										database)-2400000.5 # change to Modified Julian Date
transfer_delta_vs = pd.read_sql_query("	SELECT transfer_delta_v 								  \
										FROM lambert_scanner_results 							  \
										WHERE departure_object_id =" + str(a) + "				  \
										and arrival_object_id =" + str(b),						  \
										database)
first_departure_epoch = departure_epochs['departure_epoch'][0]
departure_epochs = departure_epochs - first_departure_epoch

times_of_flight = times_of_flight / 86400

z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight)) 
x1, y1 = np.meshgrid(times_of_flight,departure_epochs)
datapoints = np.size(z)
failures = np.count_nonzero(np.isnan(z))

# Add cutoff dV if user input requires to do so
if config['cutoff'] != 0:
	cutoff = config['cutoff']
	for i in xrange(0,len(departure_epochs)):
		for j in xrange(0,len(times_of_flight)):
			if math.isnan(z[i][j]) == True:
				z[i][j] = cutoff+1
				# print z[i][j]
			elif z[i][j] > cutoff:
				z[i][j] =  cutoff

# Plot porkchop plot
cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)
data = ax1.contourf(y1,x1,z,cmap=cmap)
cbar = plt.colorbar(data, cmap=cmap)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [days] \n Initial departure epoch = ' + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('T$_{ToF}$ [days]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
# if config['cutoff'] == 0:
#  	plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) + 					  \
#  			  ".\n Number of datapoints: " + str(datapoints) +" (" + str(len(departure_epochs)) + \
#  			  "x" + str(len(times_of_flight)) + "). Total failures: " + str(failures),		  	  \
#  			  fontsize=10, y=1.02)
# else:
# 	plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) +						  \
# 			  "\n with a cutoff $\Delta V$ of " + str(config['cutoff']) + "\n Number of 		  \
# 			  datapoints: " + str(datapoints) + " (" + str(len(departure_epochs)) + "x" + 	      \
# 			  str(len(times_of_flight)) + ") " + "Total failures: " + str(failures), 			  \
# 			  fontsize=10, y=1.02)

plt.tight_layout()

plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + ".png", 					  \
            dpi=config["figure_dpi"])

print "Figure generated successfully!"
print ""

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""