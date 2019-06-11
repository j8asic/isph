R"(

/*!
 *	\brief	Implicit matrix-vector multiplication
 */
__kernel void MatrixVectorProduct
(
	__global scalar *out			: TMP,
	__global const scalar *vol		: VOLUMES,
	__global const char *typ		: CLASS,
	__global const char *free_surface : FREE_SURFACE,
	__global const scalar *vec		: CONJUGATE,
	__global const vector *pos 		: POSITIONS,
	__global const sym_tensor *kernelCorr : KERNEL_CORRECTION,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);

	if(i >= particleCount)
		return;

	char type = typ[i];

	if(IsParticleDummy(type))
	{
		out[i] = (scalar)0;
		return;
	}

#ifdef STRONG_DIRICHLET
	if(free_surface[i])
	{
		out[i] = (scalar)0; //vec[i];
		return;
	}
#endif

	vector posI = pos[i];
	scalar vecI = vec[i];
	scalar bI = (scalar)0;
#ifdef CORRECT_KERNEL
	sym_tensor corrTensor = kernelCorr[i];
#endif
	
	ForEachSetup(posI)
	
	if(IsParticleWall(type))
	{
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		//if(!IsParticleFluid(typ[j]))
		if(IsParticleDummy(typ[j]) /*|| free_surface[j]*/)
			continue;
		
		vector gradW = SphKernelGrad(QSq, posDif);
#ifdef CORRECT_KERNEL
		gradW = CorrectGradW(gradW, corrTensor);
#endif
		//scalar aIJ = dot(gradW, posDif) / ((dot(posDif,posDif) + DIST_EPSILON));
		//bI += aIJ * (vecI - vec[j]) * vol[j];
		scalar aIJ = 1.0/vol[i] + 1.0/vol[j];
		aIJ = dot(gradW, posDif) / (aIJ*aIJ*((dot(posDif,posDif) + DIST_EPSILON)));
		bI += aIJ * (vecI - vec[j]);
	ForEachEnd
	}
	else if(IsParticleFluid(type))
	{
	if(free_surface[i])
		vecI *= 2;
	ForEachNeighbor(hashes,cellsStart,pos,posI)
		/*if(IsParticleDummy(typ[j]))
			continue;*/
		/*if(free_surface[i] && !IsParticleFluid(typ[j]))
			continue;*/
		vector gradW = SphKernelGrad(QSq, posDif);
#ifdef CORRECT_KERNEL
		gradW = CorrectGradW(gradW, corrTensor);
#endif
		//scalar aIJ = dot(gradW, posDif) / ((dot(posDif,posDif) + DIST_EPSILON));
		//bI += aIJ * (vecI - vec[j]) * vol[j];
		scalar aIJ = 1.0/vol[i] + 1.0/vol[j];
		aIJ = dot(gradW, posDif) / (aIJ*aIJ*((dot(posDif,posDif) + DIST_EPSILON)));
		bI += aIJ * (vecI - vec[j]);

	ForEachEnd
	}
	
	out[i] = 8 / MASS * bI;
}

)" /* end OpenCL code */
