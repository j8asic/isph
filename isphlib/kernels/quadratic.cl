R"(

#define KERNEL_SUPPORT 2

#if DIM == 3
	#define KERNEL_CONST (15 * M_1_PI / 16 * SMOOTHING_LENGTH_INV_3)
#else
	#define KERNEL_CONST (3 * M_1_PI / 2 * SMOOTHING_LENGTH_INV_SQ)
#endif

#define GRAD_CONST (KERNEL_CONST * SMOOTHING_LENGTH_INV_SQ)

/*!
 *	\brief	Quadratic smoothing kernel.
 */
scalar SphKernel(scalar QSq)
{
	scalar Q = sqrt(QSq);
	return KERNEL_CONST * (0.25*QSq - Q + 1);
}

/*!
 *	\brief	Quadratic smoothing kernel gradient.
 */
vector SphKernelGrad(scalar QSq, vector distVec)
{
	return distVec * (GRAD_CONST * (0.5 - rsqrt(QSq)));
}

/*!
 *	\brief	Kernel compact support.
 */
scalar SphKernelSupport(scalar smoothInv) 
{
	return KERNEL_SUPPORT / smoothInv;
}	

)" /* end OpenCL code */
