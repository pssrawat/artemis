#include "common/common.hpp"
#include "miniflux.gold.h"
#include <cassert>
#include <cstdio>

extern "C" void host_code (double*, double*, double*, double*, double*, 
		double*, double*, double*, double*, double*, 
		double*, double*, double*, double*, double*,
		double*, double*, double*, double*, double*,
		double*, double*, double*, double*, double*,
		int, int, int);

int main(int argc, char** argv) {
	int N = 320;

	double (*old_box_0)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*old_box_1)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*old_box_2)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*old_box_3)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*old_box_4)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);

	double (*new_box_ref_0)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_ref_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_ref_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_ref_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_ref_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);


	double (*new_box_0)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*new_box_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);

	double (*gx_0)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gx_1)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gx_2)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gx_3)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gx_4)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gy_0)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gy_1)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gy_2)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gy_3)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gy_4)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gz_0)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gz_1)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gz_2)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gz_3)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);
	double (*gz_4)[320][320] = (double (*)[320][320]) getRandom3DArray<double>(320, 320, 320);

	double (*gx_ref_0)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gx_ref_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gx_ref_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gx_ref_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gx_ref_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gy_ref_0)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gy_ref_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gy_ref_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gy_ref_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gy_ref_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gz_ref_0)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gz_ref_1)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gz_ref_2)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gz_ref_3)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);
	double (*gz_ref_4)[320][320] = (double (*)[320][320]) getZero3DArray<double>(320, 320, 320);

	memcpy(gx_ref_0, gx_0, sizeof(double)*N*N*N);
	memcpy(gx_ref_1, gx_1, sizeof(double)*N*N*N);
	memcpy(gx_ref_2, gx_2, sizeof(double)*N*N*N);
	memcpy(gx_ref_3, gx_3, sizeof(double)*N*N*N);
	memcpy(gx_ref_4, gx_4, sizeof(double)*N*N*N);
	memcpy(gy_ref_0, gy_0, sizeof(double)*N*N*N);
	memcpy(gy_ref_1, gy_1, sizeof(double)*N*N*N);
	memcpy(gy_ref_2, gy_2, sizeof(double)*N*N*N);
	memcpy(gy_ref_3, gy_3, sizeof(double)*N*N*N);
	memcpy(gy_ref_4, gy_4, sizeof(double)*N*N*N);
	memcpy(gz_ref_0, gz_0, sizeof(double)*N*N*N);
	memcpy(gz_ref_1, gz_1, sizeof(double)*N*N*N);
	memcpy(gz_ref_2, gz_2, sizeof(double)*N*N*N);
	memcpy(gz_ref_3, gz_3, sizeof(double)*N*N*N);
	memcpy(gz_ref_4, gz_4, sizeof(double)*N*N*N);

	miniflux_gold ((double*)new_box_ref_0, (double*)new_box_ref_1, (double*)new_box_ref_2, (double*)new_box_ref_3, (double*)new_box_ref_4,
			(double*)old_box_0, (double*)old_box_1, (double*)old_box_2, (double*)old_box_3, (double*)old_box_4,
			(double*)gx_ref_0, (double*)gx_ref_1, (double*)gx_ref_2, (double*)gx_ref_3, (double*)gx_ref_4,
			(double*)gy_ref_0, (double*)gy_ref_1, (double*)gy_ref_2, (double*)gy_ref_3, (double*)gy_ref_4,
			(double*)gz_ref_0, (double*)gz_ref_1, (double*)gz_ref_2, (double*)gz_ref_3, (double*)gz_ref_4, N);

	host_code ((double*)new_box_0, (double*)new_box_1, (double*)new_box_2, (double*)new_box_3, (double*)new_box_4,
			(double*)old_box_0, (double*)old_box_1, (double*)old_box_2, (double*)old_box_3, (double*)old_box_4,
			(double*)gx_0, (double*)gx_1, (double*)gx_2, (double*)gx_3, (double*)gx_4,
			(double*)gy_0, (double*)gy_1, (double*)gy_2, (double*)gy_3, (double*)gy_4,
			(double*)gz_0, (double*)gz_1, (double*)gz_2, (double*)gz_3, (double*)gz_4, N, N, N);


	double error_0 = checkError3D<double> (N, N, (double*)new_box_0, (double*)new_box_ref_0, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_0);
	if (error_0 > TOLERANCE)
		return -1;

	double error_1 = checkError3D<double> (N, N, (double*)new_box_1, (double*)new_box_ref_1, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_1);
	if (error_1 > TOLERANCE)
		return -1;

	double error_2 = checkError3D<double> (N, N, (double*)new_box_2, (double*)new_box_ref_2, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_2);
	if (error_2 > TOLERANCE)
		return -1;

	double error_3 = checkError3D<double> (N, N, (double*)new_box_3, (double*)new_box_ref_3, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_3);
	if (error_3 > TOLERANCE)
		return -1;

	double error_4 = checkError3D<double> (N, N, (double*)new_box_4, (double*)new_box_ref_4, 4, N-4, 4, N-4, 4, N-4);
	printf("[Test] RMS Error : %e\n",error_4);
	if (error_4 > TOLERANCE)
		return -1;
}
