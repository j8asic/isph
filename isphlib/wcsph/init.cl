/*!
 *	\brief	Init WCSPH specific stuff from OpenCL rather than from host
 */
__kernel void WcsphInit
(
	__global scalar *pods			: PODS,
	__global vector *xsph_vel		: XSPH_VELOCITIES,
	__global const vector *vel		: VELOCITIES,
	__global const scalar *density	: DENSITIES,
	__global const scalar *p		: PRESSURES,
	uint particleCount 				: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	
	pods[i] = p[i] / (density[i] * density[i]);
	xsph_vel[i] = vel[i];
}
