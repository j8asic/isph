R"(

/*!
 *	\brief	Reads specific vector  particle attribute at probes location
 */
__kernel void readProbeVector
(
	__global const vector *probesPos : PROBES_LOCATION,
	__global scalar *buffer : PROBES_BUFFER,
	__global const vector *pos : POSITIONS,
	__global const vector *value : PROBES_VECTOR,
	__global const scalar *density : DENSITIES,
	__global const scalar *mass : MASSES,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes : HASHES,
	__global uint *bufferedValues: PROBES_RECORDED_VALUES,
	uint bufferingSteps: PROBES_BUFFERING_STEPS,
	uint singleBufferSize: PROBES_SINGLE_BUFFER_SIZE,
	uint probesCount: PROBES_COUNT,
	uint attCount: PROBES_ATT_COUNT
)
{
	size_t i = get_global_id(0);
	// Calculate position in the buffer bufferedValues[0] time step index bufferedValues[1] attribute index
	size_t idx = bufferedValues[0]*singleBufferSize + bufferedValues[1]*probesCount + i;
	// Get probe position
	vector posI = probesPos[i];
    	
	vector tmp = (vector)0;
	// Average value as in SPH approximation
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		//uint j = particleJ.y;
		//if(j < fluidParticleCount)
			tmp +=  value[j] * SphKernel(QSq) * mass[j] / density[j];
	ForEachEnd
	buffer[idx] = tmp.x;
	buffer[idx + probesCount] = tmp.y;
	
	#if DIM == 3	
	  buffer[idx + 2 * probesCount] = tmp.z;
	#endif	
	barrier(CLK_LOCAL_MEM_FENCE);
    
	// At last thread execution increment global counters
	if (i==(probesCount-1)) 
	{
  	#if DIM == 3	
	   bufferedValues[1] = bufferedValues[1] + 3;
	#else	
	   bufferedValues[1] = bufferedValues[1] + 2;
	# endif
		
	   // Reset buffered attribute counter
	   if (bufferedValues[1] == attCount)
	   {
  	       bufferedValues[0] = bufferedValues[0] + 1;
  	       bufferedValues[1] = 0;
	   }
	   // Reset buffered time step counter
	   if (bufferedValues[0] == bufferingSteps)  bufferedValues[0] = 0;
	}
}

)"
