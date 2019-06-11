R"(

__kernel void FindOutOfBounds
(
	__global char *typ			: CLASS,
	__global const vector *pos 	: POSITIONS,
	uint particleCount			: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;
	if(!IsParticleFluid(typ[i]))
		return;
	
	vector r = pos[i];
	
	if(r.x < GRID_START.x || r.y < GRID_START.y || r.x > GRID_END.x || r.y > GRID_END.y)
		typ[i] = NONE_PARTICLE;
}

)" /* end OpenCL code */
