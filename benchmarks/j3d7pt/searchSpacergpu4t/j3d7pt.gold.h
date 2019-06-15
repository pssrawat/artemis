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

	for (int t = 0; t < 12; t++) {
#pragma omp parallel for
		for (int k = 1; k < L - 1; ++k) {
			for (int j = 1; j < M - 1; ++j) {
				for (int i = 1; i < N - 1; ++i) {
					if (!(t%2)) {
						temp2[k][j][i] = a*temp1[k][j][i] - c*temp1[k][j][i+1]
							+ c*temp1[k][j][i-1]
							+ c*temp1[k][j+1][i]
							+ c*temp1[k][j-1][i]
							+ c*temp1[k+1][j][i]
							+ c*temp1[k-1][j][i]
							- c*temp1[k][j][i]*6.0;
					} else {
                                                temp1[k][j][i] = a*temp2[k][j][i] - c*temp2[k][j][i+1]
                                                        + c*temp2[k][j][i-1]
                                                        + c*temp2[k][j+1][i]
                                                        + c*temp2[k][j-1][i]
                                                        + c*temp2[k+1][j][i]
                                                        + c*temp2[k-1][j][i]
                                                        - c*temp2[k][j][i]*6.0;
					}
				}
			}
		}
	}
        memcpy(fout, ftemp1, sizeof(T)*L*M*N);
}
