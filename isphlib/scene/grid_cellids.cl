R"(

__kernel void ComputeCellIds
(
	__global int2 *hash : HASHES,
	__global const vector *pos : POSITIONS,
	uint particleCount : PARTICLE_COUNT
)
{
    uint i = get_global_id(0);
	if(i < particleCount)
		hash[i] = (int2)(CellHash(CellPos(pos[i])), i);
	else
		hash[i] = (int2)(-1, 0);
}

)" /* end OpenCL code */
