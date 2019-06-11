R"(

#define KERNEL_SUPPORT 2

#if DIM == 3
	#define KERNEL_CONST (7 * M_1_PI / 8 * SMOOTHING_LENGTH_INV_3)
#else
	#define KERNEL_CONST (7 * M_1_PI / 4 * SMOOTHING_LENGTH_INV_SQ)
#endif

#define GRAD_CONST (-5 * KERNEL_CONST * SMOOTHING_LENGTH_INV)

/*!
 *	\brief	Wendland's smoothing kernel.
 */
scalar SphKernel(scalar QSq)
{
	scalar Q = sqrt(QSq);
	return max(KERNEL_CONST * pown(1-0.5*Q, 4), (scalar)0);
}

/*!
 *	\brief	Wendland's smoothing kernel gradient.
 */
vector SphKernelGrad(scalar QSq, vector distVec)
{
	scalar Q = sqrt(QSq);
	scalar dif = max(2 - Q, (scalar)0);
	return distVec * (GRAD_CONST * dif * dif / Q);
}

scalar SphKernelSupport(scalar smoothInv) 
{
	return KERNEL_SUPPORT / smoothInv;
}	

)" /* end OpenCL code */
