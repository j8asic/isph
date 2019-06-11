R"(

/*!
 *	\brief	Calculate velocity divergence
 */
__kernel void DivVel
(
	__global scalar *out			: DIV_VEL,
	__global const vector *vel		: VELOCITIES,
	__global const char *typ		: CLASS,
	__global const vector *pos 		: POSITIONS,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;
	if(!IsParticleFluid(typ[i]))
	{
		out[i] = (scalar)0;
		return;
	}
	
	vector posI = pos[i];
	vector velI = vel[i];
	scalar divVel = (scalar)0;
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
	
		/*if(!IsParticleFluid(typ[j]))
			continue;*/

		vector velDif = vel[j] - velI;
		vector gradW = SphKernelGrad(QSq, posDif);
		divVel += dot(gradW, velDif);

	ForEachEnd

	out[i] = divVel * VOLUME;
}

)" /* end OpenCL code */
