R"(
__kernel void KernelNormalize
(
	__global sym_tensor *corr		: KERNEL_CORRECTION,
	__global const char *typ		: CLASS,
	__global const vector *pos 		: POSITIONS,
	__global const uint *cellsStart : CELLS_START,
	__global const int2 *hashes 	: HASHES,
	uint particleCount				: PARTICLE_COUNT
)
{
	size_t i = get_global_id(0);
	if(i >= particleCount)
		return;
	
	if(IsParticleDummy(typ[i]))
		return;

	vector posI = pos[i];
	scalar xx = 0;
	scalar yy = 0;
	scalar xy = 0;
#if DIM == 3
	scalar xz = 0;
	scalar yz = 0;
	scalar zz = 0;
#endif
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)

		vector gradW = SphKernelGrad(QSq, posDif);
		xx -= posDif.x * gradW.x;
		yy -= posDif.y * gradW.y;
		xy -= posDif.x * gradW.y;
#if DIM == 3
		zz -= posDif.z * gradW.z;
		xz -= posDif.x * gradW.z;
		yz -= posDif.y * gradW.z;
#endif
	ForEachEnd

#if DIM == 3
	scalar det = VOLUME*VOLUME*
				( xx * (zz*yy - yz*yz)
				- xy * (zz*xy - yz*xz)
				+ xz * (yz*xy - yy*xz));

	if(fabs(det) > 0.01) // check is well conditioned matrix
		corr[i] = make_sym_tensor(zz*yy-yz*yz, yz*xz-zz*xy, yz*xy-yy*xz, zz*xx-xz*xz, xy*xz-yz*xx, yy*xx-xy*xy) / det;
	else // no correction
		corr[i] = make_sym_tensor(1, 0, 0, 1, 0, 1);
#else
	scalar det = VOLUME * (xx * yy - xy * xy);

	if(fabs(det) > 0.01 /*&& fabs(DxWx) > 0.2 && fabs(DyWy) > 0.2*/) // check is well conditioned matrix
		corr[i] = make_sym_tensor(yy, -xy, xx) / det;
	else // no correction
		corr[i] = make_sym_tensor(1, 0, 1);
#endif

}

vector CorrectGradW(vector gradW, sym_tensor c)
{
#if DIM == 3
	// opencl 1.0, todo dot product with 3-component vecs ?
	return make_vector(
			gradW.x*c.s0 + gradW.y*c.s1 + gradW.z*c.s2,
			gradW.x*c.s1 + gradW.y*c.s3 + gradW.z*c.s4,
			gradW.x*c.s2 + gradW.y*c.s4 + gradW.z*c.s5,
	);
#else
	return make_vector(dot(gradW, c.xy), dot(gradW, c.yz));
#endif
}

)" /* end OpenCL code */
