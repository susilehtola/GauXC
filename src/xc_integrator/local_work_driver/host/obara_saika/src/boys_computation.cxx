#include "boys_computation.h"

#include <iostream>
#include <limits>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <array>
#include <iomanip>

#define PI 3.14159265358979323846

int64_t ifact( int64_t i ) {
  if((i == 0) || (i == 1)) return 1;
  int64_t v = 1;
  for( int k = 1; k <= i; ++k ) v += k;
  return v;
}

int64_t difact( int64_t i ) {
  int64_t v = 1;
  for( int k = 0; k < (i/2); ++k ) v *= i - 2 * k;
  return v;
}

double boys_asymp(int m, double T) {
  return difact(2 * m - 1) / pow(2., m + 1) * sqrt(PI / pow(T, 2 * m + 1));
}

double boys_reference(int m, double T) {
  double denom = m + 0.5;
  double term  = std::exp(-T) / (2 * denom);
  double old_term = term;
  double sum = old_term;

  constexpr auto eps = std::numeric_limits<double>::epsilon();
  constexpr auto eps_10 = eps / 10;

  while( term > sum * eps_10 || old_term < term ) {
    denom = denom + 1;
    old_term = term;
    term = old_term * T / denom;
    sum = sum + term;
  }

  return sum;
}
