R"(

// types based on floating point precision
#if FP == 64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
typedef double scalar;
typedef double2 scalar2;
typedef double4 scalar4;
typedef double8 scalar8;
#else // default 32-bit precision
typedef float scalar;
typedef float2 scalar2;
typedef float4 scalar4;
typedef float8 scalar8;
#endif

#ifndef M_PI 
#define M_PI 3.1415926535897932384626433832795
#endif
#ifndef M_1_PI 
#define M_1_PI 0.31830988618379067154
#endif

// types based on dimensions of simulation
#if DIM == 3
typedef scalar4 vector;
typedef uint4 uint_vector;
typedef int4 int_vector;
typedef scalar8 sym_tensor; // tensor with symmetric coefs

#define make_vector(X,Y,Z)	((vector)(X, Y, Z, 0))
#define make_sym_tensor(A,B,C,D,E,F) ((sym_tensor)(A,B,C,D,E,F,0,0))

#else // default 2D

typedef scalar2 vector;
typedef uint2 uint_vector;
typedef int2 int_vector;
typedef scalar4 sym_tensor; // tensor with symmetric coefs

#define make_vector(X,Y)	((vector)(X, Y))
#define make_sym_tensor(A,B,C)	((sym_tensor)(A,B,C,0))

#endif

// classes of particles

#define NONE_PARTICLE -1
#define FLUID_PARTICLE 0
#define WALL_PARTICLE 1
#define DUMMY_PARTICLE 2
#define INFLOW_PARTICLE 3
#define OUTFLOW_PARTICLE 4

bool IsParticleFluid(char particleType)
{
	return particleType == FLUID_PARTICLE;
}

bool IsParticleWall(char particleType)
{
	return particleType == WALL_PARTICLE;
}

bool IsParticleDummy(char particleType)
{
	return particleType >= DUMMY_PARTICLE;
}

)" /* end OpenCL code */
