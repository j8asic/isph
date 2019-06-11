R"(

/*!
 *	\brief	Corrector step: fix velocities and advance particles
 */
__kernel void CorrectorStep
(
	__global vector *pos			: POSITIONS,
	__global vector *vel			: VELOCITIES,
	__global const scalar *vol		: VOLUMES,
	__global const char *typ		: CLASS,
	__global const scalar *press	: PRESSURES,
	__global const char *free_surface : FREE_SURFACE,
	__global const vector *old_pos	: POSITIONS_OLD,
	__global const vector *old_vel	: VELOCITIES_OLD,
	__global const vector *temp_pos : POSITIONS_TEMP,
	__global const sym_tensor *kernelCorr : KERNEL_CORRECTION,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT,
	scalar dt						: TIME_STEP
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;

	if(!IsParticleFluid(typ[i]))
		return;
	
	vector posI = temp_pos[i];
	vector gradP = (vector)0;
	scalar podI = press[i] * pown(vol[i],2);
#ifdef CORRECT_KERNEL
	sym_tensor corrTensor = kernelCorr[i];
#endif
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,temp_pos,posI)

		vector gradW = SphKernelGrad(QSq, posDif);
#ifdef CORRECT_KERNEL
		gradW = CorrectGradW(gradW, corrTensor);
#endif

#ifdef STRONG_DIRICHLET
	if(free_surface[i])
		gradP += (1.25*press[j]*pown(vol[j],2)) * gradW;
	else if(free_surface[j])
		gradP += (2*podI) * gradW;
	else
#endif
		gradP += (press[j]*pown(vol[j],2) + podI) * gradW;
		
	ForEachEnd
	
	vector v = vel[i] - gradP * (dt / MASS);
	vel[i] = v;
	pos[i] = old_pos[i] + 0.5 * dt * (old_vel[i] + v);
}

)" /* end OpenCL code */
