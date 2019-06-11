R"(

#define KERNEL_SUPPORT 2

#if DIM == 3
	#define KERNEL_CONST1 (M_1_PI * SMOOTHING_LENGTH_INV_3)
#else
	#define KERNEL_CONST1 (10.0 * M_1_PI / 7.0 * SMOOTHING_LENGTH_INV_SQ)
#endif

#define KERNEL_CONST2 (KERNEL_CONST1 / 4)

#define GRAD_CONST1 (-3 * KERNEL_CONST1 * SMOOTHING_LENGTH_INV_SQ)
#define GRAD_CONST2 (-3 * KERNEL_CONST2 * SMOOTHING_LENGTH_INV_SQ)

/*!
 *	\brief	Cubic (3rd order) spline smoothing kernel.
 */
scalar SphKernel(scalar QSq)
{
	scalar Q = sqrt(QSq);
	if(Q < 1.0)
		return KERNEL_CONST1 * (1 - 1.5*QSq + 0.75*QSq*Q);
	else
	{
		scalar dif = 2 - Q;
		return KERNEL_CONST2 * dif * dif * dif;
	}
}

/*!
 *	\brief	Cubic (3rd order) spline smoothing kernel gradient.
 */
vector SphKernelGrad(scalar QSq, vector distVec)
{
	scalar Q = sqrt(QSq);
	if(QSq <= 1)
		return distVec * (GRAD_CONST1 * (1 - 0.75*Q));
	else
	{
		scalar dif = 2 - Q;
		return distVec * (GRAD_CONST2 * dif * dif / Q);
	}
}

/*!
 *	\brief	Kernel compact support .
 */
scalar SphKernelSupport(scalar smoothInv) 
{
	return KERNEL_SUPPORT / smoothInv;
}

)" /* end OpenCL code */
