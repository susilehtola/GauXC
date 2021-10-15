#ifndef __MY_INTEGRAL_2_0
#define __MY_INTEGRAL_2_0

#include "integral_2_0.h"

void integral_2_0(int npts,
                  shells shellA,
                  shells shellB,
                  point *points,
                  double *Xi,
                  double *Xj,
                  int stX,
                  int ldX,
                  double *Gi,
                  double *Gj,
                  int stG, 
                  int ldG, 
                  double *weights);

#endif
