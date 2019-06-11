/*!
 *	\brief	Compute density acceleration with continuity equation
 */
__kernel void Continuity
(
	__global scalar *densityRoC : DENSITY_ROC,
	__global const vector *pos : SORTED_POSITIONS,
	__global const scalar *mass : SORTED_MASSES,
	__global const vector *xsphVel : XSPH_SORTED,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes : HASHES,
	uint particleCount : PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	
	vector posI = pos[i];
	vector xsphVelI = xsphVel[i];
	scalar densityI = 0;
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)

		vector gradW = SphKernelGrad(QSq, posDif);
		// Avoids pressure fluctuation at free surfaces - Monaghan 1992 ARAA
		densityI += mass[_j] * dot(gradW, xsphVelI - xsphVel[_j]);

	ForEachEnd

	densityRoC[hashes[i].y] = densityI;
}
