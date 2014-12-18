/*
 * Copyright (c) 2014 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <string>

#include <libsgp4/DateTime.h>
#include <libsgp4/Tle.h>

#include <rapidjson/document.h>

namespace d2d
{

//! Execute single Lambert transfer.
/*!
 * Executes a single Lambert transfer to compute \f$\Delta V\f$ for departure and arrival with
 * respect to given catalog objects. The transfer is modelled as conic sections. The Lambert
 * targeter employed is based on Izzo (2014), implemented in PyKEP (Izzo, 2012).
 *
 * This function is called when the user specifies the application mode to be "lambert_transfer".
 *
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeLambertTransfer( const rapidjson::Document& config );

//! Lambert single input.
/*!
 * Data struct containing all valid input parameters to execute a single Lambert transfer. This
 * struct is populated by the checkLambertTransferInput() function and can be used to execute a
 * single Lambert transfer.
 *
 * @sa checkLambertTransferInput, executeLambertTransfer
 */
struct LambertTransferInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkLambertTransferInput, executeLambertTransfer
     * @param[in] aDepartureObject              Departure object constructed from TLE strings
     * @param[in] anArrivalObject               Arrival object constructed from TLE strings
     * @param[in] aDepartureEpoch               Departure epoch
     * @param[in] aTimeOfFlight                 Time-of-flight [s]
     * @param[in] progradeFlag                  Flag indicating if prograde transfer should be
     *                                          computed (false = retrograde)
     * @param[in] aRevolutionsMaximum           Maximum number of revolutions
     * @param[in] aSolutionOutputType           Which solutions to write to file
     * @param[in] numberOfOutputSteps           Number of time steps to generate for output files
     * @param[in] anOutputDirectory             Path to output directory to write output files to
     *                                          (relative or absolute)
     * @param[in] aMetadataFilename             Output filename for simulation metadata
     * @param[in] aDepartureOrbitFilename       Output filename for sampled departure orbit
     * @param[in] aDeparturePathFilename        Output filename for sampled path of departure
     *                                          object
     * @param[in] anArrivalOrbitFilename        Output filename for sampled arrival orbit
     * @param[in] anArrivalPathFilename         Output filename for sampled path of arrival object
     * @param[in] aTransferOrbitFilename        Output filename for sampled transfer orbit
     * @param[in] aTransferPathFilename         Output filename for sampled transfer path
     */
    LambertTransferInput( const Tle&         aDepartureObject,
                          const Tle&         anArrivalObject,
                          const DateTime&    aDepartureEpoch,
                          const double       aTimeOfFlight,
                          const bool         progradeFlag,
                          const int          aRevolutionsMaximum,
                          const std::string& aSolutionOutputType,
                          const int          numberOfOutputSteps,
                          const std::string& anOutputDirectory,
                          const std::string& aMetadataFilename,
                          const std::string& aDepartureOrbitFilename,
                          const std::string& aDeparturePathFilename,
                          const std::string& anArrivalOrbitFilename,
                          const std::string& anArrivalPathFilename,
                          const std::string& aTransferOrbitFilename,
                          const std::string& aTransferPathFilename )
        : departureObject( aDepartureObject ),
          arrivalObject( anArrivalObject ),
          departureEpoch( aDepartureEpoch ),
          timeOfFlight( aTimeOfFlight ),
          isPrograde( progradeFlag ),
          revolutionsMaximum( aRevolutionsMaximum ),
          solutionOutput( aSolutionOutputType ),
          outputSteps( numberOfOutputSteps ),
          outputDirectory( anOutputDirectory ),
          metadataFilename( aMetadataFilename ),
          departureOrbitFilename( aDepartureOrbitFilename ),
          departurePathFilename( aDeparturePathFilename ),
          arrivalOrbitFilename( anArrivalOrbitFilename ),
          arrivalPathFilename( anArrivalPathFilename ),
          transferOrbitFilename( aTransferOrbitFilename ),
          transferPathFilename( aTransferPathFilename )
    { }

    //! TLE object at departure.
    const Tle departureObject;

    //! TLE object at arrival.
    const Tle arrivalObject;

    //! Departure epoch.
    const DateTime departureEpoch;

    //! Time-of-flight [s].
    const double timeOfFlight;

    //! Flag indicating if transfers are prograde. False indicates retrograde.
    const bool isPrograde;

    //! Maximum number of revolutions (N) for transfer. Number of revolutions is 2*N+1.
    const int revolutionsMaximum;

    //! Solution output type that sets which solutions to write to file.
    const std::string solutionOutput;

    //! Number of time steps to generate for output.
    const int outputSteps;

    //! Path to output directory (relative or absolute).
    const std::string outputDirectory;

    //! Simulation metadata output filename.
    const std::string metadataFilename;

    // Output filename for sampled departure orbit.
    const std::string departureOrbitFilename;

    // Output filename for sampled path of departure object.
    const std::string departurePathFilename;

    // Output filename for sampled arrival orbit.
    const std::string arrivalOrbitFilename;

    // Output filename for sampled path of arrival object.
    const std::string arrivalPathFilename;

    // Output filename for sampled transfer orbit.
    const std::string transferOrbitFilename;

    // Output filename for sampled transfer path.
    const std::string transferPathFilename;

protected:

private:
};

//! Check input parameters Lambert transfer.
/*!
 * Checks that all inputs to execute a single Lambert transfer are valid. If not, an error is
 * thrown with a short description of the problem. If all inputs are valid, a data struct
 * containing all the inputs is returned, which is subsequently used to execute the Lambert
 * scanner and related functions.
 *
 * @sa executeLambertTransfer, LambertTransferInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return          Struct containing all valid input to execute single Lambert transfer
 */
LambertTransferInput checkLambertTransferInput( const rapidjson::Document& config );

} // namespace d2d

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in
 *  astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth
 *  International Conf. Astrodynam. Tools and Techniques, ICATT. 2012.
 */
