/*!
 *	\brief	Shepard filter
 */
__kernel void ShepardFilter
(
	__global scalar *densityS : SORTED_DENSITIES,
	__global scalar *density : DENSITIES,
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
	scalar densityI = 0;
	scalar weight = 0;

	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
	
		scalar mult = SphKernel(QSq) * mass[_j];
		densityI += mult; 
		weight += mult / densityS[_j]; 
	
	ForEachEnd
	
	weight += SphKernel(0) * mass[i] / densityS[i];
	densityI += SphKernel(0) * mass[i];
	densityI /= weight;

	densityS[i] = densityI;
	density[hashes[i].y] = densityI;
}
