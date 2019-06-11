/*!
 *	\brief	Compute pressure (Tait's equation of state)
 */
__kernel void EoS
(
	__global const scalar *density : SORTED_DENSITIES,
	__global const int2 *hashes : HASHES,
	__global scalar *pods : PODS,
	__global scalar *pressure : PRESSURES,
	uint particleCount : PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	scalar densityI = density[i];
	scalar p = WC_CONST * (pow(densityI*DENSITY_INV, WC_GAMMA) - 1);
	pods[i] = p / (densityI * densityI);
	pressure[hashes[i].y] = p;
}
