R"(

#define KERNEL_SUPPORT 3

#if DIM == 3
	#define KERNEL_CONST (0.1795871221251665617 * SMOOTHING_LENGTH_INV_3)
#else
	#define KERNEL_CONST (M_1_PI * SMOOTHING_LENGTH_INV_SQ)
#endif

#define GRAD_CONST (-2 * KERNEL_CONST * SMOOTHING_LENGTH_INV_SQ)

/*!
 *	\brief	Gauss' (exponential based) smoothing kernel.
 */
scalar SphKernel(scalar QSq)
{
	return KERNEL_CONST * exp(-QSq);
}

/*!
 *	\brief	Gauss' (exponential based) smoothing kernel gradient.
 */
vector SphKernelGrad(scalar QSq, vector distVec)
{
	return distVec * (GRAD_CONST * exp(-QSq));
}

/*!
 *	\brief	Kernel compact support .
 */
scalar SphKernelSupport(scalar smoothInv) 
{
	return  KERNEL_SUPPORT / smoothInv;
}	

)" /* end OpenCL code */
