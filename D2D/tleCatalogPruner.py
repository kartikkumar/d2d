'''
Copyright (c) 2014, Dinamica Srl
Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
All rights reserved.
See COPYING for license details.
'''

def convertMeanMotionToSemiMajorAxis(meanMotion,gravitionalParameter):
	return ( gravitionalParameter[1] / ( meanMotion * meanMotion ) ) ** ( 1.0 / 3.0 )

####################################################################################################
# Set up input deck
####################################################################################################

# Set path to TLE catalog file.
tleCatalogFilePath 			= "/home/kartikkumar/Applications/d2d/D2D/140605_tleCatalog3line.txt"

# Set number of lines per entry in TLE catalog (2 or 3).
tleEntryNumberOfLines		= 3

# Set path to output directory.
outputPath 					= "/home/kartikkumar/ownCloud/Shared/Stardust Tech/" \
							  + "Output/ASR_Debris2Debris/Figures"

# Set figure DPI.
figureDPI 					= 300

# Set font size for axes labels.
fontSize 					= 24

####################################################################################################

'''

						DO NOT EDIT PARAMETERS BEYOND THIS POINT!!!

'''

####################################################################################################
# Set up Python modules and packages
####################################################################################################

from sgp4.earth_gravity import wgs72
from sgp4.io import twoline2rv
from sgp4.propagation import getgravconst

from matplotlib import rcParams
import matplotlib.pyplot as plt
import numpy as np

####################################################################################################

####################################################################################################
# Parse TLE catalog
####################################################################################################

# Read in catalog and store lines in list.
fileHandle = open(tleCatalogFilePath)
catalogLines = fileHandle.readlines()
fileHandle.close()

# Strip newline and return carriage characters.
for i in xrange(len(catalogLines)):
    catalogLines[i] = catalogLines[i].strip('\r\n')

# Parse TLE entries and store debris objects.
debrisObjects = []
for tleEntry in xrange(0,len(catalogLines),tleEntryNumberOfLines):
	debrisObjects.append(twoline2rv(catalogLines[tleEntry+1], catalogLines[tleEntry+2], wgs72))

# Sort list of debris objects based on inclination.
inclinationSortedObjects = sorted(debrisObjects, key=lambda x: x.inclo, reverse=False)

inclinations = []
raan = []
ecc = []
aop = []
for i in xrange(len(inclinationSortedObjects)-1):
	inclinations.append(inclinationSortedObjects[i].inclo)	
	raan.append(inclinationSortedObjects[i].nodeo)
	ecc.append(inclinationSortedObjects[i].ecco)	
	aop.append(inclinationSortedObjects[i].argpo)	
 
####################################################################################################
# Generate plots
####################################################################################################

# Set font size for plot labels.
rcParams.update({'font.size': fontSize})

# Plot distribution of eccentricity [-] against semi-major axis [km].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("Semi-major axis [km]")
plt.ylabel("Eccentricity [-]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
plt.plot([convertMeanMotionToSemiMajorAxis(debrisObject.no/60.0, getgravconst('wgs72')) \
		  for debrisObject in debrisObjects], \
	     [debrisObject.ecco for debrisObject in debrisObjects], \
	     marker='o', markersize=5, linestyle='none')
axis.set_xlim(xmax=5.0e4)
plt.tight_layout(True)
plt.savefig(outputPath + "/figure1_debrisPopulation_eccentricityVsSemiMajorAxis.pdf", \
			dpi = figureDPI)
plt.close()

# Plot components of eccentricity vector [-].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$e \cos{\omega}$ [-]")
plt.ylabel("$e \sin{\omega}$ [-]")
plt.plot(ecc*np.cos(aop),ecc*np.sin(aop), marker='o', markersize=5, linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-1.0, xmax=1.0)
axis.set_ylim(ymin=-1.0, ymax=1.0)
plt.tight_layout(True)
plt.savefig(outputPath + "/figure2_debrisPopulation_eccentricityVector.pdf", dpi = figureDPI)
plt.close()

# Plot distribution of inclination [deg] against semi-major axis [km].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("Semi-major axis [km]")
plt.ylabel("Inclination [deg]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
plt.plot([convertMeanMotionToSemiMajorAxis(debrisObject.no/60.0, getgravconst('wgs72')) \
		  for debrisObject in debrisObjects], \
	     [np.rad2deg(debrisObject.inclo) for debrisObject in debrisObjects], \
	     marker='o', markersize=5, linestyle='none')
axis.set_xlim(xmax=5.0e4)
plt.tight_layout(True)
plt.savefig(outputPath + "/figure3_debrisPopulation_inclinationVsSemiMajorAxis.pdf", \
			dpi = figureDPI)
plt.close()

# Plot components of inclination vector [deg].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$i \cos{\Omega}$ [rad]")
plt.ylabel("$i \sin{\Omega}$ [rad]")
plt.plot(np.rad2deg(inclinations)*np.cos(raan),np.rad2deg(inclinations)*np.sin(raan), \
		 marker='o', markersize=5, linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-180.0, xmax=180.0)
axis.set_ylim(ymin=-180.0, ymax=180.0)
plt.tight_layout(True)
plt.savefig(outputPath + "/figure4_debrisPopulation_inclinationVector.pdf", dpi = figureDPI)
plt.close()

####################################################################################################
