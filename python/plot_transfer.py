'''
Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
All rights reserved.
'''

###################################################################################################
# Set up input deck
###################################################################################################

# Set input path.
input_path                    = "../config/lambert_transfer_data/23754_23842/"

# Set solution ID.
solutionID                    = 5

# Set path to data files.
input_path_prefix             = input_path + "sol" + str(solutionID) + "_"
metadata_file                 = input_path_prefix + "metadata.csv"
departure_orbit_file          = input_path_prefix + "departure_orbit.csv"
departure_path_file           = input_path_prefix + "departure_path.csv"
arrival_orbit_file            = input_path_prefix + "arrival_orbit.csv"
arrival_path_file             = input_path_prefix + "arrival_path.csv"
transfer_orbit_file           = input_path_prefix + "transfer_orbit.csv"
transfer_path_file            = input_path_prefix + "transfer_path.csv"

# Set path to output directory.
output_path                   = "../config/plots/23754_23842/"
output_path_prefix            = output_path + "sol" + str(solutionID) + "_"

# Set output path for 2D figures.
twoD_figure                   = output_path_prefix + "2d_figure.png"

# Set output path for 3D figure.
threeD_figure                 = output_path_prefix + "3d_figure.png"

# Set figure DPI.
figure_dpi                    = 300

# Set font size for axes labels.
font_size                     = 24

# Save animation?
animate                       = True

###################################################################################################

'''

						DO NOT EDIT PARAMETERS BEYOND THIS POINT!!!

'''

###################################################################################################
# Set up modules and packages
###################################################################################################

import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d

import numpy as np
import pandas as pd

###################################################################################################

###################################################################################################
# Read and store data
###################################################################################################

metadata = pd.read_csv(metadata_file, header=None)
metadata_table = []
metadata_table.append(["Departure ID",int(metadata[1][0])," "])
metadata_table.append(["Arrival ID",int(metadata[1][1])," "])
metadata_table.append(["Departure epoch","{:,g}".format(metadata[1][2]),"JD"])
metadata_table.append(["Time-of-flight","{:,g}".format(metadata[1][3]),"s"])
metadata_table.append(["Prograde?",str(bool(metadata[1][4]))," "])
metadata_table.append(["Revolutions",int(metadata[1][5])," "])
metadata_table.append([r"Transfer $\Delta V$","{:,g}".format(metadata[1][6]),"km/s"])

departure_orbit = pd.read_csv(departure_orbit_file)
arrival_orbit = pd.read_csv(arrival_orbit_file)
transfer_orbit = pd.read_csv(transfer_orbit_file)

departure_path = pd.read_csv(departure_path_file)
arrival_path = pd.read_csv(arrival_path_file)
transfer_path = pd.read_csv(transfer_path_file)

last = len(transfer_path) - 1

###################################################################################################

###################################################################################################
# Plot Lambert transfer
###################################################################################################

# Generate figure with 2D views

# Plot X-Y projection
plt.subplot(2, 2, 1)
plt.plot(departure_orbit['x'],departure_orbit['y'],color='g')
plt.plot(arrival_orbit['x'],arrival_orbit['y'],color='r')
plt.plot(transfer_orbit['x'],transfer_orbit['y'],color='k')
plt.scatter(transfer_path['x'][0],transfer_path['y'][0],s=100,marker='o',color='g')
plt.scatter(transfer_path['x'][last],transfer_path['y'][last],s=100,marker='o',color='r')
plt.xlabel('x [km]')
plt.ylabel('y [km]')
plt.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
plt.grid()

# Plot X-Z projection
plt.subplot(2, 2, 2)
plt.plot(departure_orbit['x'],departure_orbit['z'],color='g')
plt.plot(arrival_orbit['x'],arrival_orbit['z'],color='r')
plt.plot(transfer_orbit['x'],transfer_orbit['z'],color='k')
plt.scatter(transfer_path['x'][0],transfer_path['z'][0],s=100,marker='o',color='g')
plt.scatter(transfer_path['x'][last],transfer_path['z'][last],s=100,marker='o',color='r')
plt.xlabel('x [km]')
plt.ylabel('z [km]')
plt.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
plt.grid()

# Plot Y-Z projection
plt.subplot(2, 2, 3)
plt.plot(departure_orbit['y'],departure_orbit['z'],color='g')
plt.plot(arrival_orbit['y'],arrival_orbit['z'],color='r')
plt.plot(transfer_orbit['y'],transfer_orbit['z'],color='k')
plt.scatter(transfer_path['y'][0],transfer_path['z'][0],s=100,marker='o',color='g')
plt.scatter(transfer_path['y'][last],transfer_path['z'][last],s=100,marker='o',color='r')
plt.xlabel('y [km]')
plt.ylabel('z [km]')
plt.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
plt.grid()

# Plot metadata table
plt.subplot(2, 2, 4, frameon=False)
plt.axis('off')
the_table = plt.table(cellText=metadata_table,colLabels=None,cellLoc='center',loc='center')
table_props = the_table.properties()
table_cells = table_props['child_artists']
for cell in table_cells: cell.set_height(0.15)
cell_dict = the_table.get_celld()
for row in xrange(0,7): cell_dict[(row,2)].set_width(0.1)
plt.tight_layout()
plt.savefig(twoD_figure, dpi=figure_dpi)

# # Plot in 3D
# figure = plt.figure()
# ax = figure.gca(projection='3d')
# # Plot sphere for the Earth
# u = np.linspace(0, 2 * np.pi, 100)
# v = np.linspace(0, np.pi, 100)
# x = 6371 * np.outer(np.cos(u), np.sin(v))
# y = 6371 * np.outer(np.sin(u), np.sin(v))
# z = 6371 * np.outer(np.ones(np.size(u)), np.cos(v))
# ax.plot_surface(x, y, z,  rstride=4, cstride=4, color='b',edgecolors='b')

# # Plot departure and arrival orbits
# ax.plot3D(departure['x'],departure['y'],departure['z'],'g')
# ax.plot3D(arrival['x'],arrival['y'],arrival['z'],'r')

# # # Plot transfer trajectory
# # ax.plot3D(transfer['x'],transfer['y'],transfer['z'],linestyle='None',marker='.',c='k')
# # ax.scatter(transfer['x'][0],transfer['y'][0],transfer['z'][0],s=100,marker='o',c='g')
# # ax.scatter(transfer['x'][last],transfer['y'][last],transfer['z'][last],s=100,marker='o',c='r')

# # Create cubic bounding box to simulate equal aspect ratio
# X = departure['x']
# Y = departure['y']
# Z = departure['z']

# max_range = np.array([X.max()-X.min(), Y.max()-Y.min(), Z.max()-Z.min()]).max() / 2.0

# mean_x = X.mean()
# mean_y = Y.mean()
# mean_z = Z.mean()
# ax.set_xlim(mean_x - max_range, mean_x + max_range)
# ax.set_ylim(mean_y - max_range, mean_y + max_range)
# ax.set_zlim(mean_z - max_range, mean_z + max_range)
# plt.grid()

# # Show all figures.
# #plt.show()

# ###################################################################################################
