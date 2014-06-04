 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iomanip>
#include <iostream>

#include <gsl/gsl_multiroots.h>
  
namespace d2d
{

template< typename DataType > 
void printElement( DataType datum, const int width, const char separator )
{
    std::cout << std::left << std::setw( width ) << std::setfill( separator ) << datum;
}

void printSolverStateTableHeader( )
{
	printElement( "#", 5, ' ' );
	printElement( "a", 12, ' ' );
	printElement( "e", 12, ' ' );
	printElement( "i", 12, ' ' );
	printElement( "AoP", 12, ' ' );
	printElement( "RAAN", 12, ' ' );
	printElement( "TA", 12, ' ' );
	printElement( "f1", 12, ' ' );
	printElement( "f2", 12, ' ' );
	printElement( "f3", 12, ' ' );
	printElement( "f4", 12, ' ' );
	printElement( "f5", 12, ' ' );
	printElement( "f6", 12, ' ' );
	std::cout << std::endl;
}

void printSolverState( int iteration, gsl_multiroot_fsolver* solver )
{
	printElement( iteration, 5, ' ' );
	printElement( gsl_vector_get( solver->x, 0 ), 12, ' ' );
	printElement( gsl_vector_get( solver->x, 1 ), 12, ' ' );
	printElement( gsl_vector_get( solver->x, 2 ), 12, ' ' );
	printElement( gsl_vector_get( solver->x, 3 ), 12, ' ' );
	printElement( gsl_vector_get( solver->x, 4 ), 12, ' ' );
	printElement( gsl_vector_get( solver->x, 5 ), 12, ' ' );
	printElement( gsl_vector_get( solver->f, 0 ), 12, ' ' );
	printElement( gsl_vector_get( solver->f, 1 ), 12, ' ' );
	printElement( gsl_vector_get( solver->f, 2 ), 12, ' ' );
	printElement( gsl_vector_get( solver->f, 3 ), 12, ' ' );
	printElement( gsl_vector_get( solver->f, 4 ), 12, ' ' );
	printElement( gsl_vector_get( solver->f, 5 ), 12, ' ' );
	std::cout << std::endl;
}

} // namespace d2d
