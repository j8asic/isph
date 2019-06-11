/*!
 *	\brief	Second Step of RK4 integration
 */
__kernel void IntegrateRK42
(
	__global vector *pos : POSITIONS,
	__global scalar *density : DENSITIES,
	__global vector *vel : VELOCITIES,
	__global const vector *posTmp : POSITIONS_TMP,
	__global scalar *densityTmp : DENSITY_TMP,
	__global const vector *velTmp : VELOCITIES_TMP,
	__global const vector *xsphVel : XSPH_VELOCITIES,
	__global const vector *acc : ACCELERATIONS,
	__global const scalar *densityRoC : DENSITY_ROC,
	__global vector *xsphVelRK : XSPH_VELOCITIES_RK,
	__global vector *accRK : ACCELERATIONS_RK,
	__global scalar *densityRoCRK : DENSITY_ROC_RK,
	uint fpc : FLUID_PARTICLE_COUNT,
	uint pc : PARTICLE_COUNT,
	scalar dt : HALF_TIME_STEP
)
{
	size_t i = get_global_id(0);
	if(i >= pc)
		return;
	density[i] = densityTmp[i]+densityRoC[i] * dt;
	
	// Accumulate time derivatives (final factor 1/3)
	densityRoCRK[i] += 2*densityRoC[i];
	xsphVelRK[i] += 2*xsphVel[i]; 
	accRK[i] += 2*acc[i]; 
	
	if(i < fpc)
	{
		pos[i] = posTmp[i] + xsphVel[i] * dt;
		vel[i] = velTmp[i] + acc[i] * dt;
	}
}
