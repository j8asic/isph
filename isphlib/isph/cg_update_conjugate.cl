R"(

/*!
 *	\brief	Update CG conjugate vector
 */
__kernel void UpdateConjugate
(
	__global const char *typ : CLASS,
	__global scalar *conjugate : CONJUGATE,
	__global const scalar *residual : RESIDUAL,
	uint pc : PARTICLE_COUNT,
	scalar beta : CG_BETA
)
{
	size_t i = get_global_id(0);
	if(i >= pc)
		return;
	if(IsParticleDummy(typ[i]))
	{
		conjugate[i] = (scalar)0;
	}
	{
		conjugate[i] = residual[i] + beta * conjugate[i];
	}
}

)" /* end OpenCL code */
