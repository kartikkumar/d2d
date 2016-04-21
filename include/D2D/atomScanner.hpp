/*
 * Copyright (c) 2014-2015 Kartik Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_ATOM_SCANNER_HPP
#define D2D_ATOM_SCANNER_HPP

#include <string>

#include <libsgp4/DateTime.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <keplerian_toolbox.h>

namespace d2d
{

//! Execute atom_scanner.
/*!
 * Executes atom_scanner application mode that performs a grid search to compute \f$\Delta V\f$
 * for debris-to-debris transfers. To compute transfer trajectories, the atom scanner makes use of
 * an initial guess from the Lambert targeter developed by Izzo (2014), implemented in
 * PyKEP (Izzo, 2012).
 *
 * The results obtained from the grid search are stored in a SQLite database, containing the
 * following table:
 *
 *	- "atom_scanner_results": contains all Atom transfers computed during grid search
 *
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeAtomScanner( const rapidjson::Document& config );

//! atom_scanner input.
/*!
 * Data struct containing all valid atom_scanner input parameters. This struct is populated by
 * the checkAtomScannerInput() function and can be used to execute the atom_scanner
 * application mode.
 *
 * @sa checkAtomScannerInput, executeAtomScanner
 */
struct AtomScannerInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkAtomScannerInput, executeAtomScanner
     * @param[in] aRelativeTolerance      Relative tolerance for the atom solver and the
     *                                    Cartesian-to-TLE convertor
     * @param[in] anAbsoluteTolerance     Absolute tolerance for the atom solver and the
     *                                    Cartesian-to-TLE convertor
     * @param[in] aDatabasePath           Path to SQLite database
     * @param[in] aMaximumOfIterations    Maximum number of iterations for the Atom solver
     * @param[in] aShortlistLength        Number of transfers to include in shortlist
     * @param[in] aShortlistPath          Path to shortlist file
     */
    AtomScannerInput( const double       aRelativeTolerance,
                      const double       anAbsoluteTolerance,
                      const std::string& aDatabasePath,
                      const int          aMaximumOfIterations,
                      const int          aShortlistLength,
                      const std::string& aShortlistPath )
        : relativeTolerance( aRelativeTolerance ),
          absoluteTolerance( anAbsoluteTolerance ),
          maxIterations( aMaximumOfIterations ),
          databasePath( aDatabasePath ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Relative tolerance for the atom solver and the Cartesian-to-TLE conversion function.
    const double relativeTolerance;

    //! Absolute tolerance for the atom solver and the Cartesian-to-TLE conversion function.
    const double absoluteTolerance;

    //! Maximum number of iterations for the Atom solver.
    const int maxIterations;

    //! Path to SQLite database to store the Atom scanner results.
    const std::string databasePath;

    //! Number of entries (lowest Atom transfer \f$\Delta V\f$) to include in shortlist.
    const int shortlistLength;

    //! Path to shortlist file.
    const std::string shortlistPath;

protected:

private:
};

//! Check atom_scanner input parameters.
/*!
 * Checks that all inputs for the atom_scanner application mode are valid. If not, an error is
 * thrown with a short description of the problem. If all inputs are valid, a data struct
 * containing all the inputs is returned, which is subsequently used to execute atom_scanner
 * and related functions.
 *
 * @sa executeAtomScanner, AtomScannerInput
 * @param[in]   config  User-defined configuration options (extracted from JSON input file)
 * @return              Struct containing all valid input to execute atom_scanner
 */
AtomScannerInput checkAtomScannerInput( const rapidjson::Document& config );

//! Create atom_scanner table.
/*!
 * Creates "atom_scanner_results" table in SQLite database. The table is used to store results
 * obtained from running the atom_scanner application mode.
 *
 * @sa executeAtomScanner
 * @param[in] database SQLite database handle
 */
void createAtomScannerTable( SQLite::Database& database );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of debris-to-debris Atom transfers to file. The shortlist is based on the
 * requested number of transfers with the lowest transfer \f$\Delta V\f$, retrieved by sorting the
 * transfers in the SQLite database.
 *
 * @sa executeAtomScanner, createAtomScannerTable
 * @param[in] database        SQLite database handle
 * @param[in] shortlistNumber Number of entries to include in shortlist (if it exceeds number of
 *                            entries in database table, the whole table is written to file)
 * @param[in] shortlistPath   Path to shortlist file
 */
void writeAtomTransferShortlist( SQLite::Database& database,
                                 const int shortlistNumber,
                                 const std::string& shortlistPath );

} // namespace d2d

#endif // D2D_ATOM_SCANNER_HPP

/*! References:
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in
 *  astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth
 *  International Conf. Astrodynam. Tools and Techniques, ESA/ESTEC, The Netherlands.
 */
