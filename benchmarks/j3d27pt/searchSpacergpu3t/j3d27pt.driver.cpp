#include "common/common.hpp"
#include "common/timer.hpp"
#include "j3d27pt.gold.h"
#include <cassert>
#include <cstdio>
#include <fstream>

#define TIMESTEPS 6 

extern "C" void host_code (double *h_in, double *h_out, double, double, double, int L, int M, int N);

int main(int argc, char** argv) {
  int N = 512;

  double (*out)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);
  double (*out_gold)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);
  double (*tmp)[N][N] = (double (*)[N][N]) getZero3DArray<double>(N, N, N);
  double (*in)[N][N] = (double (*)[N][N]) getRandom3DArray<double>(N, N, N);
  double h2inv = 0.625;
  double a = 1.137;
  double b = 0.163;

  std::ofstream test("test.txt", std::ofstream::binary);
  test.write((char *) in, sizeof(double) * N * N * N);
  test.close();

  jacobi_gold((double*) out_gold, (double*) in, h2inv, a, b, N, N, N);
  std::ofstream gold("gold.txt", std::ofstream::binary);
  gold.write((char *) out_gold, sizeof(double) * N * N * N);
  gold.close();

  host_code((double*) in, (double*) out, a, b, h2inv, N, N, N);

  double error = checkError3D<double> (N, N, (double*)in, (double*) out_gold, TIMESTEPS, N-TIMESTEPS, TIMESTEPS, N-TIMESTEPS, TIMESTEPS, N-TIMESTEPS);
  printf("[Test] RMS Error : %e\n",error);
  if (error > TOLERANCE)
     return -1;
}
