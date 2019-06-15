#define tf (3.0/4)
#define i6 (1.0/6)
#define i144 (1.0/144)
#define i12 (1.0/12)
#define c1 (2.0/3)
#define c2 (-(1.0/12))
#define a1 (1.0/31)

void sw4_gold (double *uacc_in_0, double *uacc_in_1, double *uacc_in_2, double *u_in_0, double *u_in_1, double *u_in_2, double *mu_in, double *la_in, double *met_in_0, double *met_in_1, double *met_in_2, double *met_in_3, double *jac_in, double *strx, double *stry, int N) {
	double (*uacc_0)[320][320] = (double (*)[320][320])uacc_in_0;
	double (*uacc_1)[320][320] = (double (*)[320][320])uacc_in_1;
	double (*uacc_2)[320][320] = (double (*)[320][320])uacc_in_2;
	double (*u_0)[320][320] = (double (*)[320][320])u_in_0;
	double (*u_1)[320][320] = (double (*)[320][320])u_in_1;
	double (*u_2)[320][320] = (double (*)[320][320])u_in_2;
	double (*mu)[320][320] = (double (*)[320][320])mu_in;
	double (*la)[320][320] = (double (*)[320][320])la_in;
	double (*met_0)[320][320] = (double (*)[320][320])met_in_0;
	double (*met_1)[320][320] = (double (*)[320][320])met_in_1;
	double (*met_2)[320][320] = (double (*)[320][320])met_in_2;
	double (*met_3)[320][320] = (double (*)[320][320])met_in_3;
	double (*jac)[320][320] = (double (*)[320][320])jac_in;

	int i, j, k;
	/* Assumptions */

#pragma omp parallel for private(j,i)
	for (k = 2; k < N-2; k++) {
		for (j = 2; j < N-2; j++) {
			for (i = 2; i < N-2; i++) {
				// 5 ops
				double ijac = strx[i]*stry[j]/jac[k][j][i];
				double istry = 1/(stry[j]);
				double istrx = 1/(strx[i]);
				double istrxy = istry*istrx;

				double r1=0, r2=0, r3=0;

				// pp derivative (u)
				// 53 ops, tot=58
				double cof1=(2*mu[k][j][i-2]+la[k][j][i-2])*met_0[k][j][i-2]*met_0[k][j][i-2]*strx[i-2];
				double cof2=(2*mu[k][j][i-1]+la[k][j][i-1])*met_0[k][j][i-1]*met_0[k][j][i-1]*strx[i-1];
				double cof3=(2*mu[k][j][i]+la[k][j][i])*met_0[k][j][i]*met_0[k][j][i]*strx[i];
				double cof4=(2*mu[k][j][i+1]+la[k][j][i+1])*met_0[k][j][i+1]*met_0[k][j][i+1]*strx[i+1];
				double cof5=(2*mu[k][j][i+2]+la[k][j][i+2])*met_0[k][j][i+2]*met_0[k][j][i+2]*strx[i+2];
				double mux1 = cof2 -tf*(cof3+cof1);
				double mux2 = cof1 + cof4+3*(cof3+cof2);
				double mux3 = cof2 + cof5+3*(cof4+cof3);
				double mux4 = cof4-tf*(cof3+cof5);

				r1 +=  i6* (mux1*(u_0[k][j][i-2]-u_0[k][j][i]) + mux2*(u_0[k][j][i-1]-u_0[k][j][i]) + mux3*(u_0[k][j][i+1]-u_0[k][j][i]) + mux4*(u_0[k][j][i+2]-u_0[k][j][i]))*istry;
				// qq derivative (u)
				// 43 ops, tot=101
				cof1=(mu[k][j-2][i])*met_0[k][j-2][i]*met_0[k][j-2][i]*stry[j-2];
				cof2=(mu[k][j-1][i])*met_0[k][j-1][i]*met_0[k][j-1][i]*stry[j-1];
				cof3=(mu[k][j][i])*met_0[k][j][i]*met_0[k][j][i]*stry[j];
				cof4=(mu[k][j+1][i])*met_0[k][j+1][i]*met_0[k][j+1][i]*stry[j+1];
				cof5=(mu[k][j+2][i])*met_0[k][j+2][i]*met_0[k][j+2][i]*stry[j+2];
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r1 += i6* (mux1*(u_0[k][j-2][i]-u_0[k][j][i]) + mux2*(u_0[k][j-1][i]-u_0[k][j][i]) + mux3*(u_0[k][j+1][i]-u_0[k][j][i]) + mux4*(u_0[k][j+2][i]-u_0[k][j][i]))*istrx;
				// rr derivative (u)
				// 5*11+14+14=83 ops, tot=184
				cof1 = (2*mu[k-2][j][i]+la[k-2][j][i])*met_1[k-2][j][i]*strx[i]*met_1[k-2][j][i]*strx[i] + mu[k-2][j][i]*(met_2[k-2][j][i]*stry[j]*met_2[k-2][j][i]*stry[j] + met_3[k-2][j][i]*met_3[k-2][j][i]);
				cof2 = (2*mu[k-1][j][i]+la[k-1][j][i])*met_1[k-1][j][i]*strx[i]*met_1[k-1][j][i]*strx[i] + mu[k-1][j][i]*(met_2[k-1][j][i]*stry[j]*met_2[k-1][j][i]*stry[j] + met_3[k-1][j][i]*met_3[k-1][j][i]);
				cof3 = (2*mu[k][j][i]+la[k][j][i])*met_1[k][j][i]*strx[i]*met_1[k][j][i]*strx[i] + mu[k][j][i]*(met_2[k][j][i]*stry[j]*met_2[k][j][i]*stry[j] + met_3[k][j][i]*met_3[k][j][i]);
				cof4 = (2*mu[k+1][j][i]+la[k+1][j][i])*met_1[k+1][j][i]*strx[i]*met_1[k+1][j][i]*strx[i] + mu[k+1][j][i]*(met_2[k+1][j][i]*stry[j]*met_2[k+1][j][i]*stry[j] + met_3[k+1][j][i]*met_3[k+1][j][i]);
				cof5 = (2*mu[k+2][j][i]+la[k+2][j][i])*met_1[k+2][j][i]*strx[i]*met_1[k+2][j][i]*strx[i] + mu[k+2][j][i]*( met_2[k+2][j][i]*stry[j]*met_2[k+2][j][i]*stry[j] + met_3[k+2][j][i]*met_3[k+2][j][i]);

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r1 += i6* (mux1*(u_0[k-2][j][i]-u_0[k][j][i]) + 
						mux2*(u_0[k-1][j][i]-u_0[k][j][i]) + 
						mux3*(u_0[k+1][j][i]-u_0[k][j][i]) +
						mux4*(u_0[k+2][j][i]-u_0[k][j][i])  )*istrxy;

				// rr derivative (v)
				// 42 ops, tot=226
				cof1=(mu[k-2][j][i]+la[k-2][j][i])*met_1[k-2][j][i]*met_2[k-2][j][i];
				cof2=(mu[k-1][j][i]+la[k-1][j][i])*met_1[k-1][j][i]*met_2[k-1][j][i];
				cof3=(mu[k][j][i]+la[k][j][i])*met_1[k][j][i]*met_2[k][j][i];
				cof4=(mu[k+1][j][i]+la[k+1][j][i])*met_1[k+1][j][i]*met_2[k+1][j][i];
				cof5=(mu[k+2][j][i]+la[k+2][j][i])*met_1[k+2][j][i]*met_2[k+2][j][i];
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r1 += i6* (
						mux1*(u_1[k-2][j][i]-u_1[k][j][i]) + 
						mux2*(u_1[k-1][j][i]-u_1[k][j][i]) + 
						mux3*(u_1[k+1][j][i]-u_1[k][j][i]) +
						mux4*(u_1[k+2][j][i]-u_1[k][j][i])  );

				// rr derivative (w)
				// 43 ops, tot=269
				cof1=(mu[k-2][j][i]+la[k-2][j][i])*met_1[k-2][j][i]*met_3[k-2][j][i];
				cof2=(mu[k-1][j][i]+la[k-1][j][i])*met_1[k-1][j][i]*met_3[k-1][j][i];
				cof3=(mu[k][j][i]+la[k][j][i])*met_1[k][j][i]*met_3[k][j][i];
				cof4=(mu[k+1][j][i]+la[k+1][j][i])*met_1[k+1][j][i]*met_3[k+1][j][i];
				cof5=(mu[k+2][j][i]+la[k+2][j][i])*met_1[k+2][j][i]*met_3[k+2][j][i];
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r1 += i6* (
						mux1*(u_2[k-2][j][i]-u_2[k][j][i]) + 
						mux2*(u_2[k-1][j][i]-u_2[k][j][i]) + 
						mux3*(u_2[k+1][j][i]-u_2[k][j][i]) +
						mux4*(u_2[k+2][j][i]-u_2[k][j][i])  )*istry;

				// pq-derivatives
				// 38 ops, tot=307
				r1 += 
					c2*(  mu[k][j+2][i]*met_0[k][j+2][i]*met_0[k][j+2][i]*(
								c2*(u_1[k][j+2][i+2]-u_1[k][j+2][i-2]) +
								c1*(u_1[k][j+2][i+1]-u_1[k][j+2][i-1])    )
							- mu[k][j-2][i]*met_0[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_1[k][j-2][i+2]-u_1[k][j-2][i-2])+
								c1*(u_1[k][j-2][i+1]-u_1[k][j-2][i-1])     )
					   ) +
					c1*(  mu[k][j+1][i]*met_0[k][j+1][i]*met_0[k][j+1][i]*(
								c2*(u_1[k][j+1][i+2]-u_1[k][j+1][i-2]) +
								c1*(u_1[k][j+1][i+1]-u_1[k][j+1][i-1])  )
							- mu[k][j-1][i]*met_0[k][j-1][i]*met_0[k][j-1][i]*(
								c2*(u_1[k][j-1][i+2]-u_1[k][j-1][i-2]) + 
								c1*(u_1[k][j-1][i+1]-u_1[k][j-1][i-1])));

				// qp-derivatives
				// 38 ops, tot=345
				r1 += 
					c2*(  la[k][j][i+2]*met_0[k][j][i+2]*met_0[k][j][i+2]*(
								c2*(u_1[k][j+2][i+2]-u_1[k][j-2][i+2]) +
								c1*(u_1[k][j+1][i+2]-u_1[k][j-1][i+2])    )
							- la[k][j][i-2]*met_0[k][j][i-2]*met_0[k][j][i-2]*(
								c2*(u_1[k][j+2][i-2]-u_1[k][j-2][i-2])+
								c1*(u_1[k][j+1][i-2]-u_1[k][j-1][i-2])     )
					   ) +
					c1*(  la[k][j][i+1]*met_0[k][j][i+1]*met_0[k][j][i+1]*(
								c2*(u_1[k][j+2][i+1]-u_1[k][j-2][i+1]) +
								c1*(u_1[k][j+1][i+1]-u_1[k][j-1][i+1])  )
							- la[k][j][i-1]*met_0[k][j][i-1]*met_0[k][j][i-1]*(
								c2*(u_1[k][j+2][i-1]-u_1[k][j-2][i-1]) + 
								c1*(u_1[k][j+1][i-1]-u_1[k][j-1][i-1])));

				// pr-derivatives
				// 130 ops., tot=475
				r1 += c2*(
						(2*mu[k+2][j][i]+la[k+2][j][i])*met_1[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_0[k+2][j][i+2]-u_0[k+2][j][i-2]) +
							c1*(u_0[k+2][j][i+1]-u_0[k+2][j][i-1])   )*strx[i]*istry 
						+ mu[k+2][j][i]*met_2[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_1[k+2][j][i+2]-u_1[k+2][j][i-2]) +
							c1*(u_1[k+2][j][i+1]-u_1[k+2][j][i-1])  ) 
						+ mu[k+2][j][i]*met_3[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_2[k+2][j][i+2]-u_2[k+2][j][i-2]) +
							c1*(u_2[k+2][j][i+1]-u_2[k+2][j][i-1])  )*istry
						- ((2*mu[k-2][j][i]+la[k-2][j][i])*met_1[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_0[k-2][j][i+2]-u_0[k-2][j][i-2]) +
								c1*(u_0[k-2][j][i+1]-u_0[k-2][j][i-1])  )*strx[i]*istry  
							+ mu[k-2][j][i]*met_2[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_1[k-2][j][i+2]-u_1[k-2][j][i-2]) +
								c1*(u_1[k-2][j][i+1]-u_1[k-2][j][i-1])   ) 
							+ mu[k-2][j][i]*met_3[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_2[k-2][j][i+2]-u_2[k-2][j][i-2]) +
								c1*(u_2[k-2][j][i+1]-u_2[k-2][j][i-1])   )*istry )
					 ) + c1*(  
						 (2*mu[k+1][j][i]+la[k+1][j][i])*met_1[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_0[k+1][j][i+2]-u_0[k+1][j][i-2]) +
							 c1*(u_0[k+1][j][i+1]-u_0[k+1][j][i-1]) )*strx[i]*istry 
						 + mu[k+1][j][i]*met_2[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_1[k+1][j][i+2]-u_1[k+1][j][i-2]) +
							 c1*(u_1[k+1][j][i+1]-u_1[k+1][j][i-1]) ) 
						 + mu[k+1][j][i]*met_3[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_2[k+1][j][i+2]-u_2[k+1][j][i-2]) +
							 c1*(u_2[k+1][j][i+1]-u_2[k+1][j][i-1])  )*istry
						 - ((2*mu[k-1][j][i]+la[k-1][j][i])*met_1[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_0[k-1][j][i+2]-u_0[k-1][j][i-2]) +
								 c1*(u_0[k-1][j][i+1]-u_0[k-1][j][i-1]) )*strx[i]*istry  
							 + mu[k-1][j][i]*met_2[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_1[k-1][j][i+2]-u_1[k-1][j][i-2]) +
								 c1*(u_1[k-1][j][i+1]-u_1[k-1][j][i-1]) ) 
							 + mu[k-1][j][i]*met_3[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_2[k-1][j][i+2]-u_2[k-1][j][i-2]) +
								 c1*(u_2[k-1][j][i+1]-u_2[k-1][j][i-1])   )*istry  ) );

				// rp derivatives
				// 130 ops, tot=605
				r1 += ( c2*(
							(2*mu[k][j][i+2]+la[k][j][i+2])*met_1[k][j][i+2]*met_0[k][j][i+2]*(
								c2*(u_0[k+2][j][i+2]-u_0[k-2][j][i+2]) +
								c1*(u_0[k+1][j][i+2]-u_0[k-1][j][i+2])   )*strx[i+2] 
							+ la[k][j][i+2]*met_2[k][j][i+2]*met_0[k][j][i+2]*(
								c2*(u_1[k+2][j][i+2]-u_1[k-2][j][i+2]) +
								c1*(u_1[k+1][j][i+2]-u_1[k-1][j][i+2])  )*stry[j] 
							+ la[k][j][i+2]*met_3[k][j][i+2]*met_0[k][j][i+2]*(
								c2*(u_2[k+2][j][i+2]-u_2[k-2][j][i+2]) +
								c1*(u_2[k+1][j][i+2]-u_2[k-1][j][i+2])  )
							- ((2*mu[k][j][i-2]+la[k][j][i-2])*met_1[k][j][i-2]*met_0[k][j][i-2]*(
									c2*(u_0[k+2][j][i-2]-u_0[k-2][j][i-2]) +
									c1*(u_0[k+1][j][i-2]-u_0[k-1][j][i-2])  )*strx[i-2] 
								+ la[k][j][i-2]*met_2[k][j][i-2]*met_0[k][j][i-2]*(
									c2*(u_1[k+2][j][i-2]-u_1[k-2][j][i-2]) +
									c1*(u_1[k+1][j][i-2]-u_1[k-1][j][i-2])   )*stry[j] 
								+ la[k][j][i-2]*met_3[k][j][i-2]*met_0[k][j][i-2]*(
									c2*(u_2[k+2][j][i-2]-u_2[k-2][j][i-2]) +
									c1*(u_2[k+1][j][i-2]-u_2[k-1][j][i-2])   ) )
					   ) + c1*(  
						   (2*mu[k][j][i+1]+la[k][j][i+1])*met_1[k][j][i+1]*met_0[k][j][i+1]*(
							   c2*(u_0[k+2][j][i+1]-u_0[k-2][j][i+1]) +
							   c1*(u_0[k+1][j][i+1]-u_0[k-1][j][i+1]) )*strx[i+1] 
						   + la[k][j][i+1]*met_2[k][j][i+1]*met_0[k][j][i+1]*(
							   c2*(u_1[k+2][j][i+1]-u_1[k-2][j][i+1]) +
							   c1*(u_1[k+1][j][i+1]-u_1[k-1][j][i+1]) )*stry[j] 
						   + la[k][j][i+1]*met_3[k][j][i+1]*met_0[k][j][i+1]*(
							   c2*(u_2[k+2][j][i+1]-u_2[k-2][j][i+1]) +
							   c1*(u_2[k+1][j][i+1]-u_2[k-1][j][i+1])  )
						   - ((2*mu[k][j][i-1]+la[k][j][i-1])*met_1[k][j][i-1]*met_0[k][j][i-1]*(
								   c2*(u_0[k+2][j][i-1]-u_0[k-2][j][i-1]) +
								   c1*(u_0[k+1][j][i-1]-u_0[k-1][j][i-1]) )*strx[i-1] 
							   + la[k][j][i-1]*met_2[k][j][i-1]*met_0[k][j][i-1]*(
								   c2*(u_1[k+2][j][i-1]-u_1[k-2][j][i-1]) +
								   c1*(u_1[k+1][j][i-1]-u_1[k-1][j][i-1]) )*stry[j] 
							   + la[k][j][i-1]*met_3[k][j][i-1]*met_0[k][j][i-1]*(
								   c2*(u_2[k+2][j][i-1]-u_2[k-2][j][i-1]) +
								   c1*(u_2[k+1][j][i-1]-u_2[k-1][j][i-1])   )  ) ) )*istry;

				// qr derivatives
				// 82 ops, tot=687
				r1 += c2*(
						mu[k+2][j][i]*met_2[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_0[k+2][j+2][i]-u_0[k+2][j-2][i]) +
							c1*(u_0[k+2][j+1][i]-u_0[k+2][j-1][i])   )*stry[j]*istrx 
						+ la[k+2][j][i]*met_1[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_1[k+2][j+2][i]-u_1[k+2][j-2][i]) +
							c1*(u_1[k+2][j+1][i]-u_1[k+2][j-1][i])  ) 
						- ( mu[k-2][j][i]*met_2[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_0[k-2][j+2][i]-u_0[k-2][j-2][i]) +
								c1*(u_0[k-2][j+1][i]-u_0[k-2][j-1][i])  )*stry[j]*istrx  
							+ la[k-2][j][i]*met_1[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_1[k-2][j+2][i]-u_1[k-2][j-2][i]) +
								c1*(u_1[k-2][j+1][i]-u_1[k-2][j-1][i])   ) ) 
					 ) + c1*(  
						 mu[k+1][j][i]*met_2[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_0[k+1][j+2][i]-u_0[k+1][j-2][i]) +
							 c1*(u_0[k+1][j+1][i]-u_0[k+1][j-1][i]) )*stry[j]*istrx  
						 + la[k+1][j][i]*met_1[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_1[k+1][j+2][i]-u_1[k+1][j-2][i]) +
							 c1*(u_1[k+1][j+1][i]-u_1[k+1][j-1][i]) )  
						 - ( mu[k-1][j][i]*met_2[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_0[k-1][j+2][i]-u_0[k-1][j-2][i]) +
								 c1*(u_0[k-1][j+1][i]-u_0[k-1][j-1][i]) )*stry[j]*istrx  
							 + la[k-1][j][i]*met_1[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_1[k-1][j+2][i]-u_1[k-1][j-2][i]) +
								 c1*(u_1[k-1][j+1][i]-u_1[k-1][j-1][i]) ) ) );

				// rq derivatives
				// 82 ops, tot=769
				r1 += c2*(
						mu[k][j+2][i]*met_2[k][j+2][i]*met_0[k][j+2][i]*(
							c2*(u_0[k+2][j+2][i]-u_0[k-2][j+2][i]) +
							c1*(u_0[k+1][j+2][i]-u_0[k-1][j+2][i])   )*stry[j+2]*istrx 
						+ mu[k][j+2][i]*met_1[k][j+2][i]*met_0[k][j+2][i]*(
							c2*(u_1[k+2][j+2][i]-u_1[k-2][j+2][i]) +
							c1*(u_1[k+1][j+2][i]-u_1[k-1][j+2][i])  ) 
						- ( mu[k][j-2][i]*met_2[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_0[k+2][j-2][i]-u_0[k-2][j-2][i]) +
								c1*(u_0[k+1][j-2][i]-u_0[k-1][j-2][i])  )*stry[j-2]*istrx  
							+ mu[k][j-2][i]*met_1[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_1[k+2][j-2][i]-u_1[k-2][j-2][i]) +
								c1*(u_1[k+1][j-2][i]-u_1[k-1][j-2][i])   ) ) 
					 ) + c1*(  
						 mu[k][j+1][i]*met_2[k][j+1][i]*met_0[k][j+1][i]*(
							 c2*(u_0[k+2][j+1][i]-u_0[k-2][j+1][i]) +
							 c1*(u_0[k+1][j+1][i]-u_0[k-1][j+1][i]) )*stry[j+1]*istrx
						 + mu[k][j+1][i]*met_1[k][j+1][i]*met_0[k][j+1][i]*(
							 c2*(u_1[k+2][j+1][i]-u_1[k-2][j+1][i]) +
							 c1*(u_1[k+1][j+1][i]-u_1[k-1][j+1][i]) )
						 - ( mu[k][j-1][i]*met_2[k][j-1][i]*met_0[k][j-1][i]*(
								 c2*(u_0[k+2][j-1][i]-u_0[k-2][j-1][i]) +
								 c1*(u_0[k+1][j-1][i]-u_0[k-1][j-1][i]) )*stry[j-1]*istrx    
							 + mu[k][j-1][i]*met_1[k][j-1][i]*met_0[k][j-1][i]*(
								 c2*(u_1[k+2][j-1][i]-u_1[k-2][j-1][i]) +
								 c1*(u_1[k+1][j-1][i]-u_1[k-1][j-1][i]) ) ) );

				// 4 ops, tot=773
				uacc_0[k][j][i] = a1*uacc_0[k][j][i] + r1*ijac;
				// v-equation

				//	    r1 = 0;
				// pp derivative (v)
				// 43 ops, tot=816
				cof1=(mu[k][j][i-2])*met_0[k][j][i-2]*met_0[k][j][i-2]*strx[i-2];
				cof2=(mu[k][j][i-1])*met_0[k][j][i-1]*met_0[k][j][i-1]*strx[i-1];
				cof3=(mu[k][j][i])*met_0[k][j][i]*met_0[k][j][i]*strx[i];
				cof4=(mu[k][j][i+1])*met_0[k][j][i+1]*met_0[k][j][i+1]*strx[i+1];
				cof5=(mu[k][j][i+2])*met_0[k][j][i+2]*met_0[k][j][i+2]*strx[i+2];

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r2 += i6* (
						mux1*(u_1[k][j][i-2]-u_1[k][j][i]) + 
						mux2*(u_1[k][j][i-1]-u_1[k][j][i]) + 
						mux3*(u_1[k][j][i+1]-u_1[k][j][i]) +
						mux4*(u_1[k][j][i+2]-u_1[k][j][i])  )*istry;

				// qq derivative (v)
				// 53 ops, tot=869
				cof1=(2*mu[k][j-2][i]+la[k][j-2][i])*met_0[k][j-2][i]*met_0[k][j-2][i]
					*stry[j-2];
				cof2=(2*mu[k][j-1][i]+la[k][j-1][i])*met_0[k][j-1][i]*met_0[k][j-1][i]
					*stry[j-1];
				cof3=(2*mu[k][j][i]+la[k][j][i])*met_0[k][j][i]*met_0[k][j][i]
					*stry[j];
				cof4=(2*mu[k][j+1][i]+la[k][j+1][i])*met_0[k][j+1][i]*met_0[k][j+1][i]
					*stry[j+1];
				cof5=(2*mu[k][j+2][i]+la[k][j+2][i])*met_0[k][j+2][i]*met_0[k][j+2][i]
					*stry[j+2];
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r2 += i6* (
						mux1*(u_1[k][j-2][i]-u_1[k][j][i]) + 
						mux2*(u_1[k][j-1][i]-u_1[k][j][i]) + 
						mux3*(u_1[k][j+1][i]-u_1[k][j][i]) +
						mux4*(u_1[k][j+2][i]-u_1[k][j][i])  )*istrx;

				// rr derivative (u)
				// 42 ops, tot=911
				cof1=(mu[k-2][j][i]+la[k-2][j][i])*met_1[k-2][j][i]*met_2[k-2][j][i];
				cof2=(mu[k-1][j][i]+la[k-1][j][i])*met_1[k-1][j][i]*met_2[k-1][j][i];
				cof3=(mu[k][j][i]+  la[k][j][i]  )*met_1[k][j][i]*  met_2[k][j][i];
				cof4=(mu[k+1][j][i]+la[k+1][j][i])*met_1[k+1][j][i]*met_2[k+1][j][i];
				cof5=(mu[k+2][j][i]+la[k+2][j][i])*met_1[k+2][j][i]*met_2[k+2][j][i];

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r2 += i6* (
						mux1*(u_0[k-2][j][i]-u_0[k][j][i]) + 
						mux2*(u_0[k-1][j][i]-u_0[k][j][i]) + 
						mux3*(u_0[k+1][j][i]-u_0[k][j][i]) +
						mux4*(u_0[k+2][j][i]-u_0[k][j][i])  );

				// rr derivative (v)
				// 83 ops, tot=994
				cof1 = (2*mu[k-2][j][i]+la[k-2][j][i])*met_2[k-2][j][i]*stry[j]*met_2[k-2][j][i]*stry[j]
					+    mu[k-2][j][i]*(met_1[k-2][j][i]*strx[i]*met_1[k-2][j][i]*strx[i]+
							met_3[k-2][j][i]*met_3[k-2][j][i]);
				cof2 = (2*mu[k-1][j][i]+la[k-1][j][i])*met_2[k-1][j][i]*stry[j]*met_2[k-1][j][i]*stry[j]
					+    mu[k-1][j][i]*(met_1[k-1][j][i]*strx[i]*met_1[k-1][j][i]*strx[i]+
							met_3[k-1][j][i]*met_3[k-1][j][i]);
				cof3 = (2*mu[k][j][i]+la[k][j][i])*met_2[k][j][i]*stry[j]*met_2[k][j][i]*stry[j] +
					mu[k][j][i]*(met_1[k][j][i]*strx[i]*met_1[k][j][i]*strx[i]+
							met_3[k][j][i]*met_3[k][j][i]);
				cof4 = (2*mu[k+1][j][i]+la[k+1][j][i])*met_2[k+1][j][i]*stry[j]*met_2[k+1][j][i]*stry[j]
					+    mu[k+1][j][i]*(met_1[k+1][j][i]*strx[i]*met_1[k+1][j][i]*strx[i]+
							met_3[k+1][j][i]*met_3[k+1][j][i]);
				cof5 = (2*mu[k+2][j][i]+la[k+2][j][i])*met_2[k+2][j][i]*stry[j]*met_2[k+2][j][i]*stry[j]
					+    mu[k+2][j][i]*(met_1[k+2][j][i]*strx[i]*met_1[k+2][j][i]*strx[i]+
							met_3[k+2][j][i]*met_3[k+2][j][i]);

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r2 += i6* (
						mux1*(u_1[k-2][j][i]-u_1[k][j][i]) + 
						mux2*(u_1[k-1][j][i]-u_1[k][j][i]) + 
						mux3*(u_1[k+1][j][i]-u_1[k][j][i]) +
						mux4*(u_1[k+2][j][i]-u_1[k][j][i])  )*istrxy;

				// rr derivative (w)
				// 43 ops, tot=1037
				cof1=(mu[k-2][j][i]+la[k-2][j][i])*met_2[k-2][j][i]*met_3[k-2][j][i];
				cof2=(mu[k-1][j][i]+la[k-1][j][i])*met_2[k-1][j][i]*met_3[k-1][j][i];
				cof3=(mu[k][j][i]  +la[k][j][i]  )*met_2[k][j][i]*  met_3[k][j][i];
				cof4=(mu[k+1][j][i]+la[k+1][j][i])*met_2[k+1][j][i]*met_3[k+1][j][i];
				cof5=(mu[k+2][j][i]+la[k+2][j][i])*met_2[k+2][j][i]*met_3[k+2][j][i];
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r2 += i6* (
						mux1*(u_2[k-2][j][i]-u_2[k][j][i]) + 
						mux2*(u_2[k-1][j][i]-u_2[k][j][i]) + 
						mux3*(u_2[k+1][j][i]-u_2[k][j][i]) +
						mux4*(u_2[k+2][j][i]-u_2[k][j][i])  )*istrx;

				// pq-derivatives
				// 38 ops, tot=1075
				r2 += 
					c2*(  la[k][j+2][i]*met_0[k][j+2][i]*met_0[k][j+2][i]*(
								c2*(u_0[k][j+2][i+2]-u_0[k][j+2][i-2]) +
								c1*(u_0[k][j+2][i+1]-u_0[k][j+2][i-1])    )
							- la[k][j-2][i]*met_0[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_0[k][j-2][i+2]-u_0[k][j-2][i-2])+
								c1*(u_0[k][j-2][i+1]-u_0[k][j-2][i-1])     )
					   ) +
					c1*(  la[k][j+1][i]*met_0[k][j+1][i]*met_0[k][j+1][i]*(
								c2*(u_0[k][j+1][i+2]-u_0[k][j+1][i-2]) +
								c1*(u_0[k][j+1][i+1]-u_0[k][j+1][i-1])  )
							- la[k][j-1][i]*met_0[k][j-1][i]*met_0[k][j-1][i]*(
								c2*(u_0[k][j-1][i+2]-u_0[k][j-1][i-2]) + 
								c1*(u_0[k][j-1][i+1]-u_0[k][j-1][i-1])));

				// qp-derivatives
				// 38 ops, tot=1113
				r2 += 
					c2*(  mu[k][j][i+2]*met_0[k][j][i+2]*met_0[k][j][i+2]*(
								c2*(u_0[k][j+2][i+2]-u_0[k][j-2][i+2]) +
								c1*(u_0[k][j+1][i+2]-u_0[k][j-1][i+2])    )
							- mu[k][j][i-2]*met_0[k][j][i-2]*met_0[k][j][i-2]*(
								c2*(u_0[k][j+2][i-2]-u_0[k][j-2][i-2])+
								c1*(u_0[k][j+1][i-2]-u_0[k][j-1][i-2])     )
					   ) +
					c1*(  mu[k][j][i+1]*met_0[k][j][i+1]*met_0[k][j][i+1]*(
								c2*(u_0[k][j+2][i+1]-u_0[k][j-2][i+1]) +
								c1*(u_0[k][j+1][i+1]-u_0[k][j-1][i+1])  )
							- mu[k][j][i-1]*met_0[k][j][i-1]*met_0[k][j][i-1]*(
								c2*(u_0[k][j+2][i-1]-u_0[k][j-2][i-1]) + 
								c1*(u_0[k][j+1][i-1]-u_0[k][j-1][i-1])));

				// pr-derivatives
				// 82 ops, tot=1195
				r2 += c2*(
						(la[k+2][j][i])*met_2[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_0[k+2][j][i+2]-u_0[k+2][j][i-2]) +
							c1*(u_0[k+2][j][i+1]-u_0[k+2][j][i-1])   ) 
						+ mu[k+2][j][i]*met_1[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_1[k+2][j][i+2]-u_1[k+2][j][i-2]) +
							c1*(u_1[k+2][j][i+1]-u_1[k+2][j][i-1])  )*strx[i]*istry 
						- ((la[k-2][j][i])*met_2[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_0[k-2][j][i+2]-u_0[k-2][j][i-2]) +
								c1*(u_0[k-2][j][i+1]-u_0[k-2][j][i-1])  ) 
							+ mu[k-2][j][i]*met_1[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_1[k-2][j][i+2]-u_1[k-2][j][i-2]) +
								c1*(u_1[k-2][j][i+1]-u_1[k-2][j][i-1]) )*strx[i]*istry ) 
					 ) + c1*(  
						 (la[k+1][j][i])*met_2[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_0[k+1][j][i+2]-u_0[k+1][j][i-2]) +
							 c1*(u_0[k+1][j][i+1]-u_0[k+1][j][i-1]) ) 
						 + mu[k+1][j][i]*met_1[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_1[k+1][j][i+2]-u_1[k+1][j][i-2]) +
							 c1*(u_1[k+1][j][i+1]-u_1[k+1][j][i-1]) )*strx[i]*istry  
						 - (la[k-1][j][i]*met_2[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_0[k-1][j][i+2]-u_0[k-1][j][i-2]) +
								 c1*(u_0[k-1][j][i+1]-u_0[k-1][j][i-1]) ) 
							 + mu[k-1][j][i]*met_1[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_1[k-1][j][i+2]-u_1[k-1][j][i-2]) +
								 c1*(u_1[k-1][j][i+1]-u_1[k-1][j][i-1]) )*strx[i]*istry  ) );

				// rp derivatives
				// 82 ops, tot=1277
				r2 += c2*(
						(mu[k][j][i+2])*met_2[k][j][i+2]*met_0[k][j][i+2]*(
							c2*(u_0[k+2][j][i+2]-u_0[k-2][j][i+2]) +
							c1*(u_0[k+1][j][i+2]-u_0[k-1][j][i+2])   ) 
						+ mu[k][j][i+2]*met_1[k][j][i+2]*met_0[k][j][i+2]*(
							c2*(u_1[k+2][j][i+2]-u_1[k-2][j][i+2]) +
							c1*(u_1[k+1][j][i+2]-u_1[k-1][j][i+2])  )*strx[i+2]*istry 
						- (mu[k][j][i-2]*met_2[k][j][i-2]*met_0[k][j][i-2]*(
								c2*(u_0[k+2][j][i-2]-u_0[k-2][j][i-2]) +
								c1*(u_0[k+1][j][i-2]-u_0[k-1][j][i-2])  )
							+ mu[k][j][i-2]*met_1[k][j][i-2]*met_0[k][j][i-2]*(
								c2*(u_1[k+2][j][i-2]-u_1[k-2][j][i-2]) +
								c1*(u_1[k+1][j][i-2]-u_1[k-1][j][i-2])   )*strx[i-2]*istry )
					 ) + c1*(  
						 (mu[k][j][i+1])*met_2[k][j][i+1]*met_0[k][j][i+1]*(
							 c2*(u_0[k+2][j][i+1]-u_0[k-2][j][i+1]) +
							 c1*(u_0[k+1][j][i+1]-u_0[k-1][j][i+1]) ) 
						 + mu[k][j][i+1]*met_1[k][j][i+1]*met_0[k][j][i+1]*(
							 c2*(u_1[k+2][j][i+1]-u_1[k-2][j][i+1]) +
							 c1*(u_1[k+1][j][i+1]-u_1[k-1][j][i+1]) )*strx[i+1]*istry 
						 - (mu[k][j][i-1]*met_2[k][j][i-1]*met_0[k][j][i-1]*(
								 c2*(u_0[k+2][j][i-1]-u_0[k-2][j][i-1]) +
								 c1*(u_0[k+1][j][i-1]-u_0[k-1][j][i-1]) ) 
							 + mu[k][j][i-1]*met_1[k][j][i-1]*met_0[k][j][i-1]*(
								 c2*(u_1[k+2][j][i-1]-u_1[k-2][j][i-1]) +
								 c1*(u_1[k+1][j][i-1]-u_1[k-1][j][i-1]) )*strx[i-1]*istry  ) );

				// qr derivatives
				// 130 ops, tot=1407
				r2 += c2*(
						mu[k+2][j][i]*met_1[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_0[k+2][j+2][i]-u_0[k+2][j-2][i]) +
							c1*(u_0[k+2][j+1][i]-u_0[k+2][j-1][i])   ) 
						+ (2*mu[k+2][j][i]+la[k+2][j][i])*met_2[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_1[k+2][j+2][i]-u_1[k+2][j-2][i]) +
							c1*(u_1[k+2][j+1][i]-u_1[k+2][j-1][i])  )*stry[j]*istrx 
						+mu[k+2][j][i]*met_3[k+2][j][i]*met_0[k+2][j][i]*(
							c2*(u_2[k+2][j+2][i]-u_2[k+2][j-2][i]) +
							c1*(u_2[k+2][j+1][i]-u_2[k+2][j-1][i])   )*istrx 
						- ( mu[k-2][j][i]*met_1[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_0[k-2][j+2][i]-u_0[k-2][j-2][i]) +
								c1*(u_0[k-2][j+1][i]-u_0[k-2][j-1][i])  ) 
							+(2*mu[k-2][j][i]+ la[k-2][j][i])*met_2[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_1[k-2][j+2][i]-u_1[k-2][j-2][i]) +
								c1*(u_1[k-2][j+1][i]-u_1[k-2][j-1][i])   )*stry[j]*istrx +
							mu[k-2][j][i]*met_3[k-2][j][i]*met_0[k-2][j][i]*(
								c2*(u_2[k-2][j+2][i]-u_2[k-2][j-2][i]) +
								c1*(u_2[k-2][j+1][i]-u_2[k-2][j-1][i])  )*istrx ) 
					 ) + c1*(  
						 mu[k+1][j][i]*met_1[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_0[k+1][j+2][i]-u_0[k+1][j-2][i]) +
							 c1*(u_0[k+1][j+1][i]-u_0[k+1][j-1][i]) ) 
						 + (2*mu[k+1][j][i]+la[k+1][j][i])*met_2[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_1[k+1][j+2][i]-u_1[k+1][j-2][i]) +
							 c1*(u_1[k+1][j+1][i]-u_1[k+1][j-1][i]) )*stry[j]*istrx
						 + mu[k+1][j][i]*met_3[k+1][j][i]*met_0[k+1][j][i]*(
							 c2*(u_2[k+1][j+2][i]-u_2[k+1][j-2][i]) +
							 c1*(u_2[k+1][j+1][i]-u_2[k+1][j-1][i]) )*istrx   
						 - ( mu[k-1][j][i]*met_1[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_0[k-1][j+2][i]-u_0[k-1][j-2][i]) +
								 c1*(u_0[k-1][j+1][i]-u_0[k-1][j-1][i]) ) 
							 + (2*mu[k-1][j][i]+la[k-1][j][i])*met_2[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_1[k-1][j+2][i]-u_1[k-1][j-2][i]) +
								 c1*(u_1[k-1][j+1][i]-u_1[k-1][j-1][i]) )*stry[j]*istrx
							 +  mu[k-1][j][i]*met_3[k-1][j][i]*met_0[k-1][j][i]*(
								 c2*(u_2[k-1][j+2][i]-u_2[k-1][j-2][i]) +
								 c1*(u_2[k-1][j+1][i]-u_2[k-1][j-1][i]) )*istrx  ) );


				// rq derivatives
				// 130 ops, tot=1537
				r2 += c2*(
						la[k][j+2][i]*met_1[k][j+2][i]*met_0[k][j+2][i]*(
							c2*(u_0[k+2][j+2][i]-u_0[k-2][j+2][i]) +
							c1*(u_0[k+1][j+2][i]-u_0[k-1][j+2][i])   ) 
						+(2*mu[k][j+2][i]+la[k][j+2][i])*met_2[k][j+2][i]*met_0[k][j+2][i]*(
							c2*(u_1[k+2][j+2][i]-u_1[k-2][j+2][i]) +
							c1*(u_1[k+1][j+2][i]-u_1[k-1][j+2][i])  )*stry[j+2]*istrx 
						+ la[k][j+2][i]*met_3[k][j+2][i]*met_0[k][j+2][i]*(
							c2*(u_2[k+2][j+2][i]-u_2[k-2][j+2][i]) +
							c1*(u_2[k+1][j+2][i]-u_2[k-1][j+2][i])   )*istrx 
						- ( la[k][j-2][i]*met_1[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_0[k+2][j-2][i]-u_0[k-2][j-2][i]) +
								c1*(u_0[k+1][j-2][i]-u_0[k-1][j-2][i])  ) 
							+(2*mu[k][j-2][i]+la[k][j-2][i])*met_2[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_1[k+2][j-2][i]-u_1[k-2][j-2][i]) +
								c1*(u_1[k+1][j-2][i]-u_1[k-1][j-2][i])   )*stry[j-2]*istrx 
							+ la[k][j-2][i]*met_3[k][j-2][i]*met_0[k][j-2][i]*(
								c2*(u_2[k+2][j-2][i]-u_2[k-2][j-2][i]) +
								c1*(u_2[k+1][j-2][i]-u_2[k-1][j-2][i])  )*istrx  ) 
					 ) + c1*(  
						 la[k][j+1][i]*met_1[k][j+1][i]*met_0[k][j+1][i]*(
							 c2*(u_0[k+2][j+1][i]-u_0[k-2][j+1][i]) +
							 c1*(u_0[k+1][j+1][i]-u_0[k-1][j+1][i]) ) 
						 + (2*mu[k][j+1][i]+la[k][j+1][i])*met_2[k][j+1][i]*met_0[k][j+1][i]*(
							 c2*(u_1[k+2][j+1][i]-u_1[k-2][j+1][i]) +
							 c1*(u_1[k+1][j+1][i]-u_1[k-1][j+1][i]) )*stry[j+1]*istrx 
						 +la[k][j+1][i]*met_3[k][j+1][i]*met_0[k][j+1][i]*(
							 c2*(u_2[k+2][j+1][i]-u_2[k-2][j+1][i]) +
							 c1*(u_2[k+1][j+1][i]-u_2[k-1][j+1][i]) )*istrx   
						 - ( la[k][j-1][i]*met_1[k][j-1][i]*met_0[k][j-1][i]*(
								 c2*(u_0[k+2][j-1][i]-u_0[k-2][j-1][i]) +
								 c1*(u_0[k+1][j-1][i]-u_0[k-1][j-1][i]) ) 
							 + (2*mu[k][j-1][i]+la[k][j-1][i])*met_2[k][j-1][i]*met_0[k][j-1][i]*(
								 c2*(u_1[k+2][j-1][i]-u_1[k-2][j-1][i]) +
								 c1*(u_1[k+1][j-1][i]-u_1[k-1][j-1][i]) )*stry[j-1]*istrx
							 + la[k][j-1][i]*met_3[k][j-1][i]*met_0[k][j-1][i]*(
								 c2*(u_2[k+2][j-1][i]-u_2[k-2][j-1][i]) +
								 c1*(u_2[k+1][j-1][i]-u_2[k-1][j-1][i]) )*istrx   ) );


				// 4 ops, tot=1541
				uacc_1[k][j][i] = a1*uacc_1[k][j][i] + r2*ijac;

				// w-equation

				//	    r1 = 0;
				// pp derivative (w)
				// 43 ops, tot=1580
				cof1=(mu[k][j][i-2])*met_0[k][j][i-2]*met_0[k][j][i-2]*strx[i-2];
				cof2=(mu[k][j][i-1])*met_0[k][j][i-1]*met_0[k][j][i-1]*strx[i-1];
				cof3=(mu[k][j][i])*met_0[k][j][i]*met_0[k][j][i]*strx[i];
				cof4=(mu[k][j][i+1])*met_0[k][j][i+1]*met_0[k][j][i+1]*strx[i+1];
				cof5=(mu[k][j][i+2])*met_0[k][j][i+2]*met_0[k][j][i+2]*strx[i+2];

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r3 += i6* (
						mux1*(u_2[k][j][i-2]-u_2[k][j][i]) + 
						mux2*(u_2[k][j][i-1]-u_2[k][j][i]) + 
						mux3*(u_2[k][j][i+1]-u_2[k][j][i]) +
						mux4*(u_2[k][j][i+2]-u_2[k][j][i])  )*istry;

				// qq derivative (w)
				// 43 ops, tot=1623
				cof1=(mu[k][j-2][i])*met_0[k][j-2][i]*met_0[k][j-2][i]*stry[j-2];
				cof2=(mu[k][j-1][i])*met_0[k][j-1][i]*met_0[k][j-1][i]*stry[j-1];
				cof3=(mu[k][j][i])*met_0[k][j][i]*met_0[k][j][i]*stry[j];
				cof4=(mu[k][j+1][i])*met_0[k][j+1][i]*met_0[k][j+1][i]*stry[j+1];
				cof5=(mu[k][j+2][i])*met_0[k][j+2][i]*met_0[k][j+2][i]*stry[j+2];
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r3 += i6* (
						mux1*(u_2[k][j-2][i]-u_2[k][j][i]) + 
						mux2*(u_2[k][j-1][i]-u_2[k][j][i]) + 
						mux3*(u_2[k][j+1][i]-u_2[k][j][i]) +
						mux4*(u_2[k][j+2][i]-u_2[k][j][i])  )*istrx;
				// rr derivative (u)
				// 43 ops, tot=1666
				cof1=(mu[k-2][j][i]+la[k-2][j][i])*met_1[k-2][j][i]*met_3[k-2][j][i];
				cof2=(mu[k-1][j][i]+la[k-1][j][i])*met_1[k-1][j][i]*met_3[k-1][j][i];
				cof3=(mu[k][j][i]+la[k][j][i])*met_1[k][j][i]*met_3[k][j][i];
				cof4=(mu[k+1][j][i]+la[k+1][j][i])*met_1[k+1][j][i]*met_3[k+1][j][i];
				cof5=(mu[k+2][j][i]+la[k+2][j][i])*met_1[k+2][j][i]*met_3[k+2][j][i];

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r3 += i6* (
						mux1*(u_0[k-2][j][i]-u_0[k][j][i]) + 
						mux2*(u_0[k-1][j][i]-u_0[k][j][i]) + 
						mux3*(u_0[k+1][j][i]-u_0[k][j][i]) +
						mux4*(u_0[k+2][j][i]-u_0[k][j][i])  )*istry;
				// rr derivative (v)
				// 43 ops, tot=1709
				cof1=(mu[k-2][j][i]+la[k-2][j][i])*met_2[k-2][j][i]*met_3[k-2][j][i];
				cof2=(mu[k-1][j][i]+la[k-1][j][i])*met_2[k-1][j][i]*met_3[k-1][j][i];
				cof3=(mu[k][j][i]+la[k][j][i])*met_2[k][j][i]*met_3[k][j][i];
				cof4=(mu[k+1][j][i]+la[k+1][j][i])*met_2[k+1][j][i]*met_3[k+1][j][i];
				cof5=(mu[k+2][j][i]+la[k+2][j][i])*met_2[k+2][j][i]*met_3[k+2][j][i];

				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r3 += i6* (
						mux1*(u_1[k-2][j][i]-u_1[k][j][i]) + 
						mux2*(u_1[k-1][j][i]-u_1[k][j][i]) + 
						mux3*(u_1[k+1][j][i]-u_1[k][j][i]) +
						mux4*(u_1[k+2][j][i]-u_1[k][j][i])  )*istrx;

				// rr derivative (w)
				// 83 ops, tot=1792
				cof1 = (2*mu[k-2][j][i]+la[k-2][j][i])*met_3[k-2][j][i]*met_3[k-2][j][i] +
					mu[k-2][j][i]*(met_1[k-2][j][i]*strx[i]*met_1[k-2][j][i]*strx[i]+
							met_2[k-2][j][i]*stry[j]*met_2[k-2][j][i]*stry[j] );
				cof2 = (2*mu[k-1][j][i]+la[k-1][j][i])*met_3[k-1][j][i]*met_3[k-1][j][i] +
					mu[k-1][j][i]*(met_1[k-1][j][i]*strx[i]*met_1[k-1][j][i]*strx[i]+
							met_2[k-1][j][i]*stry[j]*met_2[k-1][j][i]*stry[j] );
				cof3 = (2*mu[k][j][i]+la[k][j][i])*met_3[k][j][i]*met_3[k][j][i] +
					mu[k][j][i]*(met_1[k][j][i]*strx[i]*met_1[k][j][i]*strx[i]+
							met_2[k][j][i]*stry[j]*met_2[k][j][i]*stry[j] );
				cof4 = (2*mu[k+1][j][i]+la[k+1][j][i])*met_3[k+1][j][i]*met_3[k+1][j][i] +
					mu[k+1][j][i]*(met_1[k+1][j][i]*strx[i]*met_1[k+1][j][i]*strx[i]+
							met_2[k+1][j][i]*stry[j]*met_2[k+1][j][i]*stry[j]);
				cof5 = (2*mu[k+2][j][i]+la[k+2][j][i])*met_3[k+2][j][i]*met_3[k+2][j][i] +
					mu[k+2][j][i]*( met_1[k+2][j][i]*strx[i]*met_1[k+2][j][i]*strx[i]+
							met_2[k+2][j][i]*stry[j]*met_2[k+2][j][i]*stry[j] );
				mux1 = cof2 -tf*(cof3+cof1);
				mux2 = cof1 + cof4+3*(cof3+cof2);
				mux3 = cof2 + cof5+3*(cof4+cof3);
				mux4 = cof4-tf*(cof3+cof5);

				r3 += i6* (
						mux1*(u_2[k-2][j][i]-u_2[k][j][i]) + 
						mux2*(u_2[k-1][j][i]-u_2[k][j][i]) + 
						mux3*(u_2[k+1][j][i]-u_2[k][j][i]) +
						mux4*(u_2[k+2][j][i]-u_2[k][j][i])  )*istrxy
					// pr-derivatives
					// 86 ops, tot=1878
					// r1 += 
					+ c2*(
							(la[k+2][j][i])*met_3[k+2][j][i]*met_0[k+2][j][i]*(
								c2*(u_0[k+2][j][i+2]-u_0[k+2][j][i-2]) +
								c1*(u_0[k+2][j][i+1]-u_0[k+2][j][i-1])   )*istry 
							+ mu[k+2][j][i]*met_1[k+2][j][i]*met_0[k+2][j][i]*(
								c2*(u_2[k+2][j][i+2]-u_2[k+2][j][i-2]) +
								c1*(u_2[k+2][j][i+1]-u_2[k+2][j][i-1])  )*strx[i]*istry 
							- ((la[k-2][j][i])*met_3[k-2][j][i]*met_0[k-2][j][i]*(
									c2*(u_0[k-2][j][i+2]-u_0[k-2][j][i-2]) +
									c1*(u_0[k-2][j][i+1]-u_0[k-2][j][i-1])  )*istry  
								+ mu[k-2][j][i]*met_1[k-2][j][i]*met_0[k-2][j][i]*(
									c2*(u_2[k-2][j][i+2]-u_2[k-2][j][i-2]) +
									c1*(u_2[k-2][j][i+1]-u_2[k-2][j][i-1]) )*strx[i]*istry ) 
					     ) + c1*(  
						     (la[k+1][j][i])*met_3[k+1][j][i]*met_0[k+1][j][i]*(
							     c2*(u_0[k+1][j][i+2]-u_0[k+1][j][i-2]) +
							     c1*(u_0[k+1][j][i+1]-u_0[k+1][j][i-1]) )*istry  
						     + mu[k+1][j][i]*met_1[k+1][j][i]*met_0[k+1][j][i]*(
							     c2*(u_2[k+1][j][i+2]-u_2[k+1][j][i-2]) +
							     c1*(u_2[k+1][j][i+1]-u_2[k+1][j][i-1]) )*strx[i]*istry  
						     - (la[k-1][j][i]*met_3[k-1][j][i]*met_0[k-1][j][i]*(
								     c2*(u_0[k-1][j][i+2]-u_0[k-1][j][i-2]) +
								     c1*(u_0[k-1][j][i+1]-u_0[k-1][j][i-1]) )*istry  
							     + mu[k-1][j][i]*met_1[k-1][j][i]*met_0[k-1][j][i]*(
								     c2*(u_2[k-1][j][i+2]-u_2[k-1][j][i-2]) +
								     c1*(u_2[k-1][j][i+1]-u_2[k-1][j][i-1]) )*strx[i]*istry  ) )
					     // rp derivatives
					     // 79 ops, tot=1957
					     //   r1 += 
					     + istry*(c2*(
								     (mu[k][j][i+2])*met_3[k][j][i+2]*met_0[k][j][i+2]*(
									     c2*(u_0[k+2][j][i+2]-u_0[k-2][j][i+2]) +
									     c1*(u_0[k+1][j][i+2]-u_0[k-1][j][i+2])   ) 
								     + mu[k][j][i+2]*met_1[k][j][i+2]*met_0[k][j][i+2]*(
									     c2*(u_2[k+2][j][i+2]-u_2[k-2][j][i+2]) +
									     c1*(u_2[k+1][j][i+2]-u_2[k-1][j][i+2]) )*strx[i+2] 
								     - (mu[k][j][i-2]*met_3[k][j][i-2]*met_0[k][j][i-2]*(
										     c2*(u_0[k+2][j][i-2]-u_0[k-2][j][i-2]) +
										     c1*(u_0[k+1][j][i-2]-u_0[k-1][j][i-2])  )
									     + mu[k][j][i-2]*met_1[k][j][i-2]*met_0[k][j][i-2]*(
										     c2*(u_2[k+2][j][i-2]-u_2[k-2][j][i-2]) +
										     c1*(u_2[k+1][j][i-2]-u_2[k-1][j][i-2]) )*strx[i-2]  )
							 ) + c1*(  
								 (mu[k][j][i+1])*met_3[k][j][i+1]*met_0[k][j][i+1]*(
									 c2*(u_0[k+2][j][i+1]-u_0[k-2][j][i+1]) +
									 c1*(u_0[k+1][j][i+1]-u_0[k-1][j][i+1]) ) 
								 + mu[k][j][i+1]*met_1[k][j][i+1]*met_0[k][j][i+1]*(
									 c2*(u_2[k+2][j][i+1]-u_2[k-2][j][i+1]) +
									 c1*(u_2[k+1][j][i+1]-u_2[k-1][j][i+1]) )*strx[i+1]  
								 - (mu[k][j][i-1]*met_3[k][j][i-1]*met_0[k][j][i-1]*(
										 c2*(u_0[k+2][j][i-1]-u_0[k-2][j][i-1]) +
										 c1*(u_0[k+1][j][i-1]-u_0[k-1][j][i-1]) ) 
									 + mu[k][j][i-1]*met_1[k][j][i-1]*met_0[k][j][i-1]*(
										 c2*(u_2[k+2][j][i-1]-u_2[k-2][j][i-1]) +
										 c1*(u_2[k+1][j][i-1]-u_2[k-1][j][i-1]) )*strx[i-1]  ) ) )
										 // qr derivatives
										 // 86 ops, tot=2043
										 //     r1 +=
										 + c2*(
												 mu[k+2][j][i]*met_2[k+2][j][i]*met_0[k+2][j][i]*(
													 c2*(u_2[k+2][j+2][i]-u_2[k+2][j-2][i]) +
													 c1*(u_2[k+2][j+1][i]-u_2[k+2][j-1][i])   )*stry[j]*istrx 
												 + la[k+2][j][i]*met_3[k+2][j][i]*met_0[k+2][j][i]*(
													 c2*(u_1[k+2][j+2][i]-u_1[k+2][j-2][i]) +
													 c1*(u_1[k+2][j+1][i]-u_1[k+2][j-1][i])  )*istrx 
												 - ( mu[k-2][j][i]*met_2[k-2][j][i]*met_0[k-2][j][i]*(
														 c2*(u_2[k-2][j+2][i]-u_2[k-2][j-2][i]) +
														 c1*(u_2[k-2][j+1][i]-u_2[k-2][j-1][i])  )*stry[j]*istrx  
													 + la[k-2][j][i]*met_3[k-2][j][i]*met_0[k-2][j][i]*(
														 c2*(u_1[k-2][j+2][i]-u_1[k-2][j-2][i]) +
														 c1*(u_1[k-2][j+1][i]-u_1[k-2][j-1][i])   )*istrx  ) 
										      ) + c1*(  
											      mu[k+1][j][i]*met_2[k+1][j][i]*met_0[k+1][j][i]*(
												      c2*(u_2[k+1][j+2][i]-u_2[k+1][j-2][i]) +
												      c1*(u_2[k+1][j+1][i]-u_2[k+1][j-1][i]) )*stry[j]*istrx  
											      + la[k+1][j][i]*met_3[k+1][j][i]*met_0[k+1][j][i]*(
												      c2*(u_1[k+1][j+2][i]-u_1[k+1][j-2][i]) +
												      c1*(u_1[k+1][j+1][i]-u_1[k+1][j-1][i]) )*istrx   
											      - ( mu[k-1][j][i]*met_2[k-1][j][i]*met_0[k-1][j][i]*(
													      c2*(u_2[k-1][j+2][i]-u_2[k-1][j-2][i]) +
													      c1*(u_2[k-1][j+1][i]-u_2[k-1][j-1][i]) )*stry[j]*istrx  
												      + la[k-1][j][i]*met_3[k-1][j][i]*met_0[k-1][j][i]*(
													      c2*(u_1[k-1][j+2][i]-u_1[k-1][j-2][i]) +
													      c1*(u_1[k-1][j+1][i]-u_1[k-1][j-1][i]) )*istrx  ) )
										      // rq derivatives
										      //  79 ops, tot=2122
										      //  r1 += 
										      + istrx*(c2*(
													      mu[k][j+2][i]*met_2[k][j+2][i]*met_0[k][j+2][i]*(
														      c2*(u_2[k+2][j+2][i]-u_2[k-2][j+2][i]) +
														      c1*(u_2[k+1][j+2][i]-u_2[k-1][j+2][i])   )*stry[j+2] 
													      + mu[k][j+2][i]*met_3[k][j+2][i]*met_0[k][j+2][i]*(
														      c2*(u_1[k+2][j+2][i]-u_1[k-2][j+2][i]) +
														      c1*(u_1[k+1][j+2][i]-u_1[k-1][j+2][i])  ) 
													      - ( mu[k][j-2][i]*met_2[k][j-2][i]*met_0[k][j-2][i]*(
															      c2*(u_2[k+2][j-2][i]-u_2[k-2][j-2][i]) +
															      c1*(u_2[k+1][j-2][i]-u_2[k-1][j-2][i])  )*stry[j-2] 
														      + mu[k][j-2][i]*met_3[k][j-2][i]*met_0[k][j-2][i]*(
															      c2*(u_1[k+2][j-2][i]-u_1[k-2][j-2][i]) +
															      c1*(u_1[k+1][j-2][i]-u_1[k-1][j-2][i])   ) ) 
												  ) + c1*(  
													  mu[k][j+1][i]*met_2[k][j+1][i]*met_0[k][j+1][i]*(
														  c2*(u_2[k+2][j+1][i]-u_2[k-2][j+1][i]) +
														  c1*(u_2[k+1][j+1][i]-u_2[k-1][j+1][i]) )*stry[j+1] 
													  + mu[k][j+1][i]*met_3[k][j+1][i]*met_0[k][j+1][i]*(
														  c2*(u_1[k+2][j+1][i]-u_1[k-2][j+1][i]) +
														  c1*(u_1[k+1][j+1][i]-u_1[k-1][j+1][i]) )  
													  - ( mu[k][j-1][i]*met_2[k][j-1][i]*met_0[k][j-1][i]*(
															  c2*(u_2[k+2][j-1][i]-u_2[k-2][j-1][i]) +
															  c1*(u_2[k+1][j-1][i]-u_2[k-1][j-1][i]) )*stry[j-1] 
														  + mu[k][j-1][i]*met_3[k][j-1][i]*met_0[k][j-1][i]*(
															  c2*(u_1[k+2][j-1][i]-u_1[k-2][j-1][i]) +
															  c1*(u_1[k+1][j-1][i]-u_1[k-1][j-1][i]) ) ) ) );

				// 4 ops, tot=2126
				uacc_2[k][j][i] = a1*uacc_2[k][j][i] + r3*ijac;
			}
		}
	}
}
