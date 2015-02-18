/*
 * Copyright (c) 2015 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <string>

#include <rapidjson/document.h>

namespace d2d
{

//! Execute TLE catalog pruner.
/*!
 * Execute TLE catalog pruner (application mode: catalog_pruner). This pruner filters a catalog
 * provided by the user and produces a new catalog which is a subset of the original.
 *
 * The filters available currently are:
 *  - altitude (semi-major axis - Earth radius) [km]
 *  - inclination                               [deg]
 *  - eccentricity                              [-]
 *  - line-0 regex (performs regex match on line-0 of TLE; only works for 3-line TLE )
 *
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeCatalogPruner( const rapidjson::Document& config );

//! catalog_pruner input.
/*!
 * Data struct containing all valid input parameters for the "catalog_pruner application mode.
 * This struct is populated by the checkCatalogPrunerInput() function.
 *
 * @sa checkCatalogPrunerInput, executeCatalogPruner
 */
struct CatalogPrunerInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkCatalogPrunerInput, executeCatalogPruner
     * @param[in] aCatalogPath            Path to TLE catalog
     * @param[in] aSemiMajorAxisMinimum   Minimum semi-major axis filter     [km]
     * @param[in] aSemiMajorAxisMaximum   Maximum semi-major axis filter     [km]
     * @param[in] anEccentricityMinimum   Minimum eccentricity filter        [-]
     * @param[in] anEccentricityMaximum   Maximum eccentricity filter        [-]
     * @param[in] anInclinationMinimum    Minimum inclination filter         [deg]
     * @param[in] anInclinationMaximum    Maximum inclination filter         [deg]
     * @param[in] aNameRegex              Regex filter for TLE object name
     * @param[in] aCatalogCutoff          Cutoff that sets maximum objects
     * @param[in] aPrunedCatalogPath      Path to pruned TLE catalog
     */
    CatalogPrunerInput( const std::string& aCatalogPath,
                        const double aSemiMajorAxisMinimum,
                        const double aSemiMajorAxisMaximum,
                        const double anEccentricityMinimum,
                        const double anEccentricityMaximum,
                        const double anInclinationMinimum,
                        const double anInclinationMaximum,
                        const std::string& aNameRegex,
                        const int aCatalogCutoff,
                        const std::string& aPrunedCatalogPath )
        : catalogPath( aCatalogPath ),
          semiMajorAxisMinimum( aSemiMajorAxisMinimum ),
          semiMajorAxisMaximum( aSemiMajorAxisMaximum ),
          eccentricityMinimum( anEccentricityMinimum ),
          eccentricityMaximum( anEccentricityMaximum ),
          inclinationMinimum( anInclinationMinimum ),
          inclinationMaximum( anInclinationMaximum ),
          nameRegex( aNameRegex ),
          catalogCutoff( aCatalogCutoff ),
          prunedCatalogPath( aPrunedCatalogPath )
    { }

    //! Path to TLE catalog.
    const std::string catalogPath;

    //! Minimum semi-major axis filter (Earth radius subtracted) [km].
    const double semiMajorAxisMinimum;

    //! Maximum semi-major axis filter (Earth radius subtracted) [km].
    const double semiMajorAxisMaximum;

    //! Minimum eccentricity filter [-].
    const double eccentricityMinimum;

    //! Maximum eccentricity filter [-].
    const double eccentricityMaximum;

    //! Minimum inclination filter [deg].
    const double inclinationMinimum;

    //! Maximum inclination filter [deg].
    const double inclinationMaximum;

    //! Name regex.
    const std::string nameRegex;

    //! Cutoff that sets them aximum objects in pruned catalog.
    const int catalogCutoff;

    //! Pruned catalog path.
    const std::string prunedCatalogPath;

protected:

private:
};

//! Check catalog_pruner input parameters.
/*!
 * Checks that all inputs for the "catalog_pruner" application mode are valid. If not, an error is
 * thrown with a short description of the problem.
 *
 * If all inputs are valid, a data struct containing all the inputs is returned, which is
 * subsequently used to execute "catalog_pruner" and related functions.
 *
 * @sa executeCatalogPruner, CatalogPrunerInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input
 */
CatalogPrunerInput checkCatalogPrunerInput( const rapidjson::Document& config );

} // namespace d2d
