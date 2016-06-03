#!/bin/bash

./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_geo.json
./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_geo.json

python ../python/plot_lambert_scan_maps.py /config/plot_lambert_scan_map/plot_lambert_scan_maps_geo.json
python ../python/plot_lambert_scan_maps.py /config/plot_lambert_scan_map/plot_lambert_scan_maps_geo_bw.json












# read -n6 -r -p "Insert 'pruner' to inspect inut file and execute d2d in pruner mode: " key
# if [ "$key" = 'pruner' ]; then
#     vi ../../data/config/pruner/catalog_pruner_geo_intelsat.json
# 	./../../build/bin/d2d ../../data/config/pruner/catalog_pruner_geo_intelsat.json
# fi

# read -n1 -r -p "Insert 'lambert_scanner' to inspect inut file and execute d2d in lambert_scanner mode..." key
# if [ "$key" = 'lambert_scanner' ]; then
# 	vi ../../data/config/lambert_scanner/lambert_scanner_geo.json
# 	./../../build/bin/d2d ../../data/config/lambert_scanner/lambert_scanner_geo.json
# fi

# plot pork chop
# python ../../python/plot_porkchop.py ../../data/GEO/python/plot_porkchop_sso.json

# plot pork chop slice
# python ../../python/plot_porkchop_slice.py ../../data/GEO/python/plot_porkchop_slice_geo.json


# echo "lambert_scanner complete, please inspect sgp4_scanner input file."
# read -rsp $'Press any key to continue...\n' -n1 key
# vi ../../data/config/pruner/catalog_pruner_geo_intelsat.json
# ./../../build/bin/d2d ../../data/config/lambert_scanner/lambert_scanner_geo.json


# echo "spg4_scanner complete, plots will now be created."
# read -rsp $'Press any key to continue...\n' -n1 key
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_a_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_a_dep.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_aop_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_aop_dep.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_e_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_e_dep.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_i_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_i_dep.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_raan_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py geo_intelsat_lambert_scanner_map_order_raan_dep.json




# # echo "Please inspect pruner input file."
# # read -rsp $'Press any key to continue...\n' -n1 key
# echo "Pruning complete, please inspect lambert_scanner input file."
# read -rsp $'Press any key to continue...\n' -n1 key
