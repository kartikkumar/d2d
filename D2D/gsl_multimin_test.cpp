#include <gsl/gsl_multimin.h>
#include <gsl/gsl_vector.h>

double my_f(const gsl_vector *v, void *params)
{
  double x, y;
  double *p = static_cast< double* >( params );
  
  x = gsl_vector_get( v, 0 );
  y = gsl_vector_get( v, 1 );
 
  return p[2] * (x - p[0]) * (x - p[0]) +
           p[3] * (y - p[1]) * (y - p[1]) + p[4]; 
}


int  main( )
{
  double parameters[5] = {1.0, 2.0, 10.0, 20.0, 30.0};

  const gsl_multimin_fminimizer_type* optimizerType = gsl_multimin_fminimizer_nmsimplex2;
  gsl_multimin_fminimizer* optimizer = NULL;
  gsl_vector* stepSizes;
  gsl_vector* x;
  gsl_multimin_function functionToMinimize;

  int iter = 0;
  int status;
  double size;

  /* Starting point */
  x = gsl_vector_alloc( 2 );
  gsl_vector_set( x, 0, 5.0 );
  gsl_vector_set( x, 1, 7.0 );

  /* Set initial step sizes to 1 */
  stepSizes = gsl_vector_alloc( 2 );
  gsl_vector_set_all( stepSizes, 1.0 );

  /* Initialize method and iterate */
  functionToMinimize.n = 2;
  functionToMinimize.f = my_f;
  functionToMinimize.params = parameters;

  optimizer = gsl_multimin_fminimizer_alloc( optimizerType, 2 );
  gsl_multimin_fminimizer_set( optimizer, &functionToMinimize, x, stepSizes );

  do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate( optimizer );
      
      if (status) 
        break;

      size = gsl_multimin_fminimizer_size( optimizer );
      status = gsl_multimin_test_size( size, 1.0e-2 );

      if (status == GSL_SUCCESS)
        {
          printf("converged to minimum at\n");
        }

      printf("%5d %10.3e %10.3e f() = %7.3f size = %.3f\n", 
             iter,
             gsl_vector_get( optimizer->x, 0 ), 
             gsl_vector_get( optimizer->x, 1 ), 
             optimizer->fval, size );
    }
  while (status == GSL_CONTINUE && iter < 100);
  
  gsl_vector_free( x );
  gsl_vector_free( stepSizes);
  gsl_multimin_fminimizer_free( optimizer );

  return status;
}