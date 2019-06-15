#include <cstdio>
#include <cstring>

void jacobi_gold(double *fout, double *ftemp, const double *fin, double h2inv, double a, double b, int L, int M, int N) {
	double (*out)[M][N] = (double (*)[M][N]) fout;
	double (*in)[M][N] = (double (*)[M][N]) fin;
	double (*temp)[M][N] = (double (*)[M][N]) ftemp;
	double c = b * h2inv;
	double d = c * 0.5;
	double e = c * 0.125;
	double f = c * 0.3;

#pragma omp parallel for
	for (int k = 1; k < L - 1; ++k) {
		for (int j = 1; j < M - 1; ++j) {
			for (int i = 1; i < N - 1; ++i) {
				temp[k][j][i] = a*in[k][j][i] -
					d*(in[k-1][j-1][i-1] +
							in[k-1][j-1][i+1] +
							in[k-1][j+1][i-1] +
							in[k-1][j+1][i+1] +
							in[k+1][j-1][i-1] +
							in[k+1][j-1][i+1] +
							in[k+1][j+1][i-1] +
							in[k+1][j+1][i+1]) +
					e*(in[k-1][j-1][i] +
							in[k-1][j][i-1] +
							in[k-1][j][i+1] +
							in[k-1][j+1][i] +
							in[k][j-1][i-1] +
							in[k][j-1][i+1] +
							in[k][j+1][i-1] +
							in[k][j+1][i+1] +
							in[k+1][j-1][i] +
							in[k+1][j][i-1] +
							in[k+1][j][i+1] +
							in[k+1][j+1][i]) +
					f*(in[k-1][j][i] +
							in[k][j-1][i] +
							in[k][j][i-1] +
							in[k][j][i+1] +
							in[k][j+1][i] +
							in[k+1][j][i]) +
					0.13*in[k][j][i];
			}
		}
	}


#pragma omp parallel for
	for (int k = 1; k < L - 1; ++k) {
		for (int j = 1; j < M - 1; ++j) {
			for (int i = 1; i < N - 1; ++i) {
				out[k][j][i] = a*temp[k][j][i] -
					d*(temp[k-1][j-1][i-1] +               
							temp[k-1][j-1][i+1] +
							temp[k-1][j+1][i-1] +       
							temp[k-1][j+1][i+1] +               
							temp[k+1][j-1][i-1] +               
							temp[k+1][j-1][i+1] +               
							temp[k+1][j+1][i-1] +               
							temp[k+1][j+1][i+1]) +              
					e*(temp[k-1][j-1][i] +                 
							temp[k-1][j][i-1] + 
							temp[k-1][j][i+1] +
							temp[k-1][j+1][i] +
							temp[k][j-1][i-1] +
							temp[k][j-1][i+1] +
							temp[k][j+1][i-1] +
							temp[k][j+1][i+1] +
							temp[k+1][j-1][i] +
							temp[k+1][j][i-1] +
							temp[k+1][j][i+1] +
							temp[k+1][j+1][i]) +
					f*(temp[k-1][j][i] +
							temp[k][j-1][i] +
							temp[k][j][i-1] +
							temp[k][j][i+1] +
							temp[k][j+1][i] +
							temp[k+1][j][i]) +
					0.13*temp[k][j][i];
			}
		}
	}
}
