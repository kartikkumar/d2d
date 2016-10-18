'''
Copyright (c) 2014-2016, K. Kumar (me@kartikkumar.com)
Copyright (c) 2016, E.J. Hekma (ennehekma@gmail.com)
All rights reserved.
'''

###################################################################################################
# Set up input deck
###################################################################################################

# Set path to TLE catalog file.
tleCatalogFilePathall		= "../asr_2016a_data/3le.txt"
tleCatalogFilePathSSO		= "../asr_2016a_data/SSO/SSO_tle.txt"
tleCatalogFilePathGEO		= "../asr_2016a_data/GEO/GEO_intelsat_tle.txt"
tleCatalogFilePathHEO		= "../asr_2016a_data/HEO/heo_rocket_bodies_catalog.txt"

# Set number of lines per entry in TLE catalog (2 or 3).
tleEntryNumberOfLines		= 3

# Set path to output directory.
outputPath 					= "../asr_2016a_data/figures/"

# Set figure DPI.
figureDPI 					= 300

# Set font size for axes labels.
fontSize 					= 22

###################################################################################################

'''
						DO NOT EDIT PARAMETERS BEYOND THIS POINT!!!
'''

###################################################################################################
# Set up modules and packages
###################################################################################################

from sgp4.earth_gravity import wgs72
from sgp4.io import twoline2rv
from sgp4.propagation import getgravconst

from matplotlib import rcParams
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from twoBodyMethods import convertMeanMotionToSemiMajorAxis

###################################################################################################

###################################################################################################
# Read and store TLE catalog
###################################################################################################

sma = []
ecc5 = []
incl5 = []

raan3 = []
ecc3 = []
aop3 = []
inclinations3 = []

# colors = ['k','b','g','r']
# markers = ['.','s','+','D']

order = ['all','SSO','GEO','HEO']
for x in order:
	tleCatalogFilePathNew = eval(str('tleCatalogFilePath' + str(x)))

	# Read in catalog and store lines in list.
	fileHandle = open(tleCatalogFilePathNew)
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
	for i in xrange(len(inclinationSortedObjects)):
		inclinations.append(inclinationSortedObjects[i].inclo)
		raan.append(inclinationSortedObjects[i].nodeo)
		ecc.append(inclinationSortedObjects[i].ecco)
		aop.append(inclinationSortedObjects[i].argpo)

	smatemp = []
	smatemp = [convertMeanMotionToSemiMajorAxis(debrisObject.no/60.0,							  \
			   getgravconst('wgs72')[1])-6373													  \
		  	   for debrisObject in debrisObjects]
	smatemp2 = []
	smatemp2 = pd.DataFrame(smatemp, columns=[str(x)])
	sma.append(smatemp2)

	ecctemp = []
	ecctemp = [debrisObject.ecco for debrisObject in debrisObjects]
	ecc4 = []
	ecc4 = pd.DataFrame(ecctemp, columns=[str(x)])
	ecc5.append(ecc4)

	incltemp = []
	incltemp = [np.rad2deg(debrisObject.inclo) for debrisObject in debrisObjects]
	incl4 = []
	incl4 = pd.DataFrame(incltemp, columns=[str(x)])
	incl5.append(incl4)

	raan2 = []
	raan2 = pd.DataFrame(raan, columns=[str(x)])
	raan3.append(raan2)

	ecc2 = []
	ecc2 = pd.DataFrame(ecc, columns=[str(x)])
	ecc3.append(ecc2)

	aop2 = []
	aop2 = pd.DataFrame(aop, columns=[str(x)])
	aop3.append(aop2)

	inclinations2 = []
	inclinations2 = pd.DataFrame(inclinations, columns=[str(x)])
	inclinations3.append(inclinations2)

sma = pd.concat(sma, axis=1)
ecc5 = pd.concat(ecc5, axis=1)
incl5 = pd.concat(incl5, axis=1)

raan3 = pd.concat(raan3, axis=1)
ecc3 = pd.concat(ecc3, axis=1)
aop3 = pd.concat(aop3, axis=1)
inclinations3 = pd.concat(inclinations3, axis=1)

###################################################################################################

###################################################################################################
# Generate plots
###################################################################################################

# Set font size for plot labels.
rcParams.update({'font.size': fontSize})

# Plot distribution of eccentricity [-] against semi-major axis [km].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$a_H$ [km]")
plt.ylabel("$e$ [-]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
plt.plot(sma['all'],ecc3['all'], 																  \
		 	marker='.', markersize=1, color='k', linestyle='none')
plt.plot(sma['SSO'],ecc3['SSO'], 																  \
		 	marker='s', markersize=10, color='c', linestyle='none')
plt.plot(sma['GEO'],ecc3['GEO'], 																  \
		 	marker='^', markersize=10, color='g', linestyle='none')
plt.plot(sma['HEO'],ecc3['HEO'], 																  \
		 	marker='D', markersize=6, color='r', linestyle='none')
axis.set_xlim(xmax=0.4e5)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/Figure_1a_debrisPopulation_eccentricityVsSemiMajorAxis.pdf", 			  \
			dpi = figureDPI)
plt.close()

# Plot components of eccentricity vector [-].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$e \cos{\omega}$ [-]")
plt.ylabel("$e \sin{\omega}$ [-]")
plt.plot(ecc3['all']*np.cos(aop3['all']),ecc3['all']*np.sin(aop3['all']), 						  \
		 	marker='.', markersize=1, color='k', linestyle='none')
plt.plot(ecc3['SSO']*np.cos(aop3['SSO']),ecc3['SSO']*np.sin(aop3['SSO']), 						  \
		 	marker='s', markersize=10, color='c', linestyle='none')
plt.plot(ecc3['GEO']*np.cos(aop3['GEO']),ecc3['GEO']*np.sin(aop3['GEO']), 						  \
		 	marker='^', markersize=10, color='g', linestyle='none')
plt.plot(ecc3['HEO']*np.cos(aop3['HEO']),ecc3['HEO']*np.sin(aop3['HEO']), 						  \
		 	marker='D', markersize=6, color='r', linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-.82, xmax=.82)
axis.set_ylim(ymin=-.82, ymax=.82)
axis.set(xticks=[-.8,-.4,0,.4,.8])
axis.set(yticks=[-.8,-.4,0,.4,.8])
figure.set_tight_layout(True)
plt.savefig(outputPath + "/Figure_1c_debrisPopulation_eccentricityVector.pdf", dpi = figureDPI)
plt.close()


# Plot distribution of inclination [deg] against semi-major axis [km].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$a_H$ [km]")
plt.ylabel("$i$ [deg]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
plt.plot(sma['all'],incl5['all'], 																  \
		 	marker='.', markersize=1, color='k', linestyle='none')
plt.plot(sma['SSO'],incl5['SSO'], 																  \
		 	marker='s', markersize=10, color='c', linestyle='none')
plt.plot(sma['GEO'],incl5['GEO'], 																  \
		 	marker='^', markersize=10, color='g', linestyle='none')
plt.plot(sma['HEO'],incl5['HEO'], 																  \
		 	marker='D', markersize=6, color='r', linestyle='none')
axis.set_xlim(xmax=0.4e5)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/Figure_1b_debrisPopulation_inclinationVsSemiMajorAxis.pdf", 			  \
			dpi = figureDPI)
plt.close()

# Plot components of inclination vector [deg].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$i \cos{\Omega}$ [deg]")
plt.ylabel("$i \sin{\Omega}$ [deg]")
plt.plot(np.rad2deg(inclinations3['all'])*np.cos(raan3['all']),									  \
		 np.rad2deg(inclinations3['all'])*np.sin(raan3['all']), 							  	  \
		 	marker='.', markersize=1, color='k', linestyle='none')
plt.plot(np.rad2deg(inclinations3['SSO'])*np.cos(raan3['SSO']),									  \
		 np.rad2deg(inclinations3['SSO'])*np.sin(raan3['SSO']), 							  	  \
		 	marker='s', markersize=10, color='c', linestyle='none')
plt.plot(np.rad2deg(inclinations3['GEO'])*np.cos(raan3['GEO']),									  \
		 np.rad2deg(inclinations3['GEO'])*np.sin(raan3['GEO']), 							  	  \
		 	marker='^', markersize=10, color='g', linestyle='none')
plt.plot(np.rad2deg(inclinations3['HEO'])*np.cos(raan3['HEO']),									  \
		 np.rad2deg(inclinations3['HEO'])*np.sin(raan3['HEO']), 							  	  \
		 	marker='D', markersize=6, color='r', linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-110.0, xmax=110.0)
axis.set_ylim(ymin=-110.0, ymax=110.0)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/Figure_1c_debrisPopulation_inclinationVector.pdf", dpi = figureDPI)
plt.close()




###################################################################################################
###################################################################################################
