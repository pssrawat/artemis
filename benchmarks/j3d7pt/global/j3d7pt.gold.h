#include <cstring>
using std::memcpy;

template <class T>
void jacobi_gold(T *fout, const T *fin, double h2inv, double a, double b, int L, int M, int N) {
	double (*out)[M][N] = (double (*)[M][N]) fout;
	double (*in)[M][N] = (double (*)[M][N]) fin;
	auto ftemp1 = new T[L * M * N];
	memset(ftemp1, 0, sizeof(T)*L*M*N);
	double (*temp1)[M][N] = (T (*)[M][N]) ftemp1;
	double c = b * h2inv;

	for (int t = 0; t < 2; t++) {
#pragma omp parallel for
		for (int k = 1; k < L - 1; ++k) {
			for (int j = 1; j < M - 1; ++j) {
				for (int i = 1; i < N - 1; ++i) {
					if (t==0) {
						temp1[k][j][i] = a*in[k][j][i] - c*in[k][j][i+1]
							+ c*in[k][j][i-1]
							+ c*in[k][j+1][i]
							+ c*in[k][j-1][i]
							+ c*in[k+1][j][i]
							+ c*in[k-1][j][i]
							- c*in[k][j][i]*6.0;
					}
					else {
						out[k][j][i] = a*temp1[k][j][i] - c*temp1[k][j][i+1]
							+ c*temp1[k][j][i-1]
							+ c*temp1[k][j+1][i]
							+ c*temp1[k][j-1][i]
							+ c*temp1[k+1][j][i]
							+ c*temp1[k-1][j][i]
							- c*temp1[k][j][i]*6.0;
					}
				}
			}
		}
	}
}
