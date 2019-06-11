R"(

__kernel void ShiftParticlesUpdate
(
	__global vector *shiftedPos		: POSITIONS,
	__global scalar *shiftedP		: PRESSURES,
	__global vector *shiftedVel		: VELOCITIES,
	__global const scalar *vol		: VOLUMES,
	__global const char *typ		: CLASS,
	__global const char *free_surface : FREE_SURFACE,
	__global const vector *pos 		: POSITIONS_TEMP,
	__global const scalar *p		: PRESSURES_TEMP,
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
	/*scalar new_p = (scalar)0;
	vector new_vel = (vector)0;*/
	vector gp = (vector)0;
	vector gvx = (vector)0;
	vector gvy = (vector)0;
	vector velI = vel[i];
	scalar pI = p[i];
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		/*scalar c = vol[j] * SphKernel(QSq);
		new_p += c * p[j];
		new_vel += c * vel[j];*/
		vector gradW = vol[j] * SphKernelGrad(QSq, posDif);
		gp += (p[j] + pI) * gradW;
		gvx += (vel[j].x - velI.x) * gradW;
		gvy += (vel[j].y - velI.y) * gradW;
		
	ForEachEnd
	
	vector dr = shiftedPos[i] - posI;
	shiftedP[i] += dot(gp, dr);
	shiftedVel[i] += (vector)(dot(gvx, dr), dot(gvy, dr));
	
	/*shiftedP[i] = new_p;
	shiftedVel[i] = new_vel;*/
}

)" /* end OpenCL code */
