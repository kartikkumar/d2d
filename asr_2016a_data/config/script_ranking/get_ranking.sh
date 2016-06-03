sqlite3 ../../SSO/sso_debris.db <<!
.headers on
.mode csv
.output out_all_ssotest.csv
SELECT lambert_scanner_results.departure_object_id,lambert_scanner_results.arrival_object_id,((lambert_scanner_results.departure_epoch-2457399.5)*24*3600),lambert_scanner_results.time_of_flight, lambert_scanner_results.revolutions, min(lambert_scanner_results.transfer_delta_v), sgp4_scanner_results.arrival_position_error , sgp4_scanner_results.arrival_velocity_error FROM sgp4_scanner_results INNER JOIN lambert_scanner_results ON lambert_scanner_results.transfer_id = sgp4_scanner_results.lambert_transfer_id GROUP BY lambert_scanner_results.departure_object_id, lambert_scanner_results.arrival_object_id ORDER BY lambert_scanner_results.transfer_delta_v ASC LIMIT 10;
!

sqlite3 geo_intelsat.db <<!
.headers on
.mode csv
.output out_all_geo2.csv
SELECT lambert_scanner_results.departure_object_id,lambert_scanner_results.arrival_object_id,((lambert_scanner_results.departure_epoch-2457399.5)*24*3600),lambert_scanner_results.time_of_flight, lambert_scanner_results.revolutions, min(lambert_scanner_results.transfer_delta_v), sgp4_scanner_results.arrival_position_error , sgp4_scanner_results.arrival_velocity_error FROM sgp4_scanner_results INNER JOIN lambert_scanner_results ON lambert_scanner_results.transfer_id = sgp4_scanner_results.lambert_transfer_id GROUP BY lambert_scanner_results.departure_object_id, lambert_scanner_results.arrival_object_id ORDER BY lambert_scanner_results.transfer_delta_v ASC LIMIT 10;
!

sqlite3 heo_rocket_bodies.db <<!
.headers on
.mode csv
.output out_all_heo2.csv
SELECT lambert_scanner_results.departure_object_id,lambert_scanner_results.arrival_object_id,((lambert_scanner_results.departure_epoch-2457399.5)*24*3600),lambert_scanner_results.time_of_flight, lambert_scanner_results.revolutions, min(lambert_scanner_results.transfer_delta_v), sgp4_scanner_results.arrival_position_error , sgp4_scanner_results.arrival_velocity_error FROM sgp4_scanner_results INNER JOIN lambert_scanner_results ON lambert_scanner_results.transfer_id = sgp4_scanner_results.lambert_transfer_id GROUP BY lambert_scanner_results.departure_object_id, lambert_scanner_results.arrival_object_id ORDER BY lambert_scanner_results.transfer_delta_v ASC LIMIT 10;
!

# SELECT transfer_delta_v FROM lambert_scanner_results order by transfer_delta_v asc limit 500000;

