R"(

__kernel void SetParticlesPositionAndVelocity
(
	__global const vector *initPos : INITIAL_POSITIONS,
	__global vector * pos	: POSITIONS,
	__global vector * vel	: VELOCITIES,
	vector newPos			: VECTOR_VALUE,
	vector newVel			: VECTOR_VALUE2,
	uint firstParticle		: PARTICLE_INDEX_START,
	uint count				: PARTICLE_SET_COUNT
)
{
	uint i = get_global_id(0);
	if(i < count)
	{
		uint gi = i + firstParticle;
		pos[gi] = initPos[gi] + newPos;
		vel[gi] = newVel;
	}
}

)
