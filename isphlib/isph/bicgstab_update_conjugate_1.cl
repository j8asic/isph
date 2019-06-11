R"(

/*!
 *	\brief	Update BiCGSTAB 2nd conjugate vector
 */
__kernel void UpdateConjugate_1
(
	__global scalar *conjugate : CONJUGATE_1,
	__global const scalar *residual : RESIDUAL,
	__global const scalar *tmp0 : TMP_0,
	uint pc : PARTICLE_COUNT,
	scalar alpha : CG_ALPHA
)
{
	size_t i = get_global_id(0);
	if(i < pc)
	{
		conjugate[i] = residual[i] - alpha * tmp0[i];
	}
}

)" /* end OpenCL code */
