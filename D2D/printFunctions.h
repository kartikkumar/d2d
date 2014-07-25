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
	printElement( "#", 3, ' ' );
	printElement( "a", 15, ' ' );
	printElement( "e", 15, ' ' );
	printElement( "i", 15, ' ' );
	printElement( "AoP", 15, ' ' );
	printElement( "RAAN", 15, ' ' );
	printElement( "TA", 15, ' ' );
	printElement( "f1", 15, ' ' );
	printElement( "f2", 15, ' ' );
	printElement( "f3", 15, ' ' );
	printElement( "f4", 15, ' ' );
	printElement( "f5", 15, ' ' );
	printElement( "f6", 15, ' ' );
	std::cout << std::endl;
}

void printSolverState( int iteration, gsl_multiroot_fsolver* solver )
{
	printElement( iteration, 3, ' ' );
	printElement( gsl_vector_get( solver->x, 0 ), 15, ' ' );
	printElement( gsl_vector_get( solver->x, 1 ), 15, ' ' );
	printElement( gsl_vector_get( solver->x, 2 ), 15, ' ' );
	printElement( gsl_vector_get( solver->x, 3 ), 15, ' ' );
	printElement( gsl_vector_get( solver->x, 4 ), 15, ' ' );
	printElement( gsl_vector_get( solver->x, 5 ), 15, ' ' );
	printElement( gsl_vector_get( solver->f, 0 ), 15, ' ' );
	printElement( gsl_vector_get( solver->f, 1 ), 15, ' ' );
	printElement( gsl_vector_get( solver->f, 2 ), 15, ' ' );
	printElement( gsl_vector_get( solver->f, 3 ), 15, ' ' );
	printElement( gsl_vector_get( solver->f, 4 ), 15, ' ' );
	printElement( gsl_vector_get( solver->f, 5 ), 15, ' ' );
	std::cout << std::endl;
}

} // namespace d2d
