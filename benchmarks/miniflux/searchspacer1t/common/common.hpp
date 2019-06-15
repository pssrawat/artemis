#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

#define TOLERANCE 1e-5

template<typename T>
static T get_random() {
	return ((T)(rand())/(T)(RAND_MAX-1));
}

template<typename T>
static T* getRandom1DArray(int width_y) {
	T (*a) = (T (*))new T[width_y];
	for (int j = 0; j < width_y; j++)
		a[j] = get_random<T>()+0.2001;
	return (T*)a;
}

template<typename T>
static T* getRandom2DArray(int width_y, int width_x) {
	T (*a)[8200] = (T (*)[8200])new T[8200*8200];
	for (int j = 0; j < width_y; j++)
		for (int k = 0; k < width_x; k++) {
			a[j][k] = get_random<T>();
		}
	return (T*)a;
}


template<typename T>
static T* getRandom3DArray(int height, int width_y, int width_x) {
	T (*a)[320][320] = (T (*)[320][320])new T[height*320*320];
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width_y; j++)
			for (int k = 0; k < width_x; k++) {
				a[i][j][k] = get_random<T>() + 0.02121;
			}
	return (T*)a;
}

template<typename T>
static T* getRandom4DArray(int depth, int height, int width_y, int width_x) {
	T (*a)[height][width_y][width_x] = (T (*)[height][width_y][width_x])new T[depth*height*width_y*width_x];
	for (int d = 0; d < depth; d++) 
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width_y; j++)
				for (int k = 0; k < width_x; k++) {
					a[d][i][j][k] = get_random<T>();
				}
	return (T*)a;
}

template<typename T>
static T* getZero1DArray(int width_x) {
	T (*a) = (T *)new T[width_x];
	memset((void*)a, 0, sizeof(T) * width_x);
	return (T*)a;
}

template<typename T>
static T* getZero2DArray(int width_y, int width_x) {
	T (*a)[8200] = (T (*)[8200])new T[8200*8200];
	memset((void*)a, 0, sizeof(T) * width_y * width_x);
	return (T*)a;
}

template<typename T>
static T* getZero3DArray(int height, int width_y, int width_x) {
	T (*a)[320][320] = (T (*)[320][320])new T[height*320*320];
	memset((void*)a, 0, sizeof(T) * height * width_y * width_x);
	return (T*)a;
}

template<typename T>
static T* getZero4DArray(int depth, int height, int width_y, int width_x) {
	T (*a)[height][width_y][width_x] = (T (*)[height][width_y][width_x])new T[depth*height*width_y*width_x];
	memset((void*)a, 0, sizeof(T) * depth * height * width_y * width_x);
	return (T*)a;
}

template<typename T>
static double checkError1D
(int width_x, const T *output, const T *reference, int x_lb, int x_ub) {
	double error = 0.0;
	double max_error = TOLERANCE;
	int max_j = 0;
	for (int j = x_lb; j < x_ub; j++) { 
		double curr_error = output[j] - reference[j];
		curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
		error += curr_error * curr_error;
		if (reference[j] == 0 && output[j] == 0)
			printf ("All zeros");
		if (curr_error > max_error) {
			printf ("Values at index (%d) differ : %.6f and %.6f\n", j, reference[j], output[j]);
			max_error = curr_error;
			max_j = j;
		}
	}
	printf ("[Test] Max Error (%d) : %e\n", max_j, max_error);
	error = sqrt(error / (x_ub - x_lb));
	return error;
}

template<typename T>
static double checkError2D
(int width_x, const T *l_output, const T *l_reference, int y_lb, int y_ub,
 int x_lb, int x_ub) {
	const T (*output)[8200] = (const T (*)[8200])(l_output);
	const T (*reference)[8200] = (const T (*)[8200])(l_reference);
	double error = 0.0;
	double max_error = TOLERANCE;
	int max_k = 0, max_j = 0;
	for (int j = y_lb; j < y_ub; j++) 
		for (int k = x_lb; k < x_ub; k++) {
			//printf ("Values at index (%d,%d) are %.6f and %.6f\n", j, k, reference[j][k], output[j][k]);
			double curr_error = output[j][k] - reference[j][k];
			curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
			error += curr_error * curr_error;
			if (curr_error > max_error) {
				printf ("Values at index (%d,%d) differ : %.6f and %.6f\n", j, k, reference[j][k], output[j][k]);
				max_error = curr_error;
				max_k = k;
				max_j = j;
			}
		}
	printf
		("[Test] Max Error : %e @ (,%d,%d)\n", max_error, max_j, max_k);
	error = sqrt(error / ( (y_ub - y_lb) * (x_ub - x_lb)));
	return error;
}

template<typename T>
static double checkError3D
(int width_y, int width_x, const T *l_output, const T *l_reference, int z_lb,
 int z_ub, int y_lb, int y_ub, int x_lb, int x_ub) {
	const T (*output)[320][320] = (const T (*)[320][320])(l_output);
	const T (*reference)[320][320] = (const T (*)[320][320])(l_reference);
	double error = 0.0;
	double max_error = TOLERANCE, sum = 0.0;
	int max_k = 0, max_j = 0, max_i = 0, unmatch=0, match =0;
	for (int i = z_lb; i < z_ub; i++)
		for (int j = y_lb; j < y_ub; j++)
			for (int k = x_lb; k < x_ub; k++) {
				sum += output[i][j][k];
				//printf ("real var1[%d][%d][%d] = %.6f and %.6f\n", i, j, k, reference[i][j][k], output[i][j][k]);
				double curr_error = output[i][j][k] - reference[i][j][k];
				curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
				if (curr_error == 0)
					match +=1;
				else unmatch+=1;
				error += curr_error * curr_error;
				if (curr_error > max_error) {
					printf ("Values at index (%d,%d,%d) differ : %.6f and %.6f\n", i, j, k, reference[i][j][k], output[i][j][k]);
					max_error = curr_error;
					max_k = k;
					max_j = j;
					max_i = i;
				}
			}
	printf
		("[Test] Max Error : %e @ (%d,%d,%d)\n", max_error, max_i, max_j, max_k);
	error = sqrt(error / ( (z_ub - z_lb) * (y_ub - y_lb) * (x_ub - x_lb)));
	return error;
}

template<typename T>
static double checkError4D
(int height, int width_y, int width_x, const T *l_output, const T *l_reference, int w_lb, int w_ub, int z_lb,
 int z_ub, int y_lb, int y_ub, int x_lb, int x_ub) {
	const T (*output)[height][width_y][width_x] = (const T (*)[height][width_y][width_x])(l_output);
	const T (*reference)[height][width_y][width_x] = (const T (*)[height][width_y][width_x])(l_reference);
	double error = 0.0;
	double max_error = TOLERANCE;
	int max_d = 0, max_k = 0, max_j = 0, max_i = 0;
	for (int d = w_lb; d < w_ub; d++) 
		for (int i = z_lb; i < z_ub; i++)
			for (int j = y_lb; j < y_ub; j++)
				for (int k = x_lb; k < x_ub; k++) {
					double curr_error = output[d][i][j][k] - reference[d][i][j][k];
					curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
					error += curr_error * curr_error;
					if (curr_error > max_error) {
						printf ("Values at index (%d,%d,%d,%d) differ : %.6f and %.6f\n", d, i, j, k, reference[d][i][j][k], output[d][i][j][k]);
						max_error = curr_error;
						max_d = d;
						max_k = k;
						max_j = j;
						max_i = i;
					}
				}
	printf
		("[Test] Max Error : %e @ (%d,%d,%d,%d)\n", max_error, max_d,max_i, max_j, max_k);
	error = sqrt(error / ( (w_ub-w_lb) * (z_ub - z_lb) * (y_ub - y_lb) * (x_ub - x_lb)));
	return error;
}

#endif
