R"(

/*!
 *	\brief	Update BiCGSTAB first conjugate vector
 */
__kernel void UpdateConjugate_0
(
	__global scalar *conjugate : CONJUGATE_0,
	__global const scalar *residual : RESIDUAL,
	__global const scalar *tmp0 : TMP_0,
	uint pc : PARTICLE_COUNT,
	scalar beta : CG_BETA,
	scalar omega : CG_OMEGA
)
{
	size_t i = get_global_id(0);
	if(i < pc)
	{
		conjugate[i] = residual[i] + beta * (conjugate[i] - omega * tmp0[i]);
	}
}

)" /* end OpenCL code */
