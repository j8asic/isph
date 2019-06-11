R"(

__kernel void SetParticlesPosition
(
	__global const vector *initPos : INITIAL_POSITIONS,
	__global vector * pos	: POSITIONS,
	vector newPos			: VECTOR_VALUE,
	uint firstParticle		: PARTICLE_INDEX_START,
	uint count				: PARTICLE_SET_COUNT
)
{
	uint i = get_global_id(0);
	if(i < count)
		pos[i+firstParticle] = initPos[i+firstParticle] + newPos;
}

)
