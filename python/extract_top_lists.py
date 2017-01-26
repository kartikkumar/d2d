'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2016 Enne Hekma, Delft University of Technology (ennehekma@protonmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
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
print "      Copyright (c) 2016, E.J. Hekma (ennehekma@protonmail.com)   "
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

print "Fetching scan data from database ..."

for i in xrange(0,len(config['database'])):
# for i in xrange(0,1):
    print "Computations being done on database:", config['database_tag'][i]
    print config['database'][i]
    try:
        database = sqlite3.connect(config['database'][i])

    except sqlite3.Error, e:

        print "Error %s:" % e.args[0]
        sys.exit(1)


    toplist_lambert = pd.read_sql("SELECT *, min(transfer_delta_v)        \
                           FROM lambert_scanner_results                                           \
                           GROUP BY departure_object_id,                  \
                                    arrival_object_id                     \
                           ORDER BY transfer_delta_v                      \
                           ASC                                                           \
                           LIMIT 10000000                                                         \
                           ;",                                                                    \
                           database)
    toplist_lambert2 = pd.concat([ toplist_lambert['transfer_id'],
                                   # toplist_lambert['atom_transfer_id'],
                                   toplist_lambert['departure_object_id'],
                                   toplist_lambert['arrival_object_id'],
                                   toplist_lambert['departure_epoch'],
                                   toplist_lambert['time_of_flight'],
                                   toplist_lambert['revolutions'],
                                   # toplist_lambert['time_of_flight'],
                                   toplist_lambert['transfer_delta_v'] ], axis=1)

    toplist_lambert2['lambert_dv_ranking'] = toplist_lambert2['transfer_delta_v'].rank('min')
    toplist_lambert2['combo'] = toplist_lambert2['departure_object_id'].map(str) + '-' + toplist_lambert2['arrival_object_id'].map(str)
    toplist_lambert2.to_csv(str(config['output_directory'] + config['database_tag'][i] + ".csv"), index=False )

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
