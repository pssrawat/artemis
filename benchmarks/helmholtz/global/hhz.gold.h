#include <iostream>
using std::cout;

void hhz_gold (double *fout, double* fin, double h2inv, double a, double b, int N, int nt)
{
	double (*in)[N][N] = (double (*)[N][N])fin;
	double (*out)[N][N] = (double (*)[N][N])fout;
	double c = b*h2inv*0.0833;
	double d = c * 1.0;
	double e = c * 16.0;
	double f = c * 90.0;

	for (int t=0; t<nt; t++) {
		if (t % 2 == 0) {
			for (int k=2; k<N-2; k++) {
				for (int j=2; j<N-2; j++) {
					for (int i=2; i<N-2; i++) {
						out[k][j][i] = a*in[k][j][i] + 
							d*(in[k-2][j][i] + 
									in[k][j-2][i] + 
									in[k][j][i-2] + 
									in[k][j][i+2] + 
									in[k][j+2][i] + 
									in[k+2][j][i]) + 
							e*(in[k-1][j][i] + 
									in[k][j-1][i] + 
									in[k][j][i-1] + 
									in[k][j][i+1] + 
									in[k][j+1][i] + 
									in[k+1][j][i]) -
							f*in[k][j][i];
					}
				}
			}
		} else {
			for (int k=2; k<N-2; k++) {
				for (int j=2; j<N-2; j++) {
					for (int i=2; i<N-2; i++) {
						in[k][j][i] = a*out[k][j][i] + 
							d*(out[k-2][j][i] + 
									out[k][j-2][i] + 
									out[k][j][i-2] + 
									out[k][j][i+2] + 
									out[k][j+2][i] + 
									out[k+2][j][i]) + 
							e*(out[k-1][j][i] + 
									out[k][j-1][i] + 
									out[k][j][i-1] + 
									out[k][j][i+1] + 
									out[k][j+1][i] + 
									out[k+1][j][i]) -
							f*out[k][j][i];
					}
				}
			}
		}
	}
}
#undef N
