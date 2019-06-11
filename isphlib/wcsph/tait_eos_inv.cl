/*!
 *	\brief	Compute density from pressure
 */
__kernel void EoSInv
(
	__global scalar *density : DENSITIES,
	__global const scalar *pressure : PRESSURES,
	uint particleCount : PARTICLE_COUNT,
	scalar wcConst : WC_CONST,
	scalar gamma : WC_GAMMA,
	scalar restDensityInv : DENSITY_INV
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	scalar densityI = pow(((pressure[i]/wcConst )+1),1/gamma)/restDensityInv;
	density[i] = densityI;
}



