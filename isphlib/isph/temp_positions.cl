R"(
/*!
 *	\brief	Advance particles to intermediate positions
 */
__kernel void TempPositions
(
	__global vector *pos		: POSITIONS,
	__global const char *typ	: CLASS,
	__global const vector *vel	: VELOCITIES,
	scalar dt					: TIME_STEP
)
{
	size_t i = get_global_id(0);
	if(IsParticleFluid(typ[i]))
		pos[i] += vel[i] * dt;
}

)" /* end OpenCL code */
