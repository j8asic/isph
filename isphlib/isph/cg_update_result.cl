R"(

/*!
 *	\brief	Update CG result and resiudal vector
 */
__kernel void UpdateResultAndResidual
(
	__global const char *typ : CLASS,
	__global scalar *residual : RESIDUAL,
	__global scalar *result : PRESSURES,
	__global const scalar *conjugate : CONJUGATE,
	__global const scalar *tmp : TMP,
	uint pc : PARTICLE_COUNT,
	scalar alpha : CG_ALPHA
)
{
	size_t i = get_global_id(0);
	if(i >= pc)
		return;
	
	if(IsParticleDummy(typ[i]))
	{
		result[i] = residual[i] = (scalar)0;
	}
	{
		result[i] += alpha * conjugate[i];
		residual[i] -= alpha * tmp[i];
	}
}

)" /* end OpenCL code */
