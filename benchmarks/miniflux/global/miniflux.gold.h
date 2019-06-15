void miniflux_gold (
		double *new_box_0_in, double *new_box_1_in, double *new_box_2_in, double *new_box_3_in, double *new_box_4_in,
		double *old_box_0_in, double *old_box_1_in, double *old_box_2_in, double *old_box_3_in, double *old_box_4_in,
		double *gx_0_in, double *gx_1_in, double *gx_2_in, double *gx_3_in, double *gx_4_in,
		double *gy_0_in, double *gy_1_in, double *gy_2_in, double *gy_3_in, double *gy_4_in,
		double *gz_0_in, double *gz_1_in, double *gz_2_in, double *gz_3_in, double *gz_4_in, int N) {

	double factor1 = (1.0/12.0);
	double factor2 = 2.0;

	double (*new_box_0)[320][320] = (double (*)[320][320]) new_box_0_in;
	double (*new_box_1)[320][320] = (double (*)[320][320]) new_box_1_in;
	double (*new_box_2)[320][320] = (double (*)[320][320]) new_box_2_in;
	double (*new_box_3)[320][320] = (double (*)[320][320]) new_box_3_in;
	double (*new_box_4)[320][320] = (double (*)[320][320]) new_box_4_in;
	double (*old_box_0)[320][320] = (double (*)[320][320]) old_box_0_in;
	double (*old_box_1)[320][320] = (double (*)[320][320]) old_box_1_in;
	double (*old_box_2)[320][320] = (double (*)[320][320]) old_box_2_in;
	double (*old_box_3)[320][320] = (double (*)[320][320]) old_box_3_in;
	double (*old_box_4)[320][320] = (double (*)[320][320]) old_box_4_in;
	double (*gx_0)[320][320] = (double (*)[320][320]) gx_0_in;
	double (*gx_1)[320][320] = (double (*)[320][320]) gx_1_in;
	double (*gx_2)[320][320] = (double (*)[320][320]) gx_2_in;
	double (*gx_3)[320][320] = (double (*)[320][320]) gx_3_in;
	double (*gx_4)[320][320] = (double (*)[320][320]) gx_4_in;
	double (*gy_0)[320][320] = (double (*)[320][320]) gy_0_in;
	double (*gy_1)[320][320] = (double (*)[320][320]) gy_1_in;
	double (*gy_2)[320][320] = (double (*)[320][320]) gy_2_in;
	double (*gy_3)[320][320] = (double (*)[320][320]) gy_3_in;
	double (*gy_4)[320][320] = (double (*)[320][320]) gy_4_in;
	double (*gz_0)[320][320] = (double (*)[320][320]) gz_0_in;
	double (*gz_1)[320][320] = (double (*)[320][320]) gz_1_in;
	double (*gz_2)[320][320] = (double (*)[320][320]) gz_2_in;
	double (*gz_3)[320][320] = (double (*)[320][320]) gz_3_in;
	double (*gz_4)[320][320] = (double (*)[320][320]) gz_4_in;

#pragma omp parallel for 
	for(int iz=0;iz<N;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=2;ix<N-1;ix++){
				gx_0[iz][iy][ix] = factor1*
					(old_box_0[iz][iy][ix-2]+
					 7*(old_box_0[iz][iy][ix-1]+old_box_0[iz][iy][ix]) +
					 old_box_0[iz][iy][ix+1]);                           
				gx_1[iz][iy][ix] = factor1*
					(old_box_1[iz][iy][ix-2]+
					 7*(old_box_1[iz][iy][ix-1]+old_box_1[iz][iy][ix]) +
					 old_box_1[iz][iy][ix+1]);          
				gx_2[iz][iy][ix] = factor1*
					(old_box_2[iz][iy][ix-2]+
					 7*(old_box_2[iz][iy][ix-1]+old_box_2[iz][iy][ix]) +
					 old_box_2[iz][iy][ix+1]);
				gx_3[iz][iy][ix] = factor1*
					(old_box_3[iz][iy][ix-2]+
					 7*(old_box_3[iz][iy][ix-1]+old_box_3[iz][iy][ix]) +
					 old_box_3[iz][iy][ix+1]);
				gx_4[iz][iy][ix] = factor1*
					(old_box_4[iz][iy][ix-2]+
					 7*(old_box_4[iz][iy][ix-1]+old_box_4[iz][iy][ix]) +
					 old_box_4[iz][iy][ix+1]);                                                                                                                  
			}
		}
	}
	for(int iz=0;iz<N;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=0;ix<N;ix++){
				gx_0[iz][iy][ix] *= factor2*gx_2[iz][iy][ix];
				gx_1[iz][iy][ix] *= factor2*gx_2[iz][iy][ix];       
				gx_3[iz][iy][ix] *= factor2*gx_2[iz][iy][ix];
				gx_4[iz][iy][ix] *= factor2*gx_2[iz][iy][ix];
				gx_2[iz][iy][ix] *= factor2*gx_2[iz][iy][ix];
			}
		}
	}  
	for(int iz=0;iz<N;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=0;ix<N-1;ix++){
				new_box_0[iz][iy][ix]+= gx_0[iz][iy][ix+1]-gx_0[iz][iy][ix];
				new_box_1[iz][iy][ix]+= gx_1[iz][iy][ix+1]-gx_1[iz][iy][ix];
				new_box_2[iz][iy][ix]+= gx_2[iz][iy][ix+1]-gx_2[iz][iy][ix];
				new_box_3[iz][iy][ix]+= gx_3[iz][iy][ix+1]-gx_3[iz][iy][ix];
				new_box_4[iz][iy][ix]+= gx_4[iz][iy][ix+1]-gx_4[iz][iy][ix];
			}
		}
	}
	//---------------------- y-direction
	for(int iz=0;iz<N;iz++){
		for(int iy=2;iy<N-1;iy++){
			for(int ix=0;ix<N;ix++){
				gy_0[iz][iy][ix] = factor1*
					(old_box_0[iz][iy-2][ix]+
					 7*(old_box_0[iz][iy-1][ix]+old_box_0[iz][iy][ix]) +
					 old_box_0[iz][iy+1][ix]);                           
				gy_1[iz][iy][ix] = factor1*
					(old_box_1[iz][iy-2][ix]+
					 7*(old_box_1[iz][iy-1][ix]+old_box_1[iz][iy][ix]) +
					 old_box_1[iz][iy+1][ix]);          
				gy_2[iz][iy][ix] = factor1*
					(old_box_2[iz][iy-2][ix]+
					 7*(old_box_2[iz][iy-1][ix]+old_box_2[iz][iy][ix]) +
					 old_box_2[iz][iy+1][ix]);
				gy_3[iz][iy][ix] = factor1*
					(old_box_3[iz][iy-2][ix]+
					 7*(old_box_3[iz][iy-1][ix]+old_box_3[iz][iy][ix]) +
					 old_box_3[iz][iy+1][ix]);
				gy_4[iz][iy][ix] = factor1*
					(old_box_4[iz][iy-2][ix]+
					 7*(old_box_4[iz][iy-1][ix]+old_box_4[iz][iy][ix]) +
					 old_box_4[iz][iy+1][ix]);                                                                                                                  
			}
		}
	}
	for(int iz=0;iz<N;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=0;ix<N;ix++){
				gy_0[iz][iy][ix] = factor2*gy_0[iz][iy][ix]*gy_3[iz][iy][ix];                          
				gy_1[iz][iy][ix] = factor2*gy_1[iz][iy][ix]*gy_3[iz][iy][ix];
				gy_2[iz][iy][ix] = factor2*gy_2[iz][iy][ix]*gy_3[iz][iy][ix];       
				gy_4[iz][iy][ix] = factor2*gy_4[iz][iy][ix]*gy_3[iz][iy][ix];
				gy_3[iz][iy][ix] = factor2*gy_3[iz][iy][ix]*gy_3[iz][iy][ix];                                                                                                                 
			}
		}
	}
	for(int iz=0;iz<N;iz++){
		for(int iy=0;iy<N-1;iy++){
			for(int ix=0;ix<N;ix++){
				new_box_0[iz][iy][ix]+= gy_0[iz][iy+1][ix]-gy_0[iz][iy][ix];
				new_box_1[iz][iy][ix]+= gy_1[iz][iy+1][ix]-gy_1[iz][iy][ix];
				new_box_2[iz][iy][ix]+= gy_2[iz][iy+1][ix]-gy_2[iz][iy][ix];
				new_box_3[iz][iy][ix]+= gy_3[iz][iy+1][ix]-gy_3[iz][iy][ix];
				new_box_4[iz][iy][ix]+= gy_4[iz][iy+1][ix]-gy_4[iz][iy][ix];
			}
		}
	}

	//----------------------  z-direction
	for(int iz=2;iz<N-1;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=0;ix<N;ix++){
				gz_0[iz][iy][ix] = factor1*
					(old_box_0[iz-2][iy][ix]+
					 7*(old_box_0[iz-1][iy][ix]+old_box_0[iz][iy][ix]) +
					 old_box_0[iz+1][iy][ix]);                           
				gz_1[iz][iy][ix] = factor1*
					(old_box_1[iz-2][iy][ix]+
					 7*(old_box_1[iz-1][iy][ix]+old_box_1[iz][iy][ix]) +
					 old_box_1[iz+1][iy][ix]);          
				gz_2[iz][iy][ix] = factor1*
					(old_box_2[iz-2][iy][ix]+
					 7*(old_box_2[iz-1][iy][ix]+old_box_2[iz][iy][ix]) +
					 old_box_2[iz+1][iy][ix]);
				gz_3[iz][iy][ix] = factor1*
					(old_box_3[iz-2][iy][ix]+
					 7*(old_box_3[iz-1][iy][ix]+old_box_3[iz][iy][ix]) +
					 old_box_3[iz+1][iy][ix]);
				gz_4[iz][iy][ix] = factor1*
					(old_box_4[iz-2][iy][ix]+
					 7*(old_box_4[iz-1][iy][ix]+old_box_4[iz][iy][ix]) +
					 old_box_4[iz+1][iy][ix]);                                                                                                                  
			}
		}
	}

	for(int iz=0;iz<N;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=0;ix<N;ix++){
				gz_0[iz][iy][ix] = factor2*gz_0[iz][iy][ix]*gz_4[iz][iy][ix];                          
				gz_1[iz][iy][ix] = factor2*gz_1[iz][iy][ix]*gz_4[iz][iy][ix];
				gz_2[iz][iy][ix] = factor2*gz_2[iz][iy][ix]*gz_4[iz][iy][ix];       
				gz_3[iz][iy][ix] = factor2*gz_3[iz][iy][ix]*gz_4[iz][iy][ix];
				gz_4[iz][iy][ix] = factor2*gz_4[iz][iy][ix]*gz_4[iz][iy][ix];                                                                                                                 
			}
		}
	}

	for(int iz=0;iz<N-1;iz++){
		for(int iy=0;iy<N;iy++){
			for(int ix=0;ix<N;ix++){
				new_box_0[iz][iy][ix]+= gz_0[iz+1][iy][ix]-gz_0[iz][iy][ix];
				new_box_1[iz][iy][ix]+= gz_1[iz+1][iy][ix]-gz_1[iz][iy][ix];
				new_box_2[iz][iy][ix]+= gz_2[iz+1][iy][ix]-gz_2[iz][iy][ix];
				new_box_3[iz][iy][ix]+= gz_3[iz+1][iy][ix]-gz_3[iz][iy][ix];
				new_box_4[iz][iy][ix]+= gz_4[iz+1][iy][ix]-gz_4[iz][iy][ix];
			}
		}
	}
}
