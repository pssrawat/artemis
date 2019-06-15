#include <cstring>
using std::memcpy;

template <class T>
void jacobi_gold(T *fout, const T *fin, double h2inv, double a, double b, int L, int M, int N) {
	double (*out)[M][N] = (double (*)[M][N]) fout;
	double (*in)[M][N] = (double (*)[M][N]) fin;
	auto ftemp1 = new T[L * M * N];
	auto ftemp2 = new T[L * M * N];
	memset(ftemp1, 0, sizeof(T)*L*M*N);
	memset(ftemp2, 0, sizeof(T)*L*M*N);
	double (*temp1)[M][N] = (T (*)[M][N]) ftemp1;
	double (*temp2)[M][N] = (T (*)[M][N]) ftemp2;
	memcpy(ftemp1, fin, sizeof(T)*L*M*N);
	double c = b * h2inv;
	double d = c * 0.5;
	double e = c * 0.125;
	double f = c * 0.3;

	for (int t = 0; t < 10; t++) {
#pragma omp parallel for
		for (int k = 1; k < L - 1; ++k) {
			for (int j = 1; j < M - 1; ++j) {
				for (int i = 1; i < N - 1; ++i) {
					if (!(t%2)) {
						temp2[k][j][i] = a*temp1[k][j][i] -
							d*(temp1[k-1][j-1][i-1] +
									temp1[k-1][j-1][i+1] +
									temp1[k-1][j+1][i-1] +
									temp1[k-1][j+1][i+1] +
									temp1[k+1][j-1][i-1] +
									temp1[k+1][j-1][i+1] +
									temp1[k+1][j+1][i-1] +
									temp1[k+1][j+1][i+1]) +
							e*(temp1[k-1][j-1][i] +
									temp1[k-1][j][i-1] +
									temp1[k-1][j][i+1] +
									temp1[k-1][j+1][i] +
									temp1[k][j-1][i-1] +
									temp1[k][j-1][i+1] +
									temp1[k][j+1][i-1] +
									temp1[k][j+1][i+1] +
									temp1[k+1][j-1][i] +
									temp1[k+1][j][i-1] +
									temp1[k+1][j][i+1] +
									temp1[k][j+1][i]) +
							f*(temp1[k-1][j][i] +
									temp1[k][j-1][i] +
									temp1[k][j][i-1] +
									temp1[k][j][i+1] +
									temp1[k][j+1][i] +
									temp1[k+1][j][i]) +
							0.13*temp1[k][j][i];
					} else {
						temp1[k][j][i] = a*temp2[k][j][i] -
							d*(temp2[k-1][j-1][i-1] +               
									temp2[k-1][j-1][i+1] +
									temp2[k-1][j+1][i-1] +       
									temp2[k-1][j+1][i+1] +               
									temp2[k+1][j-1][i-1] +               
									temp2[k+1][j-1][i+1] +               
									temp2[k+1][j+1][i-1] +               
									temp2[k+1][j+1][i+1]) +              
							e*(temp2[k-1][j-1][i] +                 
									temp2[k-1][j][i-1] + 
									temp2[k-1][j][i+1] +
									temp2[k-1][j+1][i] +
									temp2[k][j-1][i-1] +
									temp2[k][j-1][i+1] +
									temp2[k][j+1][i-1] +
									temp2[k][j+1][i+1] +
									temp2[k+1][j-1][i] +
									temp2[k+1][j][i-1] +
									temp2[k+1][j][i+1] +
									temp2[k][j+1][i]) +
							f*(temp2[k-1][j][i] +
									temp2[k][j-1][i] +
									temp2[k][j][i-1] +
									temp2[k][j][i+1] +
									temp2[k][j+1][i] +
									temp2[k+1][j][i]) +
							0.13*temp2[k][j][i];
					}
				}
			}
		}
	}
	memcpy(fout, ftemp1, sizeof(T)*L*M*N);
}
