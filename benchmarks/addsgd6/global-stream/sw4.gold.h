void sw4_gold (double *up_in_0, double *up_in_1, double *up_in_2, double *u_in_0, double *u_in_1, double *u_in_2, double *um_in_0, double *um_in_1, double *um_in_2, double *rho_in, double *strx, double *stry, double *strz, double *dcx, double *dcy, double *dcz, double *cox, double *coy, double *coz, int N) {
	double (*up_0)[320][320] = (double (*)[320][320])up_in_0;
	double (*up_1)[320][320] = (double (*)[320][320])up_in_1;
	double (*up_2)[320][320] = (double (*)[320][320])up_in_2;
	double (*u_0)[320][320] = (double (*)[320][320])u_in_0;
	double (*u_1)[320][320] = (double (*)[320][320])u_in_1;
	double (*u_2)[320][320] = (double (*)[320][320])u_in_2;
	double (*um_0)[320][320] = (double (*)[320][320])um_in_0;
	double (*um_1)[320][320] = (double (*)[320][320])um_in_1;
	double (*um_2)[320][320] = (double (*)[320][320])um_in_2;
	double (*rho)[320][320] = (double (*)[320][320])rho_in;
	int i,j, k;
	/* Assumptions */
	double beta = 0.625;

	for (k = 3; k < N-3; k++) {
		for (j = 3; j < N-3; j++) {
			for (i = 3; i < N-3; i++) {
				double birho = 0.5*beta/rho[k][j][i];
				up_0[k][j][i] += birho*(strx[i]*coy[j]*coz[k]*(
						// x-differences
						(rho[k][j][i+2]*dcx[i+2]+rho[k][j][i+1]*dcx[i+1])*(u_0[k][j][i+3]-3*u_0[k][j][i+2]+3*u_0[k][j][i+1]-u_0[k][j][i]-(um_0[k][j][i+3]-3*um_0[k][j][i+2]+3*um_0[k][j][i+1]-um_0[k][j][i]))-3*(rho[k][j][i+1]*dcx[i+1]+rho[k][j][i]*dcx[i])*(u_0[k][j][i+2]-3*u_0[k][j][i+1]+3*u_0[k][j][i]-u_0[k][j][i-1]-(um_0[k][j][i+2]-3*um_0[k][j][i+1]+3*um_0[k][j][i]-um_0[k][j][i-1]))+3*(rho[k][j][i]*dcx[i]+rho[k][j][i-1]*dcx[i-1])*(u_0[k][j][i+1]-3*u_0[k][j][i]+3*u_0[k][j][i-1]-u_0[k][j][i-2]-(um_0[k][j][i+1]-3*um_0[k][j][i]+3*um_0[k][j][i-1]-um_0[k][j][i-2]))-(rho[k][j][i-1]*dcx[i-1]+rho[k][j][i-2]*dcx[i-2])*(u_0[k][j][i]-3*u_0[k][j][i-1]+3*u_0[k][j][i-2]-u_0[k][j][i-3]-(um_0[k][j][i]-3*um_0[k][j][i-1]+3*um_0[k][j][i-2]-um_0[k][j][i-3]))) +  stry[j]*cox[i]*coz[k]*(
						// y-differences
						(rho[k][j+2][i]*dcy[j+2]+rho[k][j+1][i]*dcy[j+1])*(u_0[k][j+3][i]-3*u_0[k][j+2][i]+3*u_0[k][j+1][i]-u_0[k][j][i]-(um_0[k][j+3][i]-3*um_0[k][j+2][i]+3*um_0[k][j+1][i]-um_0[k][j][i]))-3*(rho[k][j+1][i]*dcy[j+1]+rho[k][j][i]*dcy[j])*(u_0[k][j+2][i] -3*u_0[k][j+1][i]+3*u_0[k][j][i]-u_0[k][j-1][i]-(um_0[k][j+2][i]-3*um_0[k][j+1][i]+3*um_0[k][j][i]-um_0[k][j-1][i]))+3*(rho[k][j][i]*dcy[j]+rho[k][j-1][i]*dcy[j-1])*(u_0[k][j+1][i]-3*u_0[k][j][i]+3*u_0[k][j-1][i]-u_0[k][j-2][i]-(um_0[k][j+1][i]-3*um_0[k][j][i]+3*um_0[k][j-1][i]-um_0[k][j-2][i]))-(rho[k][j-1][i]*dcy[j-1]+rho[k][j-2][i]*dcy[j-2])*(u_0[k][j][i]-3*u_0[k][j-1][i]+3*u_0[k][j-2][i]-u_0[k][j-3][i]-(um_0[k][j][i]-3*um_0[k][j-1][i]+3*um_0[k][j-2][i]-um_0[k][j-3][i]))) +  strz[k]*cox[i]*coy[j]*(
						// z-differences
						(rho[k+2][j][i]*dcz[k+2]+rho[k+1][j][i]*dcz[k+1])*(u_0[k+3][j][i]-3*u_0[k+2][j][i]+3*u_0[k+1][j][i]-u_0[k][j][i]-(um_0[k+3][j][i]-3*um_0[k+2][j][i]+3*um_0[k+1][j][i]-um_0[k][j][i]))-3*(rho[k+1][j][i]*dcz[k+1]+rho[k][j][i]*dcz[k])*(u_0[k+2][j][i]-3*u_0[k+1][j][i]+3*u_0[k][j][i]-u_0[k-1][j][i]-(um_0[k+2][j][i]-3*um_0[k+1][j][i]+3*um_0[k][j][i]-um_0[k-1][j][i]))+3*(rho[k][j][i]*dcz[k]+rho[k-1][j][i]*dcz[k-1])*(u_0[k+1][j][i]-3*u_0[k][j][i]+3*u_0[k-1][j][i]-u_0[k-2][j][i]-(um_0[k+1][j][i]-3*um_0[k][j][i]+3*um_0[k-1][j][i]-um_0[k-2][j][i]))-(rho[k-1][j][i]*dcz[k-1]+rho[k-2][j][i]*dcz[k-2])*(u_0[k][j][i]-3*u_0[k-1][j][i]+3*u_0[k-2][j][i]-u_0[k-3][j][i]-(um_0[k][j][i]-3*um_0[k-1][j][i]+3*um_0[k-2][j][i]-um_0[k-3][j][i]))));

				up_1[k][j][i] += birho*(strx[i]*coy[j]*coz[k]*(
						// x-differences
						(rho[k][j][i+2]*dcx[i+2]+rho[k][j][i+1]*dcx[i+1])*(u_1[k][j][i+3]-3*u_1[k][j][i+2]+3*u_1[k][j][i+1]-u_1[k][j][i]-(um_1[k][j][i+3]-3*um_1[k][j][i+2]+3*um_1[k][j][i+1]-um_1[k][j][i]))-3*(rho[k][j][i+1]*dcx[i+1]+rho[k][j][i]*dcx[i])*(u_1[k][j][i+2]-3*u_1[k][j][i+1]+3*u_1[k][j][i]-u_1[k][j][i-1]-(um_1[k][j][i+2]-3*um_1[k][j][i+1]+3*um_1[k][j][i]-um_1[k][j][i-1]))+3*(rho[k][j][i]*dcx[i]+rho[k][j][i-1]*dcx[i-1])*(u_1[k][j][i+1]-3*u_1[k][j][i]+3*u_1[k][j][i-1]-u_1[k][j][i-2]-(um_1[k][j][i+1]-3*um_1[k][j][i]+3*um_1[k][j][i-1]-um_1[k][j][i-2]))-(rho[k][j][i-1]*dcx[i-1]+rho[k][j][i-2]*dcx[i-2])*(u_1[k][j][i]-3*u_1[k][j][i-1]+3*u_1[k][j][i-2]-u_1[k][j][i-3]-(um_1[k][j][i]-3*um_1[k][j][i-1]+3*um_1[k][j][i-2]-um_1[k][j][i-3]))) +  stry[j]*cox[i]*coz[k]*(
						// y-differences
						(rho[k][j+2][i]*dcy[j+2]+rho[k][j+1][i]*dcy[j+1])*(u_1[k][j+3][i]-3*u_1[k][j+2][i]+3*u_1[k][j+1][i]-u_1[k][j][i]-(um_1[k][j+3][i]-3*um_1[k][j+2][i]+3*um_1[k][j+1][i]-um_1[k][j][i]))-3*(rho[k][j+1][i]*dcy[j+1]+rho[k][j][i]*dcy[j])*(u_1[k][j+2][i] -3*u_1[k][j+1][i]+3*u_1[k][j][i]-u_1[k][j-1][i]-(um_1[k][j+2][i]-3*um_1[k][j+1][i]+3*um_1[k][j][i]-um_1[k][j-1][i]))+3*(rho[k][j][i]*dcy[j]+rho[k][j-1][i]*dcy[j-1])*(u_1[k][j+1][i]-3*u_1[k][j][i]+3*u_1[k][j-1][i]-u_1[k][j-2][i]-(um_1[k][j+1][i]-3*um_1[k][j][i]+3*um_1[k][j-1][i]-um_1[k][j-2][i]))-(rho[k][j-1][i]*dcy[j-1]+rho[k][j-2][i]*dcy[j-2])*(u_1[k][j][i]-3*u_1[k][j-1][i]+3*u_1[k][j-2][i]-u_1[k][j-3][i]-(um_1[k][j][i]-3*um_1[k][j-1][i]+3*um_1[k][j-2][i]-um_1[k][j-3][i]))) +  strz[k]*cox[i]*coy[j]*(
						// z-differences
						(rho[k+2][j][i]*dcz[k+2]+rho[k+1][j][i]*dcz[k+1])*(u_1[k+3][j][i]-3*u_1[k+2][j][i]+3*u_1[k+1][j][i]-u_1[k][j][i]-(um_1[k+3][j][i]-3*um_1[k+2][j][i]+3*um_1[k+1][j][i]-um_1[k][j][i]))-3*(rho[k+1][j][i]*dcz[k+1]+rho[k][j][i]*dcz[k])*(u_1[k+2][j][i]-3*u_1[k+1][j][i]+3*u_1[k][j][i]-u_1[k-1][j][i]-(um_1[k+2][j][i]-3*um_1[k+1][j][i]+3*um_1[k][j][i]-um_1[k-1][j][i]))+3*(rho[k][j][i]*dcz[k]+rho[k-1][j][i]*dcz[k-1])*(u_1[k+1][j][i]-3*u_1[k][j][i]+3*u_1[k-1][j][i]-u_1[k-2][j][i]-(um_1[k+1][j][i]-3*um_1[k][j][i]+3*um_1[k-1][j][i]-um_1[k-2][j][i]))-(rho[k-1][j][i]*dcz[k-1]+rho[k-2][j][i]*dcz[k-2])*(u_1[k][j][i]-3*u_1[k-1][j][i]+3*u_1[k-2][j][i]-u_1[k-3][j][i]-(um_1[k][j][i]-3*um_1[k-1][j][i]+3*um_1[k-2][j][i]-um_1[k-3][j][i]))));

				up_2[k][j][i] += birho*(strx[i]*coy[j]*coz[k]*(
						// x-differences
						(rho[k][j][i+2]*dcx[i+2]+rho[k][j][i+1]*dcx[i+1])*(u_2[k][j][i+3]-3*u_2[k][j][i+2]+3*u_2[k][j][i+1]-u_2[k][j][i]-(um_2[k][j][i+3]-3*um_2[k][j][i+2]+3*um_2[k][j][i+1]-um_2[k][j][i]))-3*(rho[k][j][i+1]*dcx[i+1]+rho[k][j][i]*dcx[i])*(u_2[k][j][i+2]-3*u_2[k][j][i+1]+3*u_2[k][j][i]-u_2[k][j][i-1]-(um_2[k][j][i+2]-3*um_2[k][j][i+1]+3*um_2[k][j][i]-um_2[k][j][i-1]))+3*(rho[k][j][i]*dcx[i]+rho[k][j][i-1]*dcx[i-1])*(u_2[k][j][i+1]-3*u_2[k][j][i]+3*u_2[k][j][i-1]-u_2[k][j][i-2]-(um_2[k][j][i+1]-3*um_2[k][j][i]+3*um_2[k][j][i-1]-um_2[k][j][i-2]))-(rho[k][j][i-1]*dcx[i-1]+rho[k][j][i-2]*dcx[i-2])*(u_2[k][j][i]-3*u_2[k][j][i-1]+3*u_2[k][j][i-2]-u_2[k][j][i-3]-(um_2[k][j][i]-3*um_2[k][j][i-1]+3*um_2[k][j][i-2]-um_2[k][j][i-3]))) +  stry[j]*cox[i]*coz[k]*(
						// y-differences
						(rho[k][j+2][i]*dcy[j+2]+rho[k][j+1][i]*dcy[j+1])*(u_2[k][j+3][i]-3*u_2[k][j+2][i]+3*u_2[k][j+1][i]-u_2[k][j][i]-(um_2[k][j+3][i]-3*um_2[k][j+2][i]+3*um_2[k][j+1][i]-um_2[k][j][i]))-3*(rho[k][j+1][i]*dcy[j+1]+rho[k][j][i]*dcy[j])*(u_2[k][j+2][i] -3*u_2[k][j+1][i]+3*u_2[k][j][i]-u_2[k][j-1][i]-(um_2[k][j+2][i]-3*um_2[k][j+1][i]+3*um_2[k][j][i]-um_2[k][j-1][i]))+3*(rho[k][j][i]*dcy[j]+rho[k][j-1][i]*dcy[j-1])*(u_2[k][j+1][i]-3*u_2[k][j][i]+3*u_2[k][j-1][i]-u_2[k][j-2][i]-(um_2[k][j+1][i]-3*um_2[k][j][i]+3*um_2[k][j-1][i]-um_2[k][j-2][i]))-(rho[k][j-1][i]*dcy[j-1]+rho[k][j-2][i]*dcy[j-2])*(u_2[k][j][i]-3*u_2[k][j-1][i]+3*u_2[k][j-2][i]-u_2[k][j-3][i]-(um_2[k][j][i]-3*um_2[k][j-1][i]+3*um_2[k][j-2][i]-um_2[k][j-3][i]))) +  strz[k]*cox[i]*coy[j]*(
						// z-differences
						(rho[k+2][j][i]*dcz[k+2]+rho[k+1][j][i]*dcz[k+1])*(u_2[k+3][j][i]-3*u_2[k+2][j][i]+3*u_2[k+1][j][i]-u_2[k][j][i]-(um_2[k+3][j][i]-3*um_2[k+2][j][i]+3*um_2[k+1][j][i]-um_2[k][j][i]))-3*(rho[k+1][j][i]*dcz[k+1]+rho[k][j][i]*dcz[k])*(u_2[k+2][j][i]-3*u_2[k+1][j][i]+3*u_2[k][j][i]-u_2[k-1][j][i]-(um_2[k+2][j][i]-3*um_2[k+1][j][i]+3*um_2[k][j][i]-um_2[k-1][j][i]))+3*(rho[k][j][i]*dcz[k]+rho[k-1][j][i]*dcz[k-1])*(u_2[k+1][j][i]-3*u_2[k][j][i]+3*u_2[k-1][j][i]-u_2[k-2][j][i]-(um_2[k+1][j][i]-3*um_2[k][j][i]+3*um_2[k-1][j][i]-um_2[k-2][j][i]))-(rho[k-1][j][i]*dcz[k-1]+rho[k-2][j][i]*dcz[k-2])*(u_2[k][j][i]-3*u_2[k-1][j][i]+3*u_2[k-2][j][i]-u_2[k-3][j][i]-(um_2[k][j][i]-3*um_2[k-1][j][i]+3*um_2[k-2][j][i]-um_2[k-3][j][i]))));
			}
		}
	}
}
