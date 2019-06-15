#include "common/common.hpp"
#include "diffterm.gold.h"
#include <cassert>
#include <cstdio>

extern "C" void host_code (double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, double*, int, int, int);

int main(int argc, char** argv) {
	int N = 320;

	double (*difflux_ref_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*difflux_ref_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*difflux_ref_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*difflux_ref_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);

	double (*difflux_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*difflux_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*difflux_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*difflux_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);

	double (*q_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*q_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*q_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*q_5)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);

	double (*ux_ref)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*ux)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*uy)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*uz)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*wx)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*wy)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*wz_ref)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*wz)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*vx)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*vy_ref)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*vy)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*vz)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	memcpy (ux, ux_ref, sizeof(double)*N*N*N);
	memcpy (vy, vy_ref, sizeof(double)*N*N*N);
	memcpy (wz, wz_ref, sizeof(double)*N*N*N);

	diffterm_gold ((double*)difflux_ref_1, (double*)difflux_ref_2, (double*)difflux_ref_3, (double*)difflux_ref_4, (double*)q_1, (double*)q_2, (double*)q_3, (double*)q_5, (double*)ux_ref, (double*)uy, (double*)uz, (double*)vx, (double*)vy_ref, (double*)vz, (double*)wx, (double*)wy, (double*)wz_ref, N);
	host_code ((double*)difflux_1, (double*)difflux_2, (double*)difflux_3, (double*)difflux_4, (double*)q_1, (double*)q_2, (double*)q_3, (double*)q_5, (double*)ux, (double*)uy, (double*)uz, (double*)vx, (double*)vy, (double*)vz, (double*)wx, (double*)wy, (double*)wz, N, N, N);

	double error_1 = checkError3D<double> (N, N, (double*)difflux_1, (double*)difflux_ref_1, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_1);
	if (error_1 > TOLERANCE)
		return -1;

	double error_2 = checkError3D<double> (N, N, (double*)difflux_2, (double*)difflux_ref_2, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_2);
	if (error_2 > TOLERANCE)
		return -1;

	double error_3 = checkError3D<double> (N, N, (double*)difflux_3, (double*)difflux_ref_3, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_3);
	if (error_3 > TOLERANCE)
		return -1;

	double error_4 = checkError3D<double> (N, N, (double*)difflux_4, (double*)difflux_ref_4, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_4);
	if (error_4 > TOLERANCE)
		return -1;
}
