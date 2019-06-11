R"(

#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154
#endif

#define KERNEL_SUPPORT 3

/*!
 *	\brief	Modified Gauss' (exponential based) smoothing kernel.
 */
scalar SphKernel(scalar QSq, scalar smoothInv, scalar smoothInvSq)
{
	scalar d = exp(-9); 
#if DIM == 3
	scalar c = 0.179587122 * smoothInvSq * smoothInv;
#else
	scalar c = M_1_PI * smoothInvSq;
#endif
	return c * (exp(-(QSq)) - d) / (1 - 10 * d);
}

/*!
 *	\brief	Modified Gauss' (exponential based) smoothing kernel gradient.
 */
vector SphKernelGrad(vector distVec, scalar smoothInv, scalar smoothInvSq)
{
	scalar c = -2 * M_1_PI * smoothInvSq * smoothInvSq;
	scalar d = exp(-9); 
	scalar dist = length(distVec);
	scalar Q = dist * smoothInv;
	if(Q < GAUSS_KERNEL_SUPPORT)
		return distVec * (c * exp(-(Q*Q)) / (1 - 10 * d));
	return (vector)0;
}

/*!
 *	\brief	Kernel compact support .
 */
scalar SphKernelSupport(scalar smoothInv) 
{
	return KERNEL_SUPPORT / smoothInv;
}	

)" /* end OpenCL code */
