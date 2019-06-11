R"(

/*!
 *	\brief	Copy scalar values of wall particles to its dummmies
 */
__kernel void DummyScalarCopy
(
	__global scalar *vec		: DUMMY_SCALAR,
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
	
	scalar vecI = vec[i];

	size_t j = i+1;
	while(j < particleCount && IsParticleDummy(typ[j]))
		vec[j++] = vecI;
}

)" /* end OpenCL code */
