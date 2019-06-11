R"(

__kernel void ComputeDeltaP
(
	__global scalar *output	: DELTA_P,
	scalar  l 				: PARTICLE_SPACING
)
{
#if DIM == 3
	*output = SphKernel(l*l*l*SMOOTHING_LENGTH_INV_SQ);
#else
	*output = SphKernel(l*l*SMOOTHING_LENGTH_INV_SQ);
#endif
}

)" /* end OpenCL code */
