/*!
 *	\brief	MLS pre step calculates MLS matrix coefficients and store them to 3 global vectors 
 *            only 6 elements needed since matrix is symmetric
 */
__kernel void MLSPre
(
	__global vector *mls_1 : MLS_1,
	__global vector *mls_2 : MLS_2,
	__global vector *mls_3 : MLS_3,
	__global const scalar *density : SORTED_DENSITIES,
	__global const scalar *mass : SORTED_MASSES,
	__global const vector *pos : SORTED_POSITIONS,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes : HASHES,
	uint particleCount : PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	
	vector posI = pos[i];
	
	
	// Zero temp array
	vector tmp_1 = (vector)0;
	vector tmp_2 = (vector)0;
	vector tmp_3 = (vector)0;
	
	// Add contribution to MLS matrix terms from neighboring particles
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
	    scalar W = SphKernel(QSq) * mass[_j] / density[_j];
	    tmp_1.x += W;                     //A(1,1)
	    tmp_1.y += posDif.x * W;               //A(1,2)
	    tmp_2.x += posDif.y * W;               //A(1,3)
	    tmp_2.y += posDif.x * posDif.x * W; //A(2,2)
	    tmp_3.x += posDif.x * posDif.y * W; //A(2,3)
	    tmp_3.y += posDif.y * posDif.y * W; //A(3,3)

// In 3d the matrix is 4x4
#if DIM == 3		
		tmp_1.z += posDif.z * W;				 //A(1,4)
    	tmp_2.z += posDif.x  * posDif.z * W; //A(2,4)
		tmp_3.z += posDif.y  * posDif.z * W; //A(3,4)
    	tmp_3.w+= posDif.z  * posDif.z * W; //A(4,4)
#endif
        //tmp_1 *= W;
	    //tmp_2 *= W;
	   // tmp_3 *= W;
    ForEachEnd
	
	// Self contribution
	tmp_1.x += SphKernel(0) * mass[i] / density[i];

	// Write to global
	mls_1[i] = tmp_1;
	mls_2[i] = tmp_2;
	mls_3[i] = tmp_3;
}
