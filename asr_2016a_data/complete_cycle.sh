#!/bin/bash
mkdir SSO
mkdir GEO
mkdir HEO
mkdir figures

# Run catalog pruner for all regimes
./../build/bin/d2d ./config/catalog_pruner/catalog_pruner_sso.json
./../build/bin/d2d ./config/catalog_pruner/catalog_pruner_heo_rocket_bodies.json
./../build/bin/d2d ./config/catalog_pruner/catalog_pruner_geo_intelsat.json

# Run lambert_scanner for all regimes
./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_sso.json
./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_heo.json
./../build/bin/d2d ./config/lambert_scanner/lambert_scanner_geo.json

# Run sgp4_scanner for all regimes
./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_sso.json
./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_heo.json
./../build/bin/d2d ./config/sgp4_scanner/sgp4_scanner_geo.json

# Figure 1abcd
python ./../python/plot_tle_catalogs.py

# Figure 2a
python ./../python/plot_porkchop.py ./config/plot_pork_chop_plot/plot_pork_chop_geo.json

# Figure 2b
python ./../python/plot_porkchop_slice.py ./config/plot_pork_chop_slice/plot_porkchop_slice_geo.json

# Figure 3
python ./../python/plot_histogram_dV.py ./config/plot_dV_histogram/plot_histogram_dv_geo.json

# Figure 4
python ./../python/plot_lambert_scan_maps.py ./config/plot_lambert_scan_map/plot_lambert_scan_maps_geo.json

# Figure 6abcd
python ./../python/plot_sgp4_error_histogram.py ./config/sgp4_error_histogram/plot_sgp4_error_histogram_sso.json

# Figure 7abcd
python ./../python/plot_sgp4_error_histogram.py ./config/sgp4_error_histogram/plot_sgp4_error_histogram_heo.json

# Figure 8abcd
python ./../python/plot_sgp4_error_histogram.py ./config/sgp4_error_histogram/plot_sgp4_error_histogram_geo.json

# Figure 9ab
python ./../python/sgp4_scanner_box_plot.py ./config/plot_sgp4_error_box_plot/sgp4_scanner_box_plot.json

# Tables 6/8/10
python ./../python/extract_top_lists.py ./config/extract_ranking_tables/extract_top_lists.json

# Table 7
python ./../python/sgp4_scanner_stats.py ./config/extract_sgp4_error_stats/sgp4_scanner_stats_sso.json

# Table 9
python ./../python/sgp4_scanner_stats.py ./config/extract_sgp4_error_stats/sgp4_scanner_stats_heo.json

# Table 11
python ./../python/sgp4_scanner_stats.py ./config/extract_sgp4_error_stats/sgp4_scanner_stats_geo.json



