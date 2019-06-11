R"(

__kernel void FixPressure
(
	__global scalar * p				: PRESSURES,
	__global const char *typ		: CLASS,
	__global const char *free_surface : FREE_SURFACE,
	__global const vector *pos 		: POSITIONS,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;
	
	if(!IsParticleFluid(typ[i]) || free_surface[i])
		return;
	
	vector posI = pos[i];
	int c = 0;
	scalar pAdd = (scalar)0;
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		
	if(IsParticleWall(typ[j]) && dot(posDif,posDif) < 1.35*PARTICLE_SPACING)
	{
		pAdd += p[j];
		c++;
	}

	ForEachEnd
	
	/*if(c)
		p[i] = pAdd / c;*/

}

)" /* end OpenCL code */
