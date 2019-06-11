R"(

#define KERNEL_SUPPORT 3

#if DIM == 3
	#define KERNEL_CONST (M_1_PI / 120 * SMOOTHING_LENGTH_INV_3)
#else
	#define KERNEL_CONST (7 * M_1_PI / 478 * SMOOTHING_LENGTH_INV_SQ)
#endif

#define KERNEL_GRAD_CONST (-5 * KERNEL_CONST * SMOOTHING_LENGTH_INV_SQ)

/*!
 *	\brief	Quintic (5th order) spline smoothing kernel.
 */
scalar SphKernel(scalar QSq)
{
	scalar Q = sqrt(QSq);
	if(Q < 1.0)
		return KERNEL_CONST * (pown(3-Q,5) - 6*pown(2-Q,5) + 15*pown(1-Q,5));
	else if(Q < 2.0)
		return KERNEL_CONST * (pown(3-Q,5) - 6*pown(2-Q,5));
	else
		return KERNEL_CONST * pown(3-Q,5);
}

/*!
 *	\brief	Quintic (5th order) spline smoothing kernel gradient.
 */
vector SphKernelGrad(scalar QSq, vector distVec)
{
	scalar Q = sqrt(QSq);
	if(Q < 1.0)
		return distVec * (KERNEL_GRAD_CONST / Q * (pown(3-Q,4) - 6*pown(2-Q,4) + 15*pown(1-Q,4)));
	else if(Q < 2.0)
		return distVec * (KERNEL_GRAD_CONST / Q * (pown(3-Q,4) - 6*pown(2-Q,4)));
	else
		return distVec * (KERNEL_GRAD_CONST / Q * pown(3-Q,4));
}

/*!
 *	\brief	Kernel compact support .
 */
scalar SphKernelSupport(scalar smoothInv) 
{
	return KERNEL_SUPPORT / smoothInv;
}

)" /* end OpenCL code */
