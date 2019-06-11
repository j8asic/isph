R"(

/*!
 *	\brief	Copy vector values of wall particles to its dummmies
 */
__kernel void DummyVectorCopy
(
	__global vector *vec		: DUMMY_VECTOR,
	__global const char *typ	: CLASS,
	uint particleCount			: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);

	if(i >= particleCount)
		return;

	char type = typ[i];

	if(!IsParticleWall(type))
		return;
	
	vector vecI = vec[i];

	size_t j = i+1;
	while(j < particleCount && IsParticleDummy(typ[j]))
		vec[j++] = vecI;
}

)" /* end OpenCL code */
