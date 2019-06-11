R"(

__kernel void VectorDotProduct
(
	__global const char *typ	: CLASS,
	__global const scalar *in1	: DOT_1,
	__global const scalar *in2	: DOT_2,
	__global scalar *g_odata	: DOT_OUT,
	__local scalar *sdata		: LOCAL_DOT,
	uint n						: PARTICLE_COUNT
)
{

#ifdef CPU

	size_t grid_size  = get_global_size(0);
	size_t chunk_size = (n + grid_size - 1) / grid_size;
	size_t start      = min((size_t)n, chunk_size * get_global_id(0));
	size_t stop       = min((size_t)n, start + chunk_size);
	
	scalar mySum      = (scalar)0;
	for (size_t i = start; i < stop; i++)
		if(!IsParticleDummy(typ[i])) mySum += in1[i] * in2[i];
	
	g_odata[get_group_id(0)] = mySum;

#else

	size_t tid        = get_local_id(0);
	size_t block_size = get_local_size(0);
	size_t p          = get_group_id(0) * block_size * 2 + tid;
	size_t gridSize   = get_num_groups(0) * block_size * 2;

	size_t i;
	scalar mySum      = (scalar)0;
	while (p < n)
	{
		i = p;
		if(!IsParticleDummy(typ[i])) mySum += in1[i] * in2[i];
		i = p + block_size;
		if (i < n)
			if(!IsParticleDummy(typ[i])) mySum += in1[i] * in2[i];
		p += gridSize;
	}
	sdata[tid] = mySum;
	barrier(CLK_LOCAL_MEM_FENCE);

	if (block_size >= 1024) { if (tid < 512) { sdata[tid] = mySum = mySum + sdata[tid + 512]; } barrier(CLK_LOCAL_MEM_FENCE); }
	if (block_size >=  512) { if (tid < 256) { sdata[tid] = mySum = mySum + sdata[tid + 256]; } barrier(CLK_LOCAL_MEM_FENCE); }
	if (block_size >=  256) { if (tid < 128) { sdata[tid] = mySum = mySum + sdata[tid + 128]; } barrier(CLK_LOCAL_MEM_FENCE); }
	if (block_size >=  128) { if (tid <  64) { sdata[tid] = mySum = mySum + sdata[tid +  64]; } barrier(CLK_LOCAL_MEM_FENCE); }

	if (tid < 32)
	{
		if (block_size >= 64) { sdata[tid] = mySum = mySum + sdata[tid + 32]; }
		if (block_size >= 32) { sdata[tid] = mySum = mySum + sdata[tid + 16]; }
		if (block_size >= 16) { sdata[tid] = mySum = mySum + sdata[tid +  8]; }
		if (block_size >=  8) { sdata[tid] = mySum = mySum + sdata[tid +  4]; }
		if (block_size >=  4) { sdata[tid] = mySum = mySum + sdata[tid +  2]; }
		if (block_size >=  2) { sdata[tid] = mySum = mySum + sdata[tid +  1]; }
	}

	if (tid == 0)
		g_odata[get_group_id(0)] = sdata[0];
#endif
}

)" /* end OpenCL code */
