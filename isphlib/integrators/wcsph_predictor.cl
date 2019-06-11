/*!
 *	\brief	Advance particles (predictor step)
 */
__kernel void IntegratePredictor
(
	__global vector *pos : POSITIONS,
	__global scalar *density : DENSITIES,
	__global vector *vel : VELOCITIES,
	__global const vector *xsphVel : XSPH_VELOCITIES,
	__global const vector *acc : ACCELERATIONS,
	__global const scalar *densityRoC : DENSITY_ROC,
	uint fpc : FLUID_PARTICLE_COUNT,
	scalar half_dt : HALF_TIME_STEP
)
{
	size_t i = get_global_id(0);
	density[i] += densityRoC[i] * half_dt;
	
	if(i < fpc)
	{
		pos[i] += xsphVel[i] * half_dt;
		vel[i] += acc[i] * half_dt;
	}
}
