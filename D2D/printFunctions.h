 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <iostream>

#include <gsl/gsl_multiroots.h>
  
namespace d2d
{

void printSolverStateTableHeader( )
{
      std::cout << "#" << "    a    e    i    AoP  RAAN TA " << std::endl;
}

void printSolverState( size_t iteration, gsl_multiroot_fsolver* solver )
{
  std::cout << iteration << "    "
            << gsl_vector_get( solver->x, 0 ) << "    "
            << gsl_vector_get( solver->x, 1 ) << "    "
            << gsl_vector_get( solver->x, 2 ) << "    "
            << gsl_vector_get( solver->x, 3 ) << "    "
            << gsl_vector_get( solver->x, 4 ) << "    "
            << gsl_vector_get( solver->x, 5 ) << "    "
            << gsl_vector_get( solver->f, 0 ) << "    "
            << gsl_vector_get( solver->f, 1 ) << "    "
            << gsl_vector_get( solver->f, 2 ) << "    "
            << gsl_vector_get( solver->f, 3 ) << "    "
            << gsl_vector_get( solver->f, 4 ) << "    "
            << gsl_vector_get( solver->f, 5 )
            << std::endl;
}

} // namespace d2d
