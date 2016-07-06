/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_LAMBERT_SCANNER_HPP
#define D2D_LAMBERT_SCANNER_HPP

#include <string>

#include <keplerian_toolbox.h>

#include <libsgp4/DateTime.h>
#include <libsgp4/Tle.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include "D2D/tools.hpp"

namespace d2d
{

//! Forward declaration.
struct LambertScannerInput;
struct LambertPorkChopPlotGridPoint;

//! Lambert pork-chop plot, consisting of list of grid points.
typedef std::vector< LambertPorkChopPlotGridPoint > LambertPorkChopPlot;
//! Collection of all Lambert pork-chop plots with leg, departure object & arrival object IDs.
typedef std::map< PorkChopPlotId, LambertPorkChopPlot > AllLambertPorkChopPlots;

//! Execute lambert_scanner.
/*!
 * Executes lambert_scanner application mode that performs a grid search to compute \f$\Delta V\f$
 * for debris-to-debris transfers. The transfers are modelled as conic sections. The Lambert
 * targeter employed is based on Izzo (2014), implemented in PyKEP (Izzo, 2012).
 *
 * The results obtained from the grid search are stored in a SQLite database, containing the
 * following table:
 *
 *	- "lambert_scanner_results": contains all Lambert transfers computed during grid search
 *
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeLambertScanner( const rapidjson::Document& config );

//! Check lambert_scanner input parameters.
/*!
 * Checks that all inputs for the lambert_scanner application mode are valid. If not, an error is
 * thrown with a short description of the problem. If all inputs are valid, a data struct
 * containing all the inputs is returned, which is subsequently used to execute lambert_scanner
 * and related functions.
 *
 * @sa executeLambertScanner, LambertScannerInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute lambert_scanner
 */
LambertScannerInput checkLambertScannerInput( const rapidjson::Document& config );

//! Create lambert_scanner table.
/*!
 * Creates lambert_scanner and lambert_sequences tables in SQLite database used to store results
 * obtained from running the lambert_scanner application mode.
 *
 * @sa executeLambertScanner
 * @param[in] database          SQLite database handle
 * @param[in] sequenceLength    Number of targets in sequence
 */
void createLambertScannerTables( SQLite::Database& database, const int sequenceLength );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of debris-to-debris Lambert transfers to file. The shortlist is based on the
 * requested number of transfers with the lowest transfer \f$\Delta V\f$, retrieved by sorting the
 * transfers in the SQLite database.
 *
 * @sa executeLambertScanner, createLambertScannerTable
 * @param[in] database        SQLite database handle
 * @param[in] shortlistNumber Number of entries to include in shortlist (if it exceeds number of
 *                            entries in database table, the whole table is written to file)
 * @param[in] shortlistPath   Path to shortlist file
 */
void writeTransferShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath );

//! Recurse through sequences leg-by-leg and compute pork-chop plots.
/*!
 * Recurses through pool of TLE objects to compute sequences and generates pork-chop plots based
 * on specified departure and arrival objects for a given leg, whilst traversing the tree of
 * sequences. The departure and arrival epochs have to be generated apriori for each leg and
 * are provided by the user. The transfers are computed using the Lambert solver (Izzo, 2014).
 *
 * All of the pork-chop plots are stored in the container provided by the user (AllPorkChopPlots).
 *
 * @sa executeLambertScanner
 * @param[in]       currentSequencePosition     Current position along sequence
 * @param[in]       tleObjects                  Pool of TLE objects to select from
 * @param[in]       allEpochs                   Pre-computed departure and arrival epochs for each
 *                                              leg
 * @param[in]       isPrograde                  Flag indicating if prograde transfer should be
 *                                              computed (false = retrograde)
 * @param[in]       revolutionsMaximum          Maximum number of revolutions
 * @param[out]      sequence                    Sequence of TLE objects
 * @param[in]       transferId                  Counter for unique ID assigned to transfers
 * @param[out]      allPorkChopPlots            Set of all Lambert pork-chop plots
 */
void recurseLambertTransfers( const int                currentSequencePosition,
                              const TleObjects&        tleObjects,
                              const AllEpochs&         allEpochs,
                              const bool               isPrograde,
                              const int                revolutionsMaximum,
                              Sequence&                sequence,
                              int&                     transferId,
                              AllLambertPorkChopPlots& allPorkChopPlots );

//! Compute Lambert pork-chop plot.
/*!
 * Computes a Lambert pork-chop plot, given TLE departure and arrival objects and a list of
 * departure and arrival epoch pairs. The transfers are computed using the Lambert solver
 * implemented in PyKep (Izzo, 2014). The sense (prograde/retrograde) of the transfer and the
 * maximum number of revolutions permitted are specified by the user in the configuration file.
 * The list of departure and arrival epoch pairs are pre-computed. The 2D pork-chop plot grid is
 * flattened into a 1D list with the grid point coordinates specified as a departure-arrival epoch
 * pair.
 *
 * The result is stored as a list of LambertPorkChopPlotGridPoint points, which contain the
 * departure epoch, time-of-flight (departure-arrival epoch) and Lambert transfer data.
 *
 * @sa LambertPorkChopPlotGridPoint, computeAllPorkChopPlotEpochs
 * @param[in] departureObject     Departure TLE object
 * @param[in] arrivalObject       Arrival TLE object
 * @param[in] listOfEpochs        Flattened list of departure-arrival epoch pairs
 * @param[in] isPrograde          Flag indicating if prograde Lambert transfer should be computed
 *                                (false = retrograde)
 * @param[in] revolutionsMaximum  Maximum number of revolutions
 * @param[in]       transferId    Counter for unique ID assigned to transfers
 * @return                        List of LambertPorkChopPlotGridPoint objects
 */
LambertPorkChopPlot computeLambertPorkChopPlot( const Tle&          departureObject,
                                                const Tle&          arrivalObject,
                                                const ListOfEpochs& listOfEpochs,
                                                const bool          isPrograde,
                                                const int           revolutionsMaximum,
                                                      int&          transferId );

//! Recurse through sequence leg-by-leg and compute multi-leg transfers.
/*!
 * Recurses leg-by-leg through a given sequence and computes multi-leg transfers, taking into
 * account matching between the arrival epoch from the previous leg, stay time at target and
 * departure epoch for next leg. All possible multi-leg transfers are computed by traversing the
 * tree of pork-chop plots for each leg.
 *
 * All of the multi-leg transfers for the specified transfer are stored in a list.
 *
 * The multiLegTransferData, launchEpoch and lastArrivalEpoch input parameters are storage variables
 * that are used during the recursion to keep track of values whilst traversing the sequence tree.
 * The multiLegTransferData parameter should be passed to the function without any values. The
 * launchEpoch and lastArrivalEpoch parameters are set to default values and should not be passed
 * by the user.
 *
 * @sa executeLambertScanner, ListOfSequences, AllPorkChopPlots, ListOfMultiLegTransfers
 * @param[in]   currentSequencePosition     Current position along sequence
 * @param[in]   sequence                    Sequence of TLE objects
 * @param[in]   allPorkChopPlots            Collection of all pork-chop plots for all sequences
 * @param[in]   stayTime                    Fixed stay time at each target
 * @param[out]  listOfMultiLegTransfers     List of all multi-leg transfers for given sequence
 * @param[out]  multiLegTransferData        Storage container for multi-leg transfer data (for
 *                                          internal use)
 * @param[in]   launchEpoch                 Storage variable for launch epoch (default: DateTime( ))
 * @param[in]   lastArrivalEpoch            Storage variable for arrival epoch from previous leg
 *                                          (default: DateTime( ))
 */
void recurseMuiltiLegLambertTransfers(  const int                 currentSequencePosition,
                                        const Sequence&           sequence,
                                        AllLambertPorkChopPlots&  allPorkChopPlots,
                                        const double              stayTime,
                                        ListOfMultiLegTransfers&  listOfMultiLegTransfers,
                                        MultiLegTransferData&     multiLegTransferData,
                                        DateTime                  launchEpoch      = DateTime( ),
                                        DateTime                  lastArrivalEpoch = DateTime( ) );

//! Input for lambert_scanner application mode.
/*!
 * Data struct containing all valid lambert_scanner input parameters. This struct is populated by
 * the checkLambertScannerInput() function and can be used to execute the lambert_scanner
 * application mode.
 *
 * @sa checkLambertScannerInput, executeLambertScanner
 */
struct LambertScannerInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkLambertScannerInput, executeLambertScanner
     * @param[in] aCatalogPath             Path to TLE catalog
     * @param[in] aDatabasePath            Path to SQLite database
     * @param[in] aSequenceLength          Sequence length
     * @param[in] aDepartureEpochInitial   Departure epoch grid initial epoch
     * @param[in] someDepartureEpochSteps  Number of steps to take in departure epoch grid
     * @param[in] aDepartureEpochStepSize  Departure epoch grid step size (derived parameter) [s]
     * @param[in] aTimeOfFlightMinimum     Minimum time-of-flight [s]
     * @param[in] aTimeOfFlightMaximum     Maximum time-of-flight [s]
     * @param[in] someTimeOfFlightSteps    Number of steps to take in time-of-flight grid
     * @param[in] aTimeOfFlightStepSize    Time-of-flight step size (derived parameter) [s]
     * @param[in] aStayTime                Fixed stay time at arrival object [s]
     * @param[in] progradeFlag             Flag indicating if prograde transfer should be computed
     *                                     (false = retrograde)
     * @param[in] aRevolutionsMaximum      Maximum number of revolutions
     * @param[in] aShortlistLength         Number of transfers to include in shortlist
     * @param[in] aShortlistPath           Path to shortlist file
     */
    LambertScannerInput( const std::string& aCatalogPath,
                         const std::string& aDatabasePath,
                         const int          aSequenceLength,
                         const DateTime&    aDepartureEpochInitial,
                         const double       someDepartureEpochSteps,
                         const double       aDepartureEpochStepSize,
                         const double       aTimeOfFlightMinimum,
                         const double       aTimeOfFlightMaximum,
                         const double       someTimeOfFlightSteps,
                         const double       aTimeOfFlightStepSize,
                         const double       aStayTime,
                         const bool         progradeFlag,
                         const int          aRevolutionsMaximum,
                         const int          aShortlistLength,
                         const std::string& aShortlistPath )
        : catalogPath( aCatalogPath ),
          databasePath( aDatabasePath ),
          sequenceLength( aSequenceLength ),
          departureEpochInitial( aDepartureEpochInitial ),
          departureEpochSteps( someDepartureEpochSteps ),
          departureEpochStepSize( aDepartureEpochStepSize ),
          timeOfFlightMinimum( aTimeOfFlightMinimum ),
          timeOfFlightMaximum( aTimeOfFlightMaximum ),
          timeOfFlightSteps( someTimeOfFlightSteps ),
          timeOfFlightStepSize( aTimeOfFlightStepSize ),
          stayTime( aStayTime ),
          isPrograde( progradeFlag ),
          revolutionsMaximum( aRevolutionsMaximum ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Path to TLE catalog.
    const std::string catalogPath;

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Length of sequence (numberOfLegs = sequenceLength - 1).
    const int sequenceLength;

    //! Initial departure epoch.
    const DateTime departureEpochInitial;

    //! Number of departure epoch steps.
    const double departureEpochSteps;

    //! Departure epoch grid step size.
    const double departureEpochStepSize;

    //! Minimum time-of-flight [s].
    const double timeOfFlightMinimum;

    //! Maximum time-of-flight [s].
    const double timeOfFlightMaximum;

    //! Number of time-of-flight steps.
    const double timeOfFlightSteps;

    //! Time-of-flight step size [s].
    const double timeOfFlightStepSize;

    //! Fixed stay time at arrival object, i.e., departure for next leg is delayed by stay time.
    const double stayTime;

    //! Flag indicating if transfers are prograde. False indicates retrograde.
    const bool isPrograde;

    //! Maximum number of revolutions (N) for transfer. Number of revolutions is 2*N+1.
    const int revolutionsMaximum;

    //! Number of entries (lowest transfer \f$\Delta V\f$) to include in shortlist.
    const int shortlistLength;

    //! Path to shortlist file.
    const std::string shortlistPath;

protected:

private:
};

//! Grid point in pork-chop plot.
/*!
 * Data struct containing all data pertaining to a grid point in a pork-chop plot, i.e., departure
 * epoch, time-of-flight and all data related to the computed transfer.
 *
 * @sa recurseTransfers
 */
struct LambertPorkChopPlotGridPoint
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on departure epoch, time-of-flight and transfer data for grid
     * point in pork-chop plot.
     *
     * @param[in] aTransferId           A unique transfer ID
     * @param[in] aDepartureEpoch       A departure epoch corresponding to a grid point
     * @param[in] anArrivalEpoch        An arrival epoch corresponding to a grid point
     * @param[in] aTimeOfFlight         A time-of-flight (arrival-departure epoch) for a grid point
     * @param[in] someRevolutions       Number of revolutions for transfer
     * @param[in] progradeFlag          Flag indicating if transfer is prograde (false = retrograde)
     * @param[in] aDepartureState       Departure state corresponding to a grid point
     * @param[in] aDepartureStateKepler Departure state in Keplerian elements corresponding to a
     *                                  grid point
     * @param[in] anArrivalState        Arrival state corresponding to a grid point
     * @param[in] anArrivalStateKepler  Arrival state in Keplerian elements corresponding to a grid
     *                                  point
     * @param[in] aTransferStateKepler  Transfer state in Keplerian elements corresponding to a grid
     *                                  point
     * @param[in] aDepartureDeltaV      Computed departure \f$\Delta V\f$
     * @param[in] anArrivalDeltaV       Computed arrival \f$\Delta V\f$
     * @param[in] aTransferDeltaV       Total computed transfer \f$\Delta V\f$
     */
    LambertPorkChopPlotGridPoint( const int       aTransferId,
                                  const DateTime& aDepartureEpoch,
                                  const DateTime& anArrivalEpoch,
                                  const double    aTimeOfFlight,
                                  const int       someRevolutions,
                                  const bool      progradeFlag,
                                  const Vector6&  aDepartureState,
                                  const Vector6&  aDepartureStateKepler,
                                  const Vector6&  anArrivalState,
                                  const Vector6&  anArrivalStateKepler,
                                  const Vector6&  aTransferStateKepler,
                                  const Vector3&  aDepartureDeltaV,
                                  const Vector3&  anArrivalDeltaV,
                                  const double    aTransferDeltaV )
        : transferId( aTransferId ),
          departureEpoch( aDepartureEpoch ),
          arrivalEpoch( anArrivalEpoch ),
          timeOfFlight( aTimeOfFlight ),
          revolutions( someRevolutions ),
          isPrograde( progradeFlag ),
          departureState( aDepartureState ),
          departureStateKepler( aDepartureStateKepler ),
          arrivalState( anArrivalState ),
          arrivalStateKepler( anArrivalStateKepler ),
          transferStateKepler( aTransferStateKepler ),
          departureDeltaV( aDepartureDeltaV ),
          arrivalDeltaV( anArrivalDeltaV ),
          transferDeltaV( aTransferDeltaV )
    { }

    //! Overload operator-=.
    /*!
     * Overloads operator-= to assign current object to object provided as input.
     *
     * WARNING: this is a dummy overload to get by the problem of adding a
     *          LambertPorkChopPlotGridPoint object to a STL container! It does not correctly assign
     *          the current object to the dummy grid point provided!
     *
     * @sa executeLambertScanner
     * @param[in] dummyGridPoint Dummy grid point that is ignored
     * @return                   The current object
     */
    LambertPorkChopPlotGridPoint& operator=( const LambertPorkChopPlotGridPoint& dummyGridPoint )
    {
        return *this;
    }

    //! Unique transfer ID.
    const int transferId;

    //! Departure epoch.
    const DateTime departureEpoch;

    //! Arrival epoch.
    const DateTime arrivalEpoch;

    //! Time of flight [s].
    const double timeOfFlight;

    //! Number of revolutions (N) for transfer.
    const int revolutions;

    //! Flag indicating if transfers are prograde. False indicates retrograde.
    const bool isPrograde;

    //! Departure state [km; km/s].
    const Vector6 departureState;

    //! Departure state in Keplerian elements [km; -; rad; rad; rad; rad].
    const Vector6 departureStateKepler;

    //! Arrival state [km; km/s].
    const Vector6 arrivalState;

    //! Arrival state in Keplerian elements [km; -; rad; rad; rad; rad].
    const Vector6 arrivalStateKepler;

    //! Transfer state in Keplerian elements [km; -; rad; rad; rad; rad].
    const Vector6 transferStateKepler;

    //! Departure \f$\Delta V\f$ [km/s].
    const Vector3 departureDeltaV;

    //! Arrival \f$\Delta V\f$ [km/s].
    const Vector3 arrivalDeltaV;

    //! Total transfer \f$\Delta V\f$ [km/s].
    const double transferDeltaV;

protected:
private:
};

} // namespace d2d

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in
 * 	astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth
 *  International Conf. Astrodynam. Tools and Techniques, ESA/ESTEC, The Netherlands.
 */

#endif // D2D_LAMBERT_SCANNER_HPP
