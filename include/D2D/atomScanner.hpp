/*
 * Copyright (c) 2014-2015 Kartik Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_ATOM_SCANNER_HPP
#define D2D_ATOM_SCANNER_HPP

#include <string>

#include <keplerian_toolbox.h>

#include <libsgp4/DateTime.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

//! Execute atom_scanner.
/*!
 * Executes atom_scanner application mode that performs a grid search to compute \f$\Delta V\f$
 * for debris-to-debris transfers. The transfers are modelled as conic sections. The Lambert
 * targeter employed is based on Izzo (2014), implemented in PyKEP (Izzo, 2012).
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
     * @sa checkSGP4ScannerInput, executeSGP4Scanner
     * @param[in] aRelativeTolerance      Relative tolerance for Cartesian-to-TLE conversion
     * @param[in] aAbsoluteTolerance      Absolute tolerance for the Cartesian-to-TLE conversion
     * @param[in] aDatabasePath           Path to SQLite database
     * @param[in] aMaximumOfIterations    Maximum of iterations the atom solver can do
     * @param[in] aShortlistLength        Number of transfers to include in shortlist
     * @param[in] aShortlistPath          Path to shortlist file
     */
    AtomScannerInput( const double       aRelativeTolerance,
                      const double       aAbsoluteTolerance,
                      const std::string& aDatabasePath,
                      const int          aMaximumOfIterations,
                      const int          aShortlistLength,
                      const std::string& aShortlistPath )
        : relativeTolerance( aRelativeTolerance ),
          absoluteTolerance( aAbsoluteTolerance ),
          maxIterations( aMaximumOfIterations ),
          databasePath( aDatabasePath ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Relative tolerance for Cartesian-to-TLE conversion function.
    const double relativeTolerance;

    //! Absolute tolerance for Cartesian-to-TLE conversion function.
    const double absoluteTolerance;

    //! Maximum of iterations for Atoms solver
    const int maxIterations;

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Number of entries (lowest Lambert transfer \f$\Delta V\f$) to include in shortlist.
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
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute atom_scanner
 */
AtomScannerInput checkAtomScannerInput( const rapidjson::Document& config );

//! Create atom_scanner table.
/*!
 * Creates atom_scanner table in SQLite database used to store results obtaned from running
 * the atom_scanner application mode.
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

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in
 * 	astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth
 *  International Conf. Astrodynam. Tools and Techniques, ESA/ESTEC, The Netherlands.
 */

#endif // D2D_ATOM_SCANNER_HPP
