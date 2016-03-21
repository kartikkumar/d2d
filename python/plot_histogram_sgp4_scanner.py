'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology (abhishek.agrawal@protonmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d

# I/O
import commentjson
import json
from pprint import pprint

# Numerical
import numpy as np
import pandas as pd

# System
import sys
import time

print ""
print "---------------------------------------------------------------------------------"
print "                               D2D                                               "
print "                              0.0.2                                              "
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


departure_epochs = pd.read_sql("SELECT DISTINCT departure_epoch                                                                         \
                                    FROM sgp4_scanner_results;",                                                                        \
                                database)

for i in xrange(0,departure_epochs.size):
# for i in xrange(0,1):
    c = departure_epochs['departure_epoch'][i]
    print "Plotting histogram with departure epoch: ",c,"Julian Date"
                      
    # Fetch scan data.
    scan_data = pd.read_sql("SELECT transfer_id, lambert_transfer_id, departure_object_id,                                              \
                                    arrival_object_id,                                                                                  \
                                    " + config['minmax'] + " (" + config['error'] + ")                                                  \
                              FROM sgp4_scanner_results                                                                                 \
                              WHERE departure_epoch BETWEEN " + str(c - 0.00001) + "                                                    \
                                                        AND " + str(c + 0.00001) + "                                                    \
                              GROUP BY departure_object_id, arrival_object_id;",                                                        \
                            database)
    scan_data.columns = ['transfer_id', 'lambert_transfer_id', 'departure_object_id','arrival_object_id',                               \
                         config['error']]
    # scan_order = scan_data.sort_values(str(map_order))                                                                                  \
    #                       .drop_duplicates('departure_object_id')[                                                                      \
    #                           ['departure_object_id',str(map_order)]]

    scan_map = scan_data.pivot(index='arrival_object_id',                                                                               \
                               columns='departure_object_id',                                       
                               values=config['error'])
    scan_map = scan_map.reindex(index=scan_order['departure_object_id'],                                                                \
                                columns=scan_order['departure_object_id'])
    print scan_map
    print scan_data
    # Set up color map.
    bins = np.linspace(scan_data[config['error']].min(),                                                                                \
                       scan_data[config['error']].max(), 10)
    groups = scan_data[config['error']].groupby(                                                                                        \
                np.digitize(scan_data[config['error']], bins))
    levels = groups.mean().values
    cmap_lin = plt.get_cmap(config['colormap'])
    cmap = nlcmap(cmap_lin, levels)
    # cmap.set_bad(color='white', alpha=1.)    
    # Plot heat map.
    ax1 = plt.subplot2grid((15,15), (2, 0),rowspan=13,colspan=14)
    heatmap = ax1.pcolormesh(scan_map.values, cmap=cmap,                                                                                \
                             vmin=scan_data[config['error']].min(),                                                                     \
                             vmax=scan_data[config['error']].max())
    ax1.set_xticks(np.arange(scan_map.shape[1] + 1)+0.5)
    ax1.set_xticklabels(scan_map.columns, rotation=90)
    ax1.set_yticks([])
    ax1.tick_params(axis='both', which='major', labelsize=config['tick_label_size'])
    ax1.set_xlim(0, scan_map.shape[1])
    ax1.set_ylim(0, scan_map.shape[0])
    ax1.set_xlabel('Departure object',fontsize=config['axis_label_size'])
    ax1.set_ylabel('Arrival object',fontsize=config['axis_label_size'])

    # Plot axis ordering.
    ax2 = plt.subplot2grid((15,15), (0, 0),rowspan=2,colspan=14,sharex=ax1)
    ax2.step(np.arange(0.5,scan_map.shape[1]+.5),scan_order[str(map_order)],'k',linewidth=2.0)
    ax2.get_yaxis().set_major_formatter(plt.FormatStrFormatter('%.2e'))
    ax2.tick_params(axis='both', which='major', labelsize=config['tick_label_size'])
    plt.setp(ax2.get_xticklabels(), visible=False)
    ax2.set_ylabel(config['map_order_axis_label'],fontsize=config['axis_label_size'])

    # Plot color bar.
    ax3 = plt.subplot2grid((15,15), (0, 14), rowspan=15)
    color_bar = plt.colorbar(heatmap,cax=ax3)
    color_bar.ax.get_yaxis().labelpad = 20
    color_bar.ax.set_ylabel(r'Error [m] or [m/s]', rotation=270)

    plt.tight_layout()

    # Save figure to file.
    plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "_"+str(i+1) +                                               \
                       ".png", dpi=config["figure_dpi"])
    plt.close()
    print "Figure ",i+1," generated successfully...."

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
