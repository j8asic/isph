R"(

/*!
 *	\brief	Update BiCGSTAB result and resiudal vector
 */
__kernel void UpdateResultAndResidual
(
	__global scalar *residual : RESIDUAL,
	__global scalar *result : PRESSURES,
	__global const scalar *p : CONJUGATE_0,
	__global const scalar *s : CONJUGATE_1,
	__global const scalar *tmp1 : TMP_1,
	uint pc : PARTICLE_COUNT,
	scalar alpha : CG_ALPHA,
	scalar omega : CG_OMEGA
)
{
	size_t i = get_global_id(0);
	if(i < pc)
	{
		result[i] += alpha * p[i] + omega * s[i];
		residual[i] = s[i] - omega * tmp1[i];
	}
}

)" /* end OpenCL code */
