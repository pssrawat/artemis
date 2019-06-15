#include "common/common.hpp"

void cheby_step (double *out_def, double* Ac_def, double* Ap_def, double* RHS_def, double* Dinv_def, double h2inv, double c1, double c2, int N) {
	double (*Ap)[N][N] = (double (*)[N][N])Ap_def;
	double (*Ac)[N][N] = (double (*)[N][N])Ac_def;
	double (*out)[N][N] = (double (*)[N][N])out_def;
	double (*RHS)[N][N] = (double (*)[N][N])RHS_def;
	double (*Dinv)[N][N] = (double (*)[N][N])Dinv_def;
#pragma omp parallel for
	for (int k = 1; k < N-1; k++) {
		for (int j = 1; j < N-1; j++) {
#pragma GCC ivdep
			for (int i = 1; i < N-1; i++) {
				double MA = Ac[k][j][i] - h2inv * (0.03 * (Ac[k-1][j-1][i-1] + Ac[k-1][j-1][i+1] + Ac[k-1][j+1][i-1] + Ac[k-1][j+1][i+1] + Ac[k+1][j-1][i-1] + Ac[k+1][j-1][i+1] + Ac[k+1][j+1][i-1] + Ac[k+1][j+1][i+1]) + 0.1 * (Ac[k-1][j-1][i] + Ac[k-1][j][i-1] + Ac[k-1][j][i+1] + Ac[k-1][j+1][i] + Ac[k][j-1][i-1] + Ac[k][j-1][i+1] + Ac[k][j+1][i-1] + Ac[k][j+1][i+1] + Ac[k+1][j-1][i] + Ac[k+1][j][i-1] + Ac[k+1][j][i+1] + Ac[k+1][j+1][i]) + 0.46 * (Ac[k-1][j][i] + Ac[k][j-1][i] + Ac[k][j][i-1] + Ac[k][j][i+1] + Ac[k][j+1][i] + Ac[k+1][j][i]) - 4.26 * Ac[k][j][i]);
				out[k][j][i] = Ac[k][j][i] + c1 * (Ac[k][j][i] - Ap[k][j][i]) + c2 * Dinv[k][j][i] * (RHS[k][j][i] - MA);
			}
		}
	}
}

extern "C" void cheby_gold (double* out, double *Ac, double* Ap, double* RHS, double* Dinv, double h2inv, double c1, double c2, int N) {
	double* temp1 = getZero3DArray<double>(N, N, N);
	double* temp2 = getZero3DArray<double>(N, N, N);
	double* temp3 = getZero3DArray<double>(N, N, N);

	cheby_step(temp1, Ac, Ap, RHS, Dinv, h2inv, c1, c2, N);
	cheby_step(out, temp1, Ac, RHS, Dinv, h2inv, c1, c2, N);

	delete[] temp1;
	delete[] temp2;
	delete[] temp3;
}
