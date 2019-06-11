/*!
 *	\brief	Compute particle masses from density
 */
__kernel void SetMasses
(
	__global const scalar *density : DENSITIES,
	__global scalar *mass: MASSES,
	uint particleCount : PARTICLE_COUNT,
	scalar r : PARTICLE_SPACING
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
#if DIM == 3
	scalar massI = r*r*r*density[i];
#else
	scalar massI = r*r*density[i];
#endif
	mass[i] = massI;
}



