/*!
 *	\brief	Compute density acceleration with continuity equation
 */
__kernel void Continuity
(
	__global scalar *densityRoC : DENSITY_ROC,
	__global const vector *pos : POSITIONS,
	__global const scalar *density: DENSITIES,
	__global const vector *xsphVel : XSPH_VELOCITIES,
	__global const uint *cellsStart : CELLS_START,
	__global const uint *hashes : CELLS_HASH,
	__global const uint *particles : HASHES_PARTICLE,
	uint particleCount : PARTICLE_COUNT,
	scalar particleMass : MASS,
	scalar2 h : SMOOTHING_LENGTH_INV,
	vector gridStart : GRID_START,
	uint_vector cellCount : CELL_COUNT,
	scalar cellSizeInv: CELL_SIZE_INV
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	vector posI = pos[i];
	vector xsphVelI = xsphVel[i];
	scalar densityI = 0;
	
	ForEachSetup(posI,gridStart,cellSizeInv,cellCount)
	ForEachNeighbor(cellCount,hashes,particles,cellsStart,h)
		vector gradW = SphKernelGrad(QSq, posDif);
		densityI += dot(gradW, (xsphVelI - xsphVel[j]))/density[j]; // Avoids pressure fluctuation at free surfaces - Monaghan 1992 ARAA 
	ForEachEnd

	densityRoC[i] = densityI * particleMass * density[i];
	
}
