#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define TOLERANCE 1e-5

struct float4 {
  float x;
  float y;
  float z;
  float w;
};


template<typename T>
static T get_random() {
  return ((T)(rand())/(T)(RAND_MAX-1));
}

template<>
float4 get_random<float4>() {
  float4 random;
  random.x = get_random<float>();
  random.y = get_random<float>();
  random.z = get_random<float>();
  random.w = get_random<float>();
  return random;
}

template<typename T>
static T* getRandom2DArray(int width_y, int width_x) {
  T (*a)[width_x] = (T (*)[width_x])new T[width_y*width_x];
  for (int j = 0; j < width_y; j++)
    for (int k = 0; k < width_x; k++) {
      a[j][k] = get_random<T>();
    }
  return (T*)a;
}


template<typename T>
static T* getRandom3DArray(int height, int width_y, int width_x) {
  T (*a)[width_y][width_x] =
    (T (*)[width_y][width_x])new T[height*width_y*width_x];
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width_y; j++)
      for (int k = 0; k < width_x; k++) {
        a[i][j][k] = get_random<T>();
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
static T* getZero2DArray(int width_y, int width_x) {
  T (*a)[width_x] = (T (*)[width_x])new T[width_y*width_x];
  memset((void*)a, 0, sizeof(T) * width_y * width_x);
  return (T*)a;
}

template<typename T>
static T* getZero3DArray(int height, int width_y, int width_x) {
  T (*a)[width_y][width_x] =
    (T (*)[width_y][width_x])new T[height*width_y*width_x];
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
static double checkError2D
(int width_x, const T *l_output, const T *l_reference, int y_lb, int y_ub,
 int x_lb, int x_ub) {
  const T (*output)[width_x] = (const T (*)[width_x])(l_output);
  const T (*reference)[width_x] = (const T (*)[width_x])(l_reference);
  double error = 0.0;
  double max_error = 0.0;
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
  const T (*output)[width_y][width_x] =
    (const T (*)[width_y][width_x])(l_output);
  const T (*reference)[width_y][width_x] =
    (const T (*)[width_y][width_x])(l_reference);
  double error = 0.0;
  double max_error = 1e-13;
  int max_k = 0, max_j = 0, max_i = 0;
  for (int i = z_lb; i < z_ub; i++)
    for (int j = y_lb; j < y_ub; j++)
      for (int k = x_lb; k < x_ub; k++) {
	//printf ("real var1[%d][%d][%d] = %.6f and %.6f\n", i, j, k, reference[i][j][k], output[i][j][k]);
        double curr_error = output[i][j][k] - reference[i][j][k];
        curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
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
  double max_error = 0.0;
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

template<>
double checkError3D<float4>
(int width_y, int width_x, const float4 *l_output, const float4 *l_reference,
 int z_lb, int z_ub, int y_lb, int y_ub, int x_lb, int x_ub) {
  const float4 (*output)[width_y][width_x] =
    (const float4 (*)[width_y][width_x])(l_output);
  const float4 (*reference)[width_y][width_x] =
    (const float4 (*)[width_y][width_x])(l_reference);
  double error = 0.0;
  double max_error = 0.0;
  int max_k = 0, max_j = 0, max_i = 0, max_field = 0;
  for (int i = z_lb; i < z_ub; i++)
    for (int j = y_lb; j < y_ub; j++)
      for (int k = x_lb; k < x_ub; k++) {
        double curr_error = output[i][j][k].x - reference[i][j][k].x;
        curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
        error += curr_error * curr_error;
        if (curr_error > max_error) {
	  printf ("Values x at index (%d,%d,%d) differ : %.6f and %.6f\n", i, j, k, reference[i][j][k].x, output[i][j][k].x);
          max_error = curr_error;
          max_k = k;
          max_j = j;
          max_i = i;
          max_field = 0;
        }
        curr_error = output[i][j][k].y - reference[i][j][k].y;
        curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
        error += curr_error * curr_error;
        if (curr_error > max_error) {
	  printf ("Values y at index (%d,%d,%d) differ : %.6f and %.6f\n", i, j, k, reference[i][j][k].y, output[i][j][k].y);
          max_error = curr_error;
          max_k = k;
          max_j = j;
          max_i = i;
          max_field = 1;
        }
        curr_error = output[i][j][k].z - reference[i][j][k].z;
        curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
        error += curr_error * curr_error;
        if (curr_error > max_error) {
	  printf ("Values z at index (%d,%d,%d) differ : %.6f and %.6f\n", i, j, k, reference[i][j][k].z, output[i][j][k].z);
          max_error = curr_error;
          max_k = k;
          max_j = j;
          max_i = i;
          max_field = 2;
        }
        curr_error = output[i][j][k].w - reference[i][j][k].w;
        curr_error = (curr_error < 0.0 ? -curr_error : curr_error);
        error += curr_error * curr_error;
        if (curr_error > max_error) {
 	  printf ("Values w at index (%d,%d,%d) differ : %.6f and %.6f\n", i, j, k, reference[i][j][k].w, output[i][j][k].w);
          max_error = curr_error;
          max_k = k;
          max_j = j;
          max_i = i;
          max_field = 3;
        }
      }
  printf
    ("[Test] Max Error : %e @ (%d,%d,%d,%c)\n", max_error, max_i, max_j,
     max_k,
     (max_field == 0 ? 'x' :
      (max_field == 1 ? 'y' : (max_field == 2 ? 'z' : 'w'))));
  error = sqrt(error / ( 4 * (z_ub - z_lb) * (y_ub - y_lb) * (x_ub - x_lb)));
  return error;
}

template<typename T>
static void print2DArray
(int width_x, const T* l_array, int y_lb, int y_ub, int x_lb, int x_ub) {
  const T (*array)[width_x] = (T (*)[width_x])l_array;
  for (int j = y_lb; j < y_ub; j++) {
    printf("\t");
    for (int k = x_lb; k < x_ub; k++)
      printf(" %e", array[j][k]);
    printf("\n");
  }
}

template<typename T>
static void print3DArray
(int width_y, int width_x, const T* l_array,  int z_lb, int z_ub, int y_lb,
 int y_ub, int x_lb, int x_ub) {
  const T (*array)[width_y][width_x] = (T (*)[width_y][width_x])l_array;
  for (int i = z_lb ; i < z_ub ; i++) {
    printf("\tLevel : %d\n", i);
    for (int j = y_lb; j < y_ub; j++) {
      printf("\t");
      for (int k = x_lb; k < x_ub; k++)
        printf(" %e", array[i][j][k]);
      printf("\n");
    }
  }
}

template<>
void print3DArray<float4>
(int width_y, int width_x, const float4* l_array,  int z_lb, int z_ub, int y_lb,
 int y_ub, int x_lb, int x_ub) {
  const float4 (*array)[width_y][width_x] =
    (float4 (*)[width_y][width_x])l_array;
  for (int i = z_lb ; i < z_ub ; i++) {
    printf("\tLevel : %d\n", i);
    for (int j = y_lb; j < y_ub; j++) {
      printf("\t");
      for (int k = x_lb; k < x_ub; k++)
        printf
          (" [%e, %e, %e, %e]", array[i][j][k].x, array[i][j][k].y,
           array[i][j][k].z, array[i][j][k].w);
      printf("\n");
    }
  }
}


#endif
