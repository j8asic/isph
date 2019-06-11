R"(

__kernel void MaxVelocitySquared
(
	__global const scalar *vel	: VELOCITIES,
	__global const char *fs		: FREE_SURFACE,
	__global scalar *g_odata	: OUT_MAX_VELOCITIES,
	__local scalar *sdata		: LOCAL_MAX_VELOCITIES,
	uint n						: PARTICLE_COUNT
)
{

#ifdef CPU

	size_t grid_size  = get_global_size(0);
	size_t chunk_size = (n + grid_size - 1) / grid_size;
	size_t start      = min((size_t)n, chunk_size * get_global_id(0));
	size_t stop       = min((size_t)n, start + chunk_size);
	
	scalar maxVel     = (scalar)0;
	vector vec;
	for (size_t i = start; i < stop; i++)
	{
		//if(fs[i]) continue;
		vec = vel[i];
		maxVel = max(maxVel, dot(vec,vec));
	}
	g_odata[get_group_id(0)] = maxVel;

#else

	size_t tid        = get_local_id(0);
	size_t block_size = get_local_size(0);
	size_t p          = get_group_id(0) * block_size * 2 + tid;
	size_t gridSize   = get_num_groups(0) * block_size * 2;

	size_t i;
	scalar maxVel     = (scalar)0;
	vector vec;
	while (p < n)
	{
		i = p;
		vec = vel[i];
		maxVel = max(maxVel, dot(vec,vec));
		i = p + block_size;
		if (i < n)
		{
			//if(!fs[i])
			{
				vec = vel[i];
				maxVel = max(maxVel, dot(vec,vec));
			}
		}
		p += gridSize;
	}
	sdata[tid] = maxVel;
	barrier(CLK_LOCAL_MEM_FENCE);

	if (block_size >= 1024) { if (tid < 512) { sdata[tid] = maxVel = max(maxVel, sdata[tid + 512]); } barrier(CLK_LOCAL_MEM_FENCE); }
	if (block_size >=  512) { if (tid < 256) { sdata[tid] = maxVel = max(maxVel, sdata[tid + 256]); } barrier(CLK_LOCAL_MEM_FENCE); }
	if (block_size >=  256) { if (tid < 128) { sdata[tid] = maxVel = max(maxVel, sdata[tid + 128]); } barrier(CLK_LOCAL_MEM_FENCE); }
	if (block_size >=  128) { if (tid <  64) { sdata[tid] = maxVel = max(maxVel, sdata[tid +  64]); } barrier(CLK_LOCAL_MEM_FENCE); }

	if (tid < 32)
	{
		if (block_size >= 64) { sdata[tid] = maxVel = max(maxVel, sdata[tid + 32]); }
		if (block_size >= 32) { sdata[tid] = maxVel = max(maxVel, sdata[tid + 16]); }
		if (block_size >= 16) { sdata[tid] = maxVel = max(maxVel, sdata[tid +  8]); }
		if (block_size >=  8) { sdata[tid] = maxVel = max(maxVel, sdata[tid +  4]); }
		if (block_size >=  4) { sdata[tid] = maxVel = max(maxVel, sdata[tid +  2]); }
		if (block_size >=  2) { sdata[tid] = maxVel = max(maxVel, sdata[tid +  1]); }
	}

	if (tid == 0)
		g_odata[get_group_id(0)] = sdata[0];
#endif
}

)" /* end OpenCL code */
