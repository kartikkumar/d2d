'''
Copyright (c) 2015, K. Kumar (me@kartikkumar.com)
All rights reserved.
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import matplotlib.animation as animation

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
print "------------------------------------------------------------------"
print "                               D2D                                "
print "                              0.0.2                               "
print "         Copyright (c) 2015, K. Kumar (me@kartikkumar.com)        "
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

print "Input data files being read ..."

# Read and store data files
input_path_prefix = config["input_directory"] + "/"

metadata = pd.read_csv(input_path_prefix + config["metadata"], header=None)
metadata_table = []
metadata_table.append(["Departure ID",int(metadata[1][0])," "])
metadata_table.append(["Arrival ID",int(metadata[1][1])," "])
metadata_table.append(["Departure epoch","{:,g}".format(float(metadata[1][2])),"JD"])
metadata_table.append(["Time-of-flight","{:,g}".format(float(metadata[1][3])),"s"])
metadata_table.append(["Prograde?",metadata[1][4].strip()," "])
metadata_table.append(["Revolutions",int(metadata[1][5])," "])
metadata_table.append([r"Transfer $\Delta V$","{:,g}".format(float(metadata[1][6])),"km/s"])

departure_orbit = pd.read_csv(input_path_prefix + config["departure_orbit"])
arrival_orbit = pd.read_csv(input_path_prefix + config["arrival_orbit"])
transfer_orbit = pd.read_csv(input_path_prefix + config["transfer_orbit"])

departure_path = pd.read_csv(input_path_prefix + config["departure_path"])
arrival_path = pd.read_csv(input_path_prefix + config["arrival_path"])
transfer_path = pd.read_csv(input_path_prefix + config["transfer_path"])

print "Input data files successfully read!"

print "Animation being generated ..."

fig = plt.figure()

# Add X-Y projection subplot
ax1 = fig.add_subplot(2, 2, 1)
departure_object_xy, = ax1.plot([], [],marker='o',color='g')
arrival_object_xy, = ax1.plot([], [],marker='o',color='r')
transfer_object_xy, = ax1.plot([], [],marker='o',color='k')

# Add X-Z projection subplot
ax2 = fig.add_subplot(2, 2, 2)
departure_object_xz, = ax2.plot([], [],marker='o',color='g')
arrival_object_xz, = ax2.plot([], [],marker='o',color='r')
transfer_object_xz, = ax2.plot([], [],marker='o',color='k')

# Add Y-Z projection subplot
ax3 = fig.add_subplot(2, 2, 3)
departure_object_yz, = ax3.plot([], [],marker='o',color='g')
arrival_object_yz, = ax3.plot([], [],marker='o',color='r')
transfer_object_yz, = ax3.plot([], [],marker='o',color='k')

# Draw metadata table
ax4 = fig.add_subplot(2, 2, 4)
ax4.axis('off')
the_table = ax4.table(cellText=metadata_table,colLabels=None,cellLoc='center',loc='center')
the_table.auto_set_font_size(False)
the_table.set_fontsize(6)
table_props = the_table.properties()
table_cells = table_props['child_artists']
for cell in table_cells: cell.set_height(0.15)
cell_dict = the_table.get_celld()
for row in xrange(0,7): cell_dict[(row,2)].set_width(0.1)

plt.subplots_adjust(hspace = 0.35, wspace = 0.35)

# Set up animation functions
def init():
    # Initialize X-Y projection
    ax1.plot(departure_orbit['x'],departure_orbit['y'],color='g')
    ax1.plot(arrival_orbit['x'],arrival_orbit['y'],color='r')
    ax1.plot(transfer_path['x'],transfer_path['y'],color='k')
    ax1.set_xlabel('x [km]')
    ax1.set_ylabel('y [km]')
    ax1.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
    ax1.grid()

    # Initialize X-Z projection
    ax2.plot(departure_orbit['x'],departure_orbit['z'],color='g')
    ax2.plot(arrival_orbit['x'],arrival_orbit['z'],color='r')
    ax2.plot(transfer_path['x'],transfer_path['z'],color='k')
    ax2.set_xlabel('x [km]')
    ax2.set_ylabel('z [km]')
    ax2.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
    ax2.grid()

    # Initialize Y-Z projection
    ax3.plot(departure_orbit['y'],departure_orbit['z'],color='g')
    ax3.plot(arrival_orbit['y'],arrival_orbit['z'],color='r')
    ax3.plot(transfer_path['y'],transfer_path['z'],color='k')
    ax3.set_xlabel('y [km]')
    ax3.set_ylabel('z [km]')
    ax3.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
    ax3.grid()

def animate(i):
    # Animate X-Y project
    departure_object_xy.set_data(departure_path['x'][i],departure_path['y'][i])
    arrival_object_xy.set_data(arrival_path['x'][i],arrival_path['y'][i])
    transfer_object_xy.set_data(transfer_path['x'][i],transfer_path['y'][i])

    # Animate X-Z project
    departure_object_xz.set_data(departure_path['x'][i],departure_path['z'][i])
    arrival_object_xz.set_data(arrival_path['x'][i],arrival_path['z'][i])
    transfer_object_xz.set_data(transfer_path['x'][i],transfer_path['z'][i])

    # Animate Y-Z project
    departure_object_yz.set_data(departure_path['y'][i],departure_path['z'][i])
    arrival_object_yz.set_data(arrival_path['y'][i],arrival_path['z'][i])
    transfer_object_yz.set_data(transfer_path['y'][i],transfer_path['z'][i])

    return departure_object_xy,arrival_object_xy,transfer_object_xy, \
           departure_object_xz,arrival_object_xz,transfer_object_xz, \
           departure_object_yz,arrival_object_yz,transfer_object_yz,

# Generate and save animation
animation_data = animation.FuncAnimation(fig,animate,init_func=init,blit=False, \
                                         frames=len(transfer_path))
animation_path = config["output_directory"] + str(config["tag"]) + "_" + config["animation"] \
                 + ".mp4"
animation_data.save(animation_path,fps=config["frame_rate"],bitrate=config["bit_rate"])

print "Animation generated successfully!"
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