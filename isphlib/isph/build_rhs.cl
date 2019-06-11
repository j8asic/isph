R"(

/*!
 *	\brief	Calculate RHS vector
 */
__kernel void BuildRHS
(
	__global scalar *rhs			: RHS,
	__global const scalar *vol		: VOLUMES,
	__global const char *typ		: CLASS,
	__global const char *free_surface : FREE_SURFACE,
	__global const vector *vel		: VELOCITIES,
	__global const vector *pos 		: POSITIONS,
	__global const sym_tensor *kernelCorr : KERNEL_CORRECTION,
	__global scalar * p				: PRESSURES,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT,
	scalar dt_inv					: TIME_STEP_INV
)
{
	size_t i = get_global_id(0);
	
	p[i] = (scalar)0;
	
	if(i >= particleCount)
	{
		rhs[i] = (scalar)0; // needed ?
		return;
	}

	if(!IsParticleFluid(typ[i])
#ifdef STRONG_DIRICHLET
	|| free_surface[i]
#endif
	)
	{
		rhs[i] = (scalar)0;
		return;
	}
	
	
	vector posI = pos[i];
	vector velI = vel[i];
	scalar bI = (scalar)0;
#ifdef CORRECT_KERNEL
	sym_tensor corrTensor = kernelCorr[i];
#endif
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)
	
		/*if(IsParticleWall(typ[i]) && !IsParticleFluid(typ[j]))
			continue;*/
	
		vector velDif = vel[j] - velI;
		vector gradW = SphKernelGrad(QSq, posDif);
#ifdef CORRECT_KERNEL
		gradW = CorrectGradW(gradW, corrTensor);
#endif
		bI += dot(gradW, velDif) * vol[j];

	ForEachEnd
	
	rhs[i] = bI * dt_inv;
}

)" /* end OpenCL code */
