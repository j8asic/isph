R"(

/*!
 *	\brief	Calculate intermediate velocities without pressure factor
 */
__kernel void TempVelocities
(
	__global char *free_surface 	: FREE_SURFACE,
	__global vector *vel 			: VELOCITIES,
	__global scalar *divP			: DIV_POS,
	__global const scalar *vol		: VOLUMES,
	__global const char *typ		: CLASS,
	__global const vector *pos 		: POSITIONS,
	__global const sym_tensor *kernelCorr : KERNEL_CORRECTION,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT,
	scalar dt 						: TIME_STEP
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;
	
	if(!IsParticleFluid(typ[i]))
	{
		free_surface[i] = 0;
		return;
	}
	
	vector posI = pos[i];
	vector velI = vel[i];
	vector accI = (vector)0;
	scalar divPos = (scalar)0;
#ifdef CORRECT_KERNEL
	sym_tensor corrTensor = kernelCorr[i];
#endif
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)

		vector velDif = velI - vel[j];
		vector gradW = SphKernelGrad(QSq, posDif);
	
		// position divergence, needs uncorrected kernel gradient
		//if(!IsParticleDummy(typ[j]))
			divPos -= dot(gradW, posDif);
		
#ifdef CORRECT_KERNEL
		gradW = CorrectGradW(gradW, corrTensor);
#endif

		// viscous acceleration
		accI += velDif * vol[j] * dot(gradW, posDif) / ((dot(posDif,posDif) + DIST_EPSILON));

	ForEachEnd
	
	accI *= 2 * DYNAMIC_VISCOSITY * vol[i] / MASS;
	accI += GRAVITY;
	
	vel[i] = velI + dt * accI;
	
	divPos *= VOLUME;
	divP[i] = divPos;
	free_surface[i] = (divPos < FREE_SURFACE_FACTOR) ? 1 : 0;
}

)" /* end OpenCL code */
