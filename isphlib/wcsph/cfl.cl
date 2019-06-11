/*!
 *	\brief	Suggest next time step according to CFL
 */
__kernel void CflTimeStep
(
	__global const vector *vel : SORTED_VELOCITIES,
	__global const vector *pos		: SORTED_POSITIONS,
	__global const scalar *density : SORTED_DENSITIES,
	__global const vector *acc : ACCELERATIONS,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes : HASHES,
	uint particleCount : PARTICLE_COUNT,
	__global scalar *dt : NEXT_TIME_STEP
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount) return;
	vector posI = pos[i];
	vector velI = vel[i];
	scalar densityI = density[i];
	scalar sigmaMax = 0;
	scalar localSoundSpeed = densityI * DENSITY_INV;
	localSoundSpeed *= WC_SOUND_SPEED * localSoundSpeed * localSoundSpeed;

	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		// Find maximum of viscosity stability parameter Monaghan JWPCOE 1999
		scalar sigma = fabs(dot(vel[_j]-velI, posDif)) / (dot(posDif, posDif) + DIST_EPSILON);
		if(sigma > sigmaMax)
			sigmaMax = sigma;
	ForEachEnd

	// Compare with maximum acceleration stability Monaghan ARAA 1992
    scalar dtMin = WC_SOUND_SPEED + SMOOTHING_LENGTH * sigmaMax;//(maxlocalSoundSpeed + h * sigmaMax, dot(accI,accI));
    dtMin =  0.3 * SMOOTHING_LENGTH / dtMin;
		
	if(dtMin < *dt)
		*dt = dtMin;
}
