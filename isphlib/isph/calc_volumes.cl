R"(
__kernel void CalcVolumes
(
	__global scalar *vol			: VOLUMES,
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
	
	vol[i] = VOLUME; return;
	
	char type = typ[i];
	
	/*if(IsParticleDummy(type))
		return;*/
	
	vector posI = pos[i];
	scalar v = (scalar)0;
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		v += SphKernel(QSq);
	ForEachEnd
	
	v = 1.0 / (v + SphKernel(0));
	vol[i] = v;
	
	/*if(IsParticleWall(type))
	{
		size_t j = i+1;
		while(j < particleCount && IsParticleDummy(typ[j]))
			vol[j++] = v;
	}*/

}
)" /* end OpenCL code */
