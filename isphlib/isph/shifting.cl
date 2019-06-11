R"(

/*!
 *	\brief	Particle shifting to remove tensile instability
 */
__kernel void ShiftParticles
(
	__global vector *shiftedPos		: POSITIONS,
	__global const scalar *vol		: VOLUMES,
	__global const char *typ		: CLASS,
	__global const char *free_surface : FREE_SURFACE,
	__global const vector *pos 		: POSITIONS_TEMP,
	__global const vector *vel		: VELOCITIES_TEMP,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT,
	scalar factor					: SHIFTING_FACTOR,
	scalar dt 						: TIME_STEP
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;
	if(!IsParticleFluid(typ[i]) || free_surface[i])
		return;

	vector posI = pos[i];

	// 1st step: limit shifting radius for particles near free surface

	scalar effRadiusSq = KERNEL_SUPPORT_SQ*SMOOTHING_LENGTH_SQ;

	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		if(free_surface[j] /*|| IsParticleWall(typ[j])*/)
		{
			scalar distSq = dot(posDif, posDif);
			if(distSq < effRadiusSq)
				effRadiusSq = distSq;
		}
	ForEachEnd

	// 2nd step: create shifting vector

	int neighborCount = 0;
	scalar avgSpacing = (scalar)0;
	vector shiftVec = (vector)0;

	ForEachNeighbor(hashes,cellsStart,pos,posI)

		scalar distSq = dot(posDif, posDif);
		if(distSq > effRadiusSq)
			continue;

		scalar dist = sqrt(distSq);
		neighborCount++;
		avgSpacing += dist;
		shiftVec += posDif / (distSq*dist);
		
	ForEachEnd
	
	if(!neighborCount)
		return;

	avgSpacing /= neighborCount;
	shiftVec *= avgSpacing*avgSpacing;

	// 3rd step: shift
	scalar velMagnitude = length(vel[i]);
	//shiftVec *= factor * velMagnitude * dt;
	shiftVec = normalize(shiftVec) * min(length(shiftVec)*factor*velMagnitude*dt, PARTICLE_SPACING);
	shiftedPos[i] += shiftVec;
}

)" /* end OpenCL code */
