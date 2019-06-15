#include "common/common.hpp"
#include "common/timer.hpp"
#include "cheby.gold.h"
#include <cassert>
#include <cstdio>
#include <fstream>

#define TIMESTEPS 8 

extern "C" void host_code (double *, double *, double *, double *, double *, double, double, double, int, int, int);

int main(int argc, char** argv) {
  int N = 512;

  double (*out)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);
  double (*out_gold)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);
  double (*Ac)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double (*Ap)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double (*RHS)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double (*Dinv)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double h2inv = 0.625;
  double c1 = 1.137;
  double c2 = 0.163;

  cheby_gold((double*) out_gold, (double*) Ac, (double *)Ap, (double *)RHS, (double *)Dinv, h2inv, c1, c2, N);
  host_code((double*) Ac, (double*) Ap, (double*)out, (double *)Dinv, (double *)RHS, c1, c2, h2inv, N, N, N);

  double error = checkError3D<double> (N, N, (double*)out, (double*) out_gold, TIMESTEPS, N-TIMESTEPS, TIMESTEPS, N-TIMESTEPS, TIMESTEPS, N-TIMESTEPS);
  printf("[Test] RMS Error : %e\n",error);
  if (error > TOLERANCE)
     return -1;
}
