#include "common/common.hpp"
#include "denoise.gold.h"
#include <cassert>
#include <cstdio>

extern "C" void host_code (double *, double *, double *, int, int, int);

int main(int argc, char** argv) {
  int N = 512;

  double (*u0)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double (*f)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double (*u_gold)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);
  double (*u_opt)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);

  denoise_gold((double*)u_gold, (double*)u0, (double*)f, N);

  host_code ((double*)u_opt, (double*)f, (double*)u0, N, N, N);

  double error = checkError3D<double> (N, N, (double*)u_opt, (double*)u_gold, 4, N-4, 4, N-4, 4, N-4);
  printf("[Test] RMS Error : %e\n",error);
  if (error > TOLERANCE)
    return -1;

  delete[] u0;
  delete[] u_gold;
  delete[] u_opt;
  delete[] f;
}
