#include "isph.h"
#include <cmath>
#include <cfloat>
using namespace isph;

IsphSimulation::IsphSimulation(unsigned int simDimensions, VariableDataType scalarDataType)
	: Simulation(simDimensions, scalarDataType)
	, projectionForm(NonIncremental)
	, projectionOrder(1)
	, solverType(CG)
	, maxIterations(100)
	, solvingTolerance(0.001)
	, freeSurfaceFactor(simDimensions==2 ? 1.5 : 2.4)
	, shifting(false)
	, shiftingFactor(0.04)
	, shiftingFrequency(1)
	, strongDirichletBC(false)
{
}

IsphSimulation::~IsphSimulation()
{
}

bool IsphSimulation::InitSph()
{
	LogDebug("Initializing ISPH stuff");

	// buffers 
	this->InitSimulationBuffer("VOLUMES", this->ScalarDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("FREE_SURFACE", CharType, this->deviceParticleCount);
	this->InitSimulationBuffer("DIV_POS", this->ScalarDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("DIV_VEL", this->ScalarDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("POSITIONS_TEMP", this->VectorDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("POSITIONS_OLD", this->VectorDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("VELOCITIES_OLD", this->VectorDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("VELOCITIES_OLDER", this->VectorDataType(), projectionOrder > 1 ? this->deviceParticleCount : 1);
	this->InitSimulationBuffer("PRESSURES_OLD", this->ScalarDataType(), projectionForm != NonIncremental ? this->deviceParticleCount : 1);
	
	// program build options
	program->AddBuildOption("-D ISPH");
	program->AddBuildOption("-D ORDER=" + Utils::IntegerString(projectionOrder));
	program->AddBuildOption("-D PROJECTION=" + Utils::IntegerString((int)projectionForm + 1));
	if(strongDirichletBC)
		program->AddBuildOption("-D STRONG_DIRICHLET");

	// vars
	this->InitSimulationVariable("FREE_SURFACE_FACTOR", this->ScalarDataType(), freeSurfaceFactor, true);
	this->InitSimulationVariable("SHIFTING_FACTOR", this->ScalarDataType(), shiftingFactor, true);

	// subprograms
   this->LoadSubprogram("calc volumes",
                        #include "isph/calc_volumes.cl"
                        );
   this->LoadSubprogram("temp positions",
                        #include "isph/temp_positions.cl"
                        );
   this->LoadSubprogram("temp velocities",
                        #include "isph/temp_velocities.cl"
                        );
   this->LoadSubprogram("build rhs",
                        #include "isph/build_rhs.cl"
                        );
   this->LoadSubprogram("dummy scalar copy",
                        #include "isph/dummy_scalar_copy.cl"
                        );
   this->LoadSubprogram("dummy vector copy",
                        #include "isph/dummy_vector_copy.cl"
                        );
   this->LoadSubprogram("matrix-vector product",
                        #include "isph/spmv_product.cl"
                        );
   this->LoadSubprogram("corrector step",
                        #include "isph/correct.cl"
                        );
   this->LoadSubprogram("velocity divergence",
                        #include "isph/div_vel.cl"
                        );
   this->LoadSubprogram("fix pressure",
                        #include "isph/fix_pressure.cl"
                        );
	if(shifting)
	{
		this->InitSimulationBuffer("VELOCITIES_TEMP", this->VectorDataType(), this->deviceParticleCount);
		this->InitSimulationBuffer("PRESSURES_TEMP", this->ScalarDataType(), this->deviceParticleCount);
      this->LoadSubprogram("shift particles",
                           #include "isph/shifting.cl"
                           );
      this->LoadSubprogram("update shifted particles",
                           #include "isph/shifting_update.cl"
                           );
	}

	LogDebug("Initializing PPE solver stuff");

	// PPE solvers
	this->InitSimulationBuffer("RHS", this->ScalarDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("SORTED_PRESSURES", this->ScalarDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("RESIDUAL", this->ScalarDataType(), this->deviceParticleCount);
	this->InitSimulationVariable("CG_ALPHA", this->ScalarDataType(), false);
	this->InitSimulationVariable("CG_BETA", this->ScalarDataType(), false);

   LoadSubprogram("dot product",
                  #include "isph/dot.cl"
                  );
	InitSimulationBuffer("DOT_OUT", ScalarDataType(), 2 * Devices()->Device(0)->ComputeUnits());
	CLLocalBuffer *var = new CLLocalBuffer(program, "LOCAL_DOT");  var->SetSpace(ScalarDataType(), 2 * 256);

	if(solverType == CG)
	{
		this->InitSimulationBuffer("CONJUGATE", this->ScalarDataType(), this->deviceParticleCount);
		this->InitSimulationBuffer("TMP", this->ScalarDataType(), this->deviceParticleCount);
      this->LoadSubprogram("update result and residual",
                           #include "isph/cg_update_result.cl"
                           );
      this->LoadSubprogram("update conjugate",
                           #include "isph/cg_update_conjugate.cl"
                           );
		program->ConnectSemantic("DOT_1", program->Buffer("TMP"));
		program->ConnectSemantic("DOT_2", program->Buffer("CONJUGATE"));
	}
	else if(solverType == BiCGSTAB)
	{
		this->InitSimulationVariable("CG_OMEGA", this->ScalarDataType(), false);
		this->InitSimulationBuffer("CONJUGATE_0", this->ScalarDataType(), this->deviceParticleCount);
		this->InitSimulationBuffer("CONJUGATE_1", this->ScalarDataType(), this->deviceParticleCount);
		this->InitSimulationBuffer("TMP_0", this->ScalarDataType(), this->deviceParticleCount);
		this->InitSimulationBuffer("TMP_1", this->ScalarDataType(), this->deviceParticleCount);
      this->LoadSubprogram("update result and residual",
                           #include "isph/bicgstab_update_result.cl"
                           );
      this->LoadSubprogram("update conjugate 0",
                           #include "isph/bicgstab_update_conjugate_0.cl"
                           );
      this->LoadSubprogram("update conjugate 1",
                           #include "isph/bicgstab_update_conjugate_1.cl"
                           );
		program->ConnectSemantic("TMP", program->Buffer("TMP_0"));
		program->ConnectSemantic("CONJUGATE", program->Buffer("CONJUGATE_0"));
		program->ConnectSemantic("DOT_1", program->Buffer("TMP_0"));
		program->ConnectSemantic("DOT_2", program->Buffer("CONJUGATE_0"));
	}

	program->ConnectSemantic("DUMMY_SCALAR", program->Buffer("PRESSURES"));
	program->ConnectSemantic("DUMMY_VECTOR", program->Buffer("VELOCITIES"));

	return true;
}

bool IsphSimulation::PostInitSph()
{
	/// \todo 
	/*if(!this->EnqueueSubprogram("velocity divergence"))
		return false;*/

	/*program->ConnectSemantic("DUMMY_VECTOR", program->Buffer("VELOCITIES"));
	if(!this->EnqueueSubprogram("dummy vector copy"))
		return false;*/

	if(projectionOrder > 1)
	{
		if(!program->Buffer("VELOCITIES_OLD")->CopyFrom(program->Buffer("VELOCITIES")))
			return false;
	}

	return true;
}

bool IsphSimulation::RunSph()
{
	/*
	if(!this->RunGrid())
		return false;
	*/
	if(projectionOrder > 1)
	{
		if(!program->Buffer("VELOCITIES_OLDER")->CopyFrom(program->Buffer("VELOCITIES_OLD"), false))
			return false;
	}

	if(!program->Buffer("POSITIONS_OLD")->CopyFrom(program->Buffer("POSITIONS"), false))
		return false;

	if(!program->Buffer("VELOCITIES_OLD")->CopyFrom(program->Buffer("VELOCITIES"), false))
		return false;

	if(!this->EnqueueSubprogram("temp positions"))
		return false;

	if(!program->Buffer("POSITIONS_TEMP")->CopyFrom(program->Buffer("POSITIONS"), false))
		return false;

	if(!this->RunGrid())
		return false;

	if(!this->EnqueueSubprogram("calc volumes"))
		return false;

	if(projectionForm != NonIncremental)
	{
		if(!program->Buffer("PRESSURES_OLD")->CopyFrom(program->Buffer("PRESSURES"), false))
			return false;
	}

	if(SmoothingKernelCorrection())
	{
		if(!this->EnqueueSubprogram("kernel correction"))
			return false;
	}

	if(!this->EnqueueSubprogram("temp velocities"))
		return false;

	if(!this->EnqueueSubprogram("build rhs"))
		return false;

	if(solverType == CG)
	{
		if(!SolvePressureWithCG())
			return false;
	}
	else if(solverType == BiCGSTAB)
	{
		if(!SolvePressureWithBiCGSTAB())
			return false;
	}
	else return false;

	if(!this->EnqueueSubprogram("corrector step"))
		return false;

	if(shifting && (this->TimeStepCount() + 1) % shiftingFrequency == 0)
	{
		if(!this->RunGrid())
			return false;
		if(!program->Buffer("PRESSURES_TEMP")->CopyFrom(program->Buffer("PRESSURES"), false))
			return false;
		if(!program->Buffer("VELOCITIES_TEMP")->CopyFrom(program->Buffer("VELOCITIES"), false))
			return false;
		if(!program->Buffer("POSITIONS_TEMP")->CopyFrom(program->Buffer("POSITIONS"), false))
			return false;
		if(!this->EnqueueSubprogram("shift particles"))
			return false;
		if(!this->EnqueueSubprogram("update shifted particles"))
			return false;
	}

	/*if(!this->RunGrid())
		return false;

	if(!this->EnqueueSubprogram("velocity divergence"))
		return false;*/
	
	return true;
}

double IsphSimulation::SuggestTimeStep()
{
	double dt_cfl = this->timeStepCount ? 0.2 * this->particleSpacing / this->MaximumVelocity() : 1e-6;
	double dt_visc = 0.2 * smoothingLength * smoothingLength * density / (dynamicViscosity + 1e-10);
	return (std::min)((std::min)(dt_cfl, dt_visc), 1e-3);
}

void IsphSimulation::SetSolver( SolverType type )
{
	solverType = type;
}

void IsphSimulation::SetMaxSolverIterations( unsigned int iterations )
{
	maxIterations = iterations;
}

void IsphSimulation::SetSolverTolerance( double tolerance )
{
	solvingTolerance = tolerance;
}

void IsphSimulation::SetFreeSurfaceFactor( double value )
{
	freeSurfaceFactor = value;
}

bool IsphSimulation::SolvePressureWithCG()
{
	CLGlobalBuffer* rhs = program->Buffer("RHS");
	CLGlobalBuffer* residual = program->Buffer("RESIDUAL");
	CLGlobalBuffer* conjugate = program->Buffer("CONJUGATE");
	CLGlobalBuffer* tmp = program->Buffer("TMP");

	residual->CopyFrom(rhs); // ok if initial solution is {0}, else residual = rhs - A.x0
	conjugate->CopyFrom(residual); // todo do this already in build rhs kernel

	unsigned int i;
	double ip_rr = dot(residual, residual);

	if(abs(ip_rr) <= DBL_EPSILON)
	{
		Log::Send(Log::Error, "Poisson equation has null vector right hand side.");
		return false;
	}

	double norm_rhs_squared = ip_rr;
	double residual_norm_squared;

	CLKernelArgument* alpha = program->Argument("CG_ALPHA");
	CLKernelArgument* beta = program->Argument("CG_BETA");

	program->ConnectSemantic("DUMMY_SCALAR", program->Buffer("CONJUGATE"));

	for(i = 0; i < maxIterations; i++)
	{
		if(!this->EnqueueSubprogram("dummy scalar copy"))
			return false;

		if(!this->EnqueueSubprogram("matrix-vector product"))
			return false;

		double inner_prod_temp = dot(tmp, conjugate);
		alpha->SetScalar(ip_rr / inner_prod_temp);

		if(!this->EnqueueSubprogram("update result and residual"))
			return false;

		residual_norm_squared = dot(residual, residual);

		if(abs(residual_norm_squared / norm_rhs_squared) <= solvingTolerance * solvingTolerance)
			break;

		// todo lower stuff put at start loop if(i>0)
		beta->SetScalar(residual_norm_squared / ip_rr);
		ip_rr = residual_norm_squared;

		if(!this->EnqueueSubprogram("update conjugate"))
			return false;

	}

	program->ConnectSemantic("DUMMY_SCALAR", program->Buffer("PRESSURES"));
	if(!this->EnqueueSubprogram("dummy scalar copy"))
		return false;

	double errorPrecision = sqrt(abs(residual_norm_squared / norm_rhs_squared));

	if(errorPrecision > solvingTolerance)
		Log::Send(Log::Warning, "CG solver hasn't converged to specified error tolerance. Increase maximum iterations or try BiCGSTAB solver.");
	else
		LogDebug("CG solver solved pressure with " + Utils::IntegerString(i) + " iterations, and error of " + Utils::DoubleString(errorPrecision));

	return true;
}

bool IsphSimulation::SolvePressureWithBiCGSTAB()
{
	CLGlobalBuffer* rhs = program->Buffer("RHS");
	CLGlobalBuffer* residual = program->Buffer("RESIDUAL");
	CLGlobalBuffer* conjugate0 = program->Buffer("CONJUGATE_0");
	CLGlobalBuffer* conjugate1 = program->Buffer("CONJUGATE_1");
	CLGlobalBuffer* tmp0 = program->Buffer("TMP_0");
	CLGlobalBuffer* tmp1 = program->Buffer("TMP_1");

	residual->CopyFrom(rhs); // ok if initial solution is {0}, else residual = rhs - A.x0
	conjugate0->CopyFrom(residual); // todo do this already in build rhs kernel

	double ip_rr = dot(residual, residual);

	if(abs(ip_rr) <= DBL_EPSILON)
	{
		//Log::Send(Log::Error, "Poisson equation has null vector right hand side.");
		return true;
	}

	double norm_rhs_squared = ip_rr;
	double residual_norm_squared;

	CLKernelArgument* alpha = program->Argument("CG_ALPHA");
	CLKernelArgument* beta = program->Argument("CG_BETA");
	CLKernelArgument* omega = program->Argument("CG_OMEGA");

	unsigned int i;
	for(i = 0; i < maxIterations; i++)
	{
		program->ConnectSemantic("DUMMY_SCALAR", conjugate0);
		if(!this->EnqueueSubprogram("dummy scalar copy"))
			return false;

		program->ConnectSemantic("TMP", tmp0, false);
		program->ConnectSemantic("CONJUGATE", conjugate0);
		if(!this->EnqueueSubprogram("matrix-vector product", Utils::NearestMultiple(this->ParticleCount(), 256), 256))
			return false;

		alpha->SetScalar(ip_rr / dot(tmp0, rhs));

		if(!this->EnqueueSubprogram("update conjugate 1"))
			return false;

		program->ConnectSemantic("DUMMY_SCALAR", conjugate1);
		if(!this->EnqueueSubprogram("dummy scalar copy"))
			return false;

		program->ConnectSemantic("TMP", tmp1, false);
		program->ConnectSemantic("CONJUGATE", conjugate1);
		if(!this->EnqueueSubprogram("matrix-vector product", Utils::NearestMultiple(this->ParticleCount(), 256), 256))
			return false;

		omega->SetScalar(dot(tmp1, conjugate1) / dot(tmp1, tmp1));

		if(!this->EnqueueSubprogram("update result and residual"))
			return false;

		residual_norm_squared = dot(residual, residual);

		if(abs(residual_norm_squared / norm_rhs_squared) <= solvingTolerance * solvingTolerance)
			break;

		double new_ip_rr = dot(residual, rhs);
		beta->SetScalar((new_ip_rr / ip_rr) * (alpha->GetScalar() / omega->GetScalar()));
		ip_rr = new_ip_rr;

		if(!this->EnqueueSubprogram("update conjugate 0"))
			return false;

	}

	program->ConnectSemantic("DUMMY_SCALAR", program->Buffer("PRESSURES"));
	if(!this->EnqueueSubprogram("dummy scalar copy"))
		return false;

	double errorPrecision = sqrt(abs(residual_norm_squared / norm_rhs_squared));

	if(errorPrecision > solvingTolerance)
		Log::Send(Log::Warning, "BiCGSTAB solver hasn't converged to specified error tolerance. Increase maximum iterations.");
	else
		LogDebug("BiCGSTAB solver solved pressure with " + Utils::IntegerString(i) + " iterations, and error of " + Utils::DoubleString(errorPrecision));

	return true;
}

double IsphSimulation::dot( CLGlobalBuffer* a, CLGlobalBuffer* b )
{
	program->ConnectSemantic("DOT_1", a);
	program->ConnectSemantic("DOT_2", b);
	CLGlobalBuffer* out = program->Buffer("DOT_OUT");
	
	// enqueue reduction kernel and get the result
	int localSize = Devices()->Device(0)->IsCPU() ? 1 : 256;
	if(!this->EnqueueSubprogram("dot product", localSize * out->Elements(), localSize))
		return 0.0;
	if(!out->Download(false, true))
		return 0.0;
	program->Finish();

	// accumulate downloaded data
	double sum = 0.0;
   for(size_t i=0; i < out->Elements(); i++)
		sum += out->GetScalar(i);

	return sum;
}

void IsphSimulation::SetProjection( ProjectionForm form, unsigned int order )
{
	projectionForm = form;

	if(form == NonIncremental)
	{
		if(order == 1)
			projectionOrder = 1;
		else if(order == 2)
			Log::Send(Log::Warning, "Non-incremental projection order is set to 2nd. Since it's naturally 1st order, simulation improvement is not guaranteed.");
		else
			Log::Send(Log::Warning, "Non-incremental projection is implemented in 1st (and 2nd) order. Defaulting to 1nd order.");
	}
	else
	{
		if(order != 1 && order != 2)
		{
			Log::Send(Log::Warning, "Incremental projection forms are implemented in 1st and 2nd order. Defaulting to 2nd order.");
			projectionOrder = 2;
		}
		else
			projectionOrder = order;
	}
}

void IsphSimulation::SetParticleShifting( bool enable, double factor, unsigned int frequency)
{
	shifting = enable;
	shiftingFactor = factor;

	if(enable && factor < 0.005)
		Log::Send(Log::Warning, "Particle shifting factor is very small, it may not have an effect on simulation.");
	else if(enable && factor > 0.1)
		Log::Send(Log::Warning, "Particle shifting factor is rather large, it may cause inaccuracy in simulation.");

	if(frequency < 1)
	{
		Log::Send(Log::Warning, "Particle shifting frequency cannot be less than 1. Setting to 1.");
		shiftingFrequency = 1;
	}
	else
		shiftingFrequency = frequency;
}

void IsphSimulation::SetStrongDirichletBC( bool strong )
{
	strongDirichletBC = strong;
}
