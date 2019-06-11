R"(

__kernel void SetParticlesVelocity
(
	__global vector * vel	: VELOCITIES,
	vector newVel			: VECTOR_VALUE,
	uint firstParticle		: PARTICLE_INDEX_START,
	uint count				: PARTICLE_SET_COUNT
)
{
	uint i = get_global_id(0);
	if(i < count)
		vel[i+firstParticle] = newVel;
}

)
