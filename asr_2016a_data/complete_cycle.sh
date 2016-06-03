#!/bin/bash


# ./../build/bin/d2d ./config/catalog_pruner/catalog_pruner_sso.json
# ./../build/bin/d2d ./config/catalog_pruner/catalog_pruner_heo_rocket_bodies.json
# ./../build/bin/d2d ./config/catalog_pruner/catalog_pruner_geo_intelsat.json

# ./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_sso.json
# ./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_heo.json
# ./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_geo.json

# ./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_sso.json
# ./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_heo.json
# ./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_geo.json

python ./../python/plot_histogram_dV.py ./config/plot_dV_histogram/plot_histogram_dv_geo.json

# python ./../python/sgp4_scanner_stats.py ./config/extract_sgp4_error_stats/sgp4_scanner_stats_sso.json
# python ./../python/sgp4_scanner_stats.py ./config/extract_sgp4_error_stats/sgp4_scanner_stats_heo.json
# python ./../python/sgp4_scanner_stats.py ./config/extract_sgp4_error_stats/sgp4_scanner_stats_geo.json



