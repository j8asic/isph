/*!
 *	\brief	Inverts MLS 3x3 matrices and calculates beta MLS kernel correction vector 
 *             then corrects density.
 */
__kernel void MLSPost
(
	__global const vector *mls_1 : MLS_1,
	__global const vector *mls_2 : MLS_2,
	__global const vector *mls_3 : MLS_3,
	__global scalar *density : DENSITIES,
	__global scalar *densityS : SORTED_DENSITIES,
	__global scalar *mass : SORTED_MASSES,
	__global const vector *pos : SORTED_POSITIONS,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes : HASHES,
	uint particleCount : PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	
	// Load global values
	vector tmp_1 = mls_1[i];
	vector tmp_2 = mls_2[i];
	vector tmp_3 = mls_3[i];
// Calculate adjoint matrix terms and determinant (it is symmetric)
// In 2d the matrix is 3x3
#if DIM == 2		
	scalar t11 =  (tmp_3.y * tmp_2.y  - tmp_3.x * tmp_3.x); //(a33*a22 - a32*a23) 
	scalar t12 = -(tmp_3.y * tmp_1.y  - tmp_3.x * tmp_2.x); //(a33*a12 - a32*a13)  
	scalar t13 =  (tmp_3.x * tmp_1.y  - tmp_2.y * tmp_2.x); //(a23*a12 - a22*a13)
	//Calculate determinant a11*(a33*a22 - a32*a23) - a21*(a33*a12 - a32*a13) + a31*(a23*a12 - a22*a13)
	scalar detA = (tmp_1.x * t11) + (tmp_1.y * t12) + (tmp_2.x * t13);
#endif
	
// In 3d the matrix is 4x4
#if DIM == 3		
	// t11 = ((a24*((-(a33*a24))+(a23*a34)))-(a23*((-(a34*a24))+(a23*a44)))+(a22*((-(a34^2))+(a33*a44))))
	scalar t11 =  ((tmp_2.z*((-(tmp_3.y*tmp_2.z))+(tmp_3.x*tmp_3.z)))-
	                     (tmp_3.x*((-(tmp_3.z*tmp_2.z))+(tmp_3.x*tmp_3.w)))+
						 (tmp_2.y*((-(tmp_3.z*tmp_3.z))+(tmp_3.y*tmp_3.w))));
	// t12 = -((a24*((-(a33*a14))+(a13*a34)))-(a23*((-(a34*a14))+(a13*a44)))+(a12*((-(a34^2))+(a33*a44))))
	scalar t12 =  -((tmp_2.z*((-(tmp_3.y*tmp_1.z))+(tmp_2.x*tmp_3.z)))-
	                     (tmp_3.x*((-(tmp_3.z*tmp_1.z))+(tmp_2.x*tmp_3.w)))+
						 (tmp_1.y*((-(tmp_3.z*tmp_3.z))+(tmp_3.y*tmp_3.w))));
    // t13 = ((a24*((-(a23*a14))+(a13*a24)))-(a22*((-(a34*a14))+(a13*a44)))+(a12*((-(a34*a24))+(a23*a44))))
	scalar t13 =  ((tmp_2.z*((-(tmp_3.x*tmp_1.z))+(tmp_2.x*tmp_2.z)))-
						(tmp_2.y*((-(tmp_3.z*tmp_1.z))+(tmp_2.x*tmp_3.w)))+
						(tmp_1.y*((-(tmp_3.z*tmp_2.z))+(tmp_3.x*tmp_3.w))));
	// t14 = -((a23*((-(a23*a14))+(a13*a24)))-(a22*((-(a33*a14))+(a13*a34)))+(a12*((-(a33*a24))+(a23*a34))))
	scalar t14 = -((tmp_3.x*((-(tmp_3.x*tmp_1.z))+(tmp_2.x*tmp_2.z)))-
	                     (tmp_2.y*((-(tmp_3.y*tmp_1.z))+(tmp_2.x*tmp_3.z)))+
						 (tmp_1.y*((-(tmp_3.y*tmp_2.z))+(tmp_3.x*tmp_3.z))));
	//Calculate determinant a11*(t11) - a21*(t12) + a31*(t13)+ a41*(t14)
	scalar detA = (tmp_1.x * t11) + (tmp_1.y * t12) + (tmp_2.x * t13) + (tmp_1.z * t14);
#endif	
	scalar beta0, beta1, beta2, beta3;
	
	if(fabs(detA) < 0.01f)
	{
		if(fabs(tmp_1.x) < 0.01f)
			return;
		// noo, matrix is sigualar, deal with it
		beta0 = 1 / tmp_1.x;
		
		//beta1 = beta2 = 0;
		
		beta1 = beta2 = beta3 = 0;
	}
	else
	{
		// Calculate MLS parameters
		scalar detAInv = 1 / detA;
		beta0 = t11 * detAInv;
		beta1 = t12 * detAInv;
		beta2 = t13 * detAInv;
#if DIM == 3		
		beta3 = t14 * detAInv;
#endif
	}
   
	vector posI = pos[i];
	scalar densityI = 0;
	
	// Correct density as integral of neighboring masses with corrected kernel
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
#if DIM == 2
        scalar W = (beta0 + (beta1 * posDif.x) + (beta2 * posDif.y)) * SphKernel(QSq);
#endif		
#if DIM == 3		
    	scalar W = (beta0 + (beta1 * posDif.x) + (beta2 * posDif.y) + (beta3 * posDif.z)) * SphKernel(QSq);
#endif	
     	densityI += W * mass[_j]; 
	ForEachEnd 
    
	// Correct density adding particle self contribution
	densityI += beta0 * SphKernel(0) * mass[i];
	densityS[i] = densityI;
	density[hashes[i].y] = densityI;
}
