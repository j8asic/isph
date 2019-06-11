R"(

__kernel void FindCellStart
(
	__global const int2 *sortedHash	: HASHES,
	__global uint *cellsStart		: CELLS_START,
	__local int *localHash			: LOCAL_SIZE_INT,
	uint particleCount				: PARTICLE_COUNT
)
{
    size_t i = get_global_id(0);
	size_t j = get_local_id(0);
	int2 hash;
	
	if(i < particleCount)
	{
		hash = sortedHash[i];
		localHash[j+1] = hash.x;
		if(i>0 && j==0)
			localHash[0] = sortedHash[i-1].x;
	}

    barrier(CLK_LOCAL_MEM_FENCE);
	
	if(i < particleCount)
	{
		if(hash.x != -1)
		{
			if(i==0 || hash.x != localHash[j])
				cellsStart[hash.x] = (uint)i;
		}
	}
}

)" /* end OpenCL code */
