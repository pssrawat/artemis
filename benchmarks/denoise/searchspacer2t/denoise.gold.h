#include "common/common.hpp"
#define epsilon (1.0e-20)

void denoise_step (double* h_u0, double *h_u, double *h_f, double *h_g, int N) {
  double (*u)[N][N] = (double (*)[N][N])h_u;
  double (*u0)[N][N] = (double (*)[N][N])h_u0;
  double (*f)[N][N] = (double (*)[N][N])h_f;
  double (*g)[N][N] = (double (*)[N][N])h_g;
  double sigma2 = 0.05*0.05;
  double gamma = 0.065/sigma2;

#pragma omp parallel for
  for (int i = 1; i < N-1; i++)
    for (int j = 1; j < N-1; j++)
#pragma GCC ivdep
      for (int k = 1; k < N-1; k++) {
          g[i][j][k] = 1.0/sqrt (epsilon + (u0[i][j][k] - u0[i][j+1][k])*(u0[i][j][k] - u0[i][j+1][k]) + (u0[i][j][k] - u0[i][j-1][k])*(u0[i][j][k] - u0[i][j-1][k]) + (u0[i][j][k] - u0[i][j][k+1])*(u0[i][j][k] - u0[i][j][k+1]) + (u0[i][j][k] - u0[i][j][k-1])*(u0[i][j][k] - u0[i][j][k-1]) + (u0[i][j][k] - u0[i+1][j][k])*(u0[i][j][k] - u0[i+1][j][k]) + (u0[i][j][k] - u0[i-1][j][k])*(u0[i][j][k] - u0[i-1][j][k]));
      }

#pragma omp parallel for
  for (int i = 1; i < N-1; i++) 
    for (int j = 1; j < N-1; j++)
      for (int k = 1; k < N-1; k++) {
          double r = u0[i][j][k]*f[i][j][k]/sigma2;
          r = (r*(2.38944 + r*(0.950037 + r))) / (4.65314 + r*(2.57541 + r*(1.48937 + r)));
          /* Update U */
          u[i][j][k] = (u0[i][j][k] + 5.0*(u0[i][j+1][k]*g[i][j+1][k] + u0[i][j-1][k]*g[i][j-1][k] + u0[i][j][k+1]*g[i][j][k+1] + u0[i][j][k-1]*g[i][j][k-1] + u0[i+1][j][k]*g[i+1][j][k] + u0[i-1][j][k]*g[i-1][j][k] + gamma*f[i][j][k]*r)) / (1.0 + 5.0*(g[i][j+1][k] + g[i][j-1][k] + g[i][j][k+1] + g[i][j][k-1] + g[i+1][j][k] + g[i-1][j][k] + gamma));
     }
}


extern "C" void denoise_gold (double *u, double *u0, double *f, int N) {
  double* g = getZero3DArray<double>(N, N, N);
  denoise_step(u0, u, f, g, N);
  delete[] g;
}
