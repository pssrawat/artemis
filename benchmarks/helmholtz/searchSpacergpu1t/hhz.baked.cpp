#include "common/common.hpp"
#include "common/timer.hpp"
#include "hhz.gold.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
using std::cout;

#define TIMESTEPS 6 
#define ORDER 2

extern "C" void host_code (double *h_in, double *h_out, double *h_t, double a, double b, double h2inv, int L, int M, int N);

int main(int argc, char** argv) {
  int N = 512;
  double *out = getZero3DArray<double>(N, N, N);
  double *t = getZero3DArray<double>(N, N, N);
  double a = 0.12, b = 0.1, h2inv = 0.4;

  double *in_dev = new double[N * N * N];
  std::ifstream infile ("test.txt",std::ofstream::binary);
  infile.read((char *) in_dev, N * N * N * sizeof(double));
  infile.close();

  double *in = new double[N * N * N];
  std::ifstream goldfile ("gold.txt",std::ofstream::binary);
  goldfile.read((char *) in, N * N * N * sizeof(double));
  goldfile.close();

  host_code(in_dev, out, t, a, b, h2inv, N, N, N);

  double error = checkError3D<double> (N, N, (double*)in, (double*) in_dev, TIMESTEPS * ORDER, N-TIMESTEPS * ORDER, TIMESTEPS * ORDER, N-TIMESTEPS * ORDER, TIMESTEPS * ORDER, N-TIMESTEPS * ORDER);

  printf("[Test] RMS Error : %e\n",error);
  if (error > TOLERANCE) {
    return -1;
  }
}
