 /*
  * Copyright (c) 2014, Dinamica Srl
  * Copyright (c) 2014, K. Kumar (kumar@dinamicatech.com)
  * All rights reserved.
  * See COPYING for license details.
  */

#include <gsl/gsl_vector.h>
  
namespace d2d
{

struct modifiedTLEGeneratorParameters 
{ 
    double temp; 
};

int modifiedTLEGenerator( const gsl_vector* unknowns, void* parameters, gsl_vector* function )
{
    return GSL_SUCCESS;
}

} // namespace d2d
