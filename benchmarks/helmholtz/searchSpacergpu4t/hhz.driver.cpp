#include "common/common.hpp"
#include "common/timer.hpp"
#include "hhz.gold.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
using std::cout;

#define TIMESTEPS 8 
#define ORDER 2

extern "C" void host_code (double *h_in, double *h_out, double *h_t, double a, double b, double h2inv, int L, int M, int N);

int main(int argc, char** argv) {
  int N = 512;

  double *in = getRandom3DArray<double>(N, N, N);
  double *in_dev = new double[N * N * N];
  memcpy(in_dev, in, sizeof(double) * N * N * N);
  std::ofstream test("test.txt", std::ofstream::binary);
  test.write((char *) in, sizeof(double) * N * N * N);
  test.close();

  double *out = getZero3DArray<double>(N, N, N);
  double *out_dev = getZero3DArray<double>(N, N, N);

  double *t = getZero3DArray<double>(N, N, N);
  double a = 0.12, b = 0.1, h2inv = 0.4;

  host_code(in_dev, out_dev, t, a, b, h2inv, N, N, N);

  hhz_gold(out, in, h2inv, a, b, N, TIMESTEPS);

  std::ofstream gold("gold.txt", std::ofstream::binary);
  gold.write((char *) in, sizeof(double) * N * N * N);
  gold.close();

  double error = checkError3D<double> (N, N, (double*)in, (double*) in_dev,
				       TIMESTEPS * ORDER, N-TIMESTEPS * ORDER, TIMESTEPS * ORDER, N-TIMESTEPS * ORDER, TIMESTEPS * ORDER, N-TIMESTEPS * ORDER);

  printf("[Test] RMS Error : %e\n",error);
  if (error > TOLERANCE) {
    return -1;
  }
}
