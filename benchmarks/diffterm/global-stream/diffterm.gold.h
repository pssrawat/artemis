#include "common/common.hpp"

extern "C" void diffterm_gold (double *in_difflux_1, double *in_difflux_2, double *in_difflux_3, double *in_difflux_4, double *in_q_1, double *in_q_2, double *in_q_3, double *in_q_5, double *in_ux, double *in_uy, double *in_uz, double *in_vx, double *in_vy, double *in_vz, double *in_wx, double *in_wy, double *in_wz, int N) {
	double (*difflux_1)[320][320] = (double (*)[320][320])in_difflux_1;
	double (*difflux_2)[320][320] = (double (*)[320][320])in_difflux_2;
	double (*difflux_3)[320][320] = (double (*)[320][320])in_difflux_3;
	double (*difflux_4)[320][320] = (double (*)[320][320])in_difflux_4;
	double (*q_1)[320][320] = (double (*)[320][320])in_q_1;
	double (*q_2)[320][320] = (double (*)[320][320])in_q_2;
	double (*q_3)[320][320] = (double (*)[320][320])in_q_3;
	double (*q_5)[320][320] = (double (*)[320][320])in_q_5;
	double (*ux)[320][320] = (double (*)[320][320])in_ux;
	double (*uy)[320][320] = (double (*)[320][320])in_uy;
	double (*uz)[320][320] = (double (*)[320][320])in_uz;
	double (*vx)[320][320] = (double (*)[320][320])in_vx;
	double (*vy)[320][320] = (double (*)[320][320])in_vy;
	double (*vz)[320][320] = (double (*)[320][320])in_vz;
	double (*wx)[320][320] = (double (*)[320][320])in_wx;
	double (*wy)[320][320] = (double (*)[320][320])in_wy;
	double (*wz)[320][320] = (double (*)[320][320])in_wz;
	double dxinv0 = 0.01;
	double dxinv1 = 0.02;
	double dxinv2 = 0.03;

	int t, i, j, k;
#pragma omp parallel 
	{
#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					ux[k][j][i] = (0.8 * (q_1[k][j][i+1] - q_1[k][j][i-1]) + -0.2 * (q_1[k][j][i+2] - q_1[k][j][i-2]) + 0.038 * (q_1[k][j][i+3] - q_1[k][j][i-3]) + -0.0035 * (q_1[k][j][i+4] - q_1[k][j][i-4])) * dxinv0;
					vx[k][j][i] = (0.8 * (q_2[k][j][i+1] - q_2[k][j][i-1]) + -0.2 * (q_2[k][j][i+2] - q_2[k][j][i-2]) + 0.038 * (q_2[k][j][i+3] - q_2[k][j][i-3]) + -0.0035 * (q_2[k][j][i+4] - q_2[k][j][i-4])) * dxinv0;
					wx[k][j][i] = (0.8 * (q_3[k][j][i+1] - q_3[k][j][i-1]) + -0.2 * (q_3[k][j][i+2] - q_3[k][j][i-2]) + 0.038 * (q_3[k][j][i+3] - q_3[k][j][i-3]) + -0.0035 * (q_3[k][j][i+4] - q_3[k][j][i-4])) * dxinv0;
				}
			}
		}

#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					uy[k][j][i] = (0.8 * (q_1[k][j+1][i] - q_1[k][j-1][i]) + -0.2 * (q_1[k][j+2][i] - q_1[k][j-2][i]) + 0.038 * (q_1[k][j+3][i] - q_1[k][j-3][i]) + -0.0035 * (q_1[k][j+4][i] - q_1[k][j-4][i])) * dxinv1;
					vy[k][j][i] = (0.8 * (q_2[k][j+1][i] - q_2[k][j-1][i]) + -0.2 * (q_2[k][j+2][i] - q_2[k][j-2][i]) + 0.038 * (q_2[k][j+3][i] - q_2[k][j-3][i]) + -0.0035 * (q_2[k][j+4][i] - q_2[k][j-4][i])) * dxinv1;
					wy[k][j][i] = (0.8 * (q_3[k][j+1][i] - q_3[k][j-1][i]) + -0.2 * (q_3[k][j+2][i] - q_3[k][j-2][i]) + 0.038 * (q_3[k][j+3][i] - q_3[k][j-3][i]) + -0.0035 * (q_3[k][j+4][i] - q_3[k][j-4][i])) * dxinv1;
				}
			}
		}

#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					uz[k][j][i] = (0.8 * (q_1[k+1][j][i] - q_1[k-1][j][i]) + -0.2 * (q_1[k+2][j][i] - q_1[k-2][j][i]) + 0.038 * (q_1[k+3][j][i] - q_1[k-3][j][i]) + -0.0035 * (q_1[k+4][j][i] - q_1[k-4][j][i])) * dxinv2;
					vz[k][j][i] = (0.8 * (q_2[k+1][j][i] - q_2[k-1][j][i]) + -0.2 * (q_2[k+2][j][i] - q_2[k-2][j][i]) + 0.038 * (q_2[k+3][j][i] - q_2[k-3][j][i]) + -0.0035 * (q_2[k+4][j][i] - q_2[k-4][j][i])) * dxinv2;
					wz[k][j][i] = (0.8 * (q_3[k+1][j][i] - q_3[k-1][j][i]) + -0.2 * (q_3[k+2][j][i] - q_3[k-2][j][i]) + 0.038 * (q_3[k+3][j][i] - q_3[k-3][j][i]) + -0.0035 * (q_3[k+4][j][i] - q_3[k-4][j][i])) * dxinv2;
				}
			}
		}

#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					double uxx = (-2.847 * q_1[k][j][i]+1.6 * (q_1[k][j][i+1] + q_1[k][j][i-1]) + -0.2 * (q_1[k][j][i+2] + q_1[k][j][i-2]) + 0.0253 * (q_1[k][j][i+3] + q_1[k][j][i-3]) + -0.0017 * (q_1[k][j][i+4] + q_1[k][j][i-4])) * (dxinv0*dxinv0);
					double uyy = (-2.847 * q_1[k][j][i]+1.6 * (q_1[k][j+1][i] + q_1[k][j-1][i]) + -0.2 * (q_1[k][j+2][i] + q_1[k][j-2][i]) + 0.0253 * (q_1[k][j+3][i] + q_1[k][j-3][i]) + -0.0017 * (q_1[k][j+4][i] + q_1[k][j-4][i])) * (dxinv1*dxinv1);
					double uzz = (-2.847 * q_1[k][j][i]+1.6 * (q_1[k+1][j][i] + q_1[k-1][j][i]) + -0.2 * (q_1[k+2][j][i] + q_1[k-2][j][i]) + 0.0253 * (q_1[k+3][j][i] + q_1[k-3][j][i]) + -0.0017 * (q_1[k+4][j][i] + q_1[k-4][j][i])) * (dxinv2*dxinv2);
					double vyx = (0.8 * (vy[k][j][i+1] - vy[k][j][i-1]) + -0.2 * (vy[k][j][i+2] - vy[k][j][i-2]) + 0.038 * (vy[k][j][i+3] - vy[k][j][i-3]) + -0.0035 * (vy[k][j][i+4] - vy[k][j][i-4])) * dxinv0;
					double wzx = (0.8 * (wz[k][j][i+1] - wz[k][j][i-1]) + -0.2 * (wz[k][j][i+2] - wz[k][j][i-2]) + 0.038 * (wz[k][j][i+3] - wz[k][j][i-3]) + -0.0035 * (wz[k][j][i+4] - wz[k][j][i-4])) * dxinv0;
					difflux_1[k][j][i] = 0.3311 * (1.333 * uxx + uyy + uzz + 0.333 * (vyx + wzx));
				}
			}
		}

#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					double vxx = (-2.847 * q_2[k][j][i]+1.6 * (q_2[k][j][i+1] + q_2[k][j][i-1]) + -0.2 * (q_2[k][j][i+2] + q_2[k][j][i-2]) + 0.0253 * (q_2[k][j][i+3] + q_2[k][j][i-3]) + -0.0017 * (q_2[k][j][i+4] + q_2[k][j][i-4])) * (dxinv0*dxinv0);
					double vyy = (-2.847 * q_2[k][j][i]+1.6 * (q_2[k][j+1][i] + q_2[k][j-1][i]) + -0.2 * (q_2[k][j+2][i] + q_2[k][j-2][i]) + 0.0253 * (q_2[k][j+3][i] + q_2[k][j-3][i]) + -0.0017 * (q_2[k][j+4][i] + q_2[k][j-4][i])) * (dxinv1*dxinv1);
					double vzz = (-2.847 * q_2[k][j][i]+1.6 * (q_2[k+1][j][i] + q_2[k-1][j][i]) + -0.2 * (q_2[k+2][j][i] + q_2[k-2][j][i]) + 0.0253 * (q_2[k+3][j][i] + q_2[k-3][j][i]) + -0.0017 * (q_2[k+4][j][i] + q_2[k-4][j][i])) * (dxinv2*dxinv2);
					double uxy = (0.8 * (ux[k][j+1][i] - ux[k][j-1][i]) + -0.2 * (ux[k][j+2][i] - ux[k][j-2][i]) + 0.038 * (ux[k][j+3][i] - ux[k][j-3][i]) + -0.0035 * (ux[k][j+4][i] - ux[k][j-4][i])) * dxinv1;
					double wzy = (0.8 * (wz[k][j+1][i] - wz[k][j-1][i]) + -0.2 * (wz[k][j+2][i] - wz[k][j-2][i]) + 0.038 * (wz[k][j+3][i] - wz[k][j-3][i]) + -0.0035 * (wz[k][j+4][i] - wz[k][j-4][i])) * dxinv1;
					difflux_2[k][j][i] = 0.3311 * (vxx+1.333 * vyy + vzz + 0.333 * (uxy + wzy));
				}
			}
		}

#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					double wxx = (-2.847 * q_3[k][j][i]+1.6 * (q_3[k][j][i+1] + q_3[k][j][i-1]) + -0.2 * (q_3[k][j][i+2] + q_3[k][j][i-2]) + 0.0253 * (q_3[k][j][i+3] + q_3[k][j][i-3]) + -0.0017 * (q_3[k][j][i+4] + q_3[k][j][i-4])) * (dxinv0*dxinv0);
					double wyy = (-2.847 * q_3[k][j][i]+1.6 * (q_3[k][j+1][i] + q_3[k][j-1][i]) + -0.2 * (q_3[k][j+2][i] + q_3[k][j-2][i]) + 0.0253 * (q_3[k][j+3][i] + q_3[k][j-3][i]) + -0.0017 * (q_3[k][j+4][i] + q_3[k][j-4][i])) * (dxinv1*dxinv1);
					double wzz = (-2.847 * q_3[k][j][i]+1.6 * (q_3[k+1][j][i] + q_3[k-1][j][i]) + -0.2 * (q_3[k+2][j][i] + q_3[k-2][j][i]) + 0.0253 * (q_3[k+3][j][i] + q_3[k-3][j][i]) + -0.0017 * (q_3[k+4][j][i] + q_3[k-4][j][i])) * (dxinv2*dxinv2);
					double uxz = (0.8 * (ux[k+1][j][i] - ux[k-1][j][i]) + -0.2 * (ux[k+2][j][i] - ux[k-2][j][i]) + 0.038 * (ux[k+3][j][i] - ux[k-3][j][i]) + -0.0035 * (ux[k+4][j][i] - ux[k-4][j][i])) * dxinv2;
					double vyz = (0.8 * (vy[k+1][j][i] - vy[k-1][j][i]) + -0.2 * (vy[k+2][j][i] - vy[k-2][j][i]) + 0.038 * (vy[k+3][j][i] - vy[k-3][j][i]) + -0.0035 * (vy[k+4][j][i] - vy[k-4][j][i])) * dxinv2;
					difflux_3[k][j][i] = 0.3311 * (wxx + wyy+1.333 * wzz + 0.333 * (uxz + vyz));
				}
			}
		}

#pragma omp for private(j,i) 
		for (k = 4; k < N-4; k++) {
			for (j = 4; j < N-4; j++) {
#pragma GCC ivdep
#pragma clang loop vectorize (enable) interleave(enable)
				for (i = 4; i < N-4; i++) {
					double txx = (-2.847 * q_5[k][j][i]+1.6 * (q_5[k][j][i+1] + q_5[k][j][i-1]) + -0.2 * (q_5[k][j][i+2] + q_5[k][j][i-2]) + 0.0253 * (q_5[k][j][i+3] + q_5[k][j][i-3]) + -0.0017 * (q_5[k][j][i+4] + q_5[k][j][i-4])) * (dxinv0*dxinv0);
					double tyy = (-2.847 * q_5[k][j][i]+1.6 * (q_5[k][j+1][i] + q_5[k][j-1][i]) + -0.2 * (q_5[k][j+2][i] + q_5[k][j-2][i]) + 0.0253 * (q_5[k][j+3][i] + q_5[k][j-3][i]) + -0.0017 * (q_5[k][j+4][i] + q_5[k][j-4][i])) * (dxinv1*dxinv1);
					double tzz = (-2.847 * q_5[k][j][i]+1.6 * (q_5[k+1][j][i] + q_5[k-1][j][i]) + -0.2 * (q_5[k+2][j][i] + q_5[k-2][j][i]) + 0.0253 * (q_5[k+3][j][i] + q_5[k-3][j][i]) + -0.0017 * (q_5[k+4][j][i] + q_5[k-4][j][i])) * (dxinv2*dxinv2);
					double divu = 0.666 * (ux[k][j][i] + vy[k][j][i] + wz[k][j][i]);
					double tauxx = 2.e0 * ux[k][j][i] - divu;
					double tauyy = 2.e0 * vy[k][j][i] - divu;
					double tauzz = 2.e0 * wz[k][j][i] - divu;
					double tauxy = uy[k][j][i] + vx[k][j][i];
					double tauxz = uz[k][j][i] + wx[k][j][i];
					double tauyz = vz[k][j][i] + wy[k][j][i];
					double mechwork = tauxx * ux[k][j][i] + tauyy * vy[k][j][i] + tauzz * wz[k][j][i] + (tauxy*tauxy) + (tauxz*tauxz) + (tauyz*tauyz);
					mechwork = 0.3311 * mechwork + difflux_1[k][j][i] * q_1[k][j][i] + difflux_2[k][j][i] * q_2[k][j][i] + difflux_3[k][j][i] * q_3[k][j][i];
					difflux_4[k][j][i] = 0.7112 * (txx + tyy + tzz) + mechwork;
				}
			}
		}
	}

}
