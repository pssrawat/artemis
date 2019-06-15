template <class T>
void jacobi_gold(T *out, T *in, int L, int M, int N) {
  T *temp = new T[L][M][N];
  for (int k = 0; k < L; ++k) {
    for (int j = 0; j < M; ++j) {
      for (int i = 0; i < N; ++i) {
	temp[k][j][i] = 0.1*in[k-1][j][i] +
	  0.2*(in[k][j-1][i] + in[k][j+1][i] + in[k][j][i] + in[k][j][i-1] + in[k][j][i+1]) +
	  0.3*in[k+1][j][i]; 
      }
    }
  }
  for (int k = 0; k < L; ++k) {
    for (int j = 0; j < M; ++j) {
      for (int i = 0; i < N; ++i) {
	out[k][j][i] = 0.1*temp[k-1][j][i] +
	  0.2*(temp[k][j-1][i] + temp[k][j+1][i] + temp[k][j][i] + temp[k][j][i-1] + temp[k][j][i+1]) +
	  0.3*temp[k+1][j][i]; 
      }
    }
  }
}

