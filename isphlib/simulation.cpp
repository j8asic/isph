#include "simulation.h"
#include "isph.h"
#include "extern/clpp/clpp.h"
#include <cfloat>
#include <stack>
#include <queue>
#include <set>

using namespace isph;
using namespace std;

Simulation::Simulation(unsigned int simDimensions, VariableDataType scalarDataType)
   : dimensions(simDimensions)
   , particleCount(0)
   , massesBuffer(NULL)
   , positionsBuffer(NULL)
   , velocitiesBuffer(NULL)
   , pressuresBuffer(NULL)
   , densitiesBuffer(NULL)
	, smoothingKernel(CubicSplineKernel)
	, smoothingLength(0)
	, smoothingKernelCorrection(false)
	, density(0)
	, dynamicViscosity(0)
	, gridCellSize(0)
	, maxTime(0)
	, wantedTimeStep(0)
	, timeOverall(0)
	, timeStep(0)
	, cflFactor(1.0)
	, timeStepCount(0)
	, timeStepTimer(0)
	, asyncExport(true)
{
	LogDebug("Creating new simulation object");

	program = new CLProgram();

	for (unsigned int i=0; i<(unsigned int)ParticleTypeCount; i++)
	{
		particleMass[i] = 0;
        particleCountByType[i] = 0;
	}

	particleCount = 0;

	if(scalarDataType == DoubleType)
		scalarType = DoubleType;
	else
		scalarType = FloatType;
}


Simulation::~Simulation()
{
	LogDebug("Waiting for all running computations to finish");
	Finish();
	for(std::list<Writer*>::iterator i = exporters.begin(); i != exporters.end(); i++)
		(*i)->Finish();

	LogDebug("Destroying simulation object");
	delete program;

	LogDebug("Destroying exporters"); // TODO fix this
	/*for(std::list<Writer*>::iterator i = exporters.begin(); i != exporters.end();)
	{
		Writer* w = *i;
		i++;
		delete w;
	}*/

	Log::Send(Log::Info, "Simulation: " + name + " destroyed");
}


void Simulation::SetSmoothingKernel(SmoothingKernelType type, double length, bool correction)
{
	smoothingKernel = type;
	smoothingLength = length;
	smoothingKernelCorrection = correction;
}


void Simulation::SetDensity( double density )
{
	if(density < DBL_EPSILON)
	{
		Log::Send(Log::Error, "Fluid density must be greater than zero");
		return;
	}

	this->density = density;
}


void Simulation::SetGravity(Vec<3,double> acceleration)
{
	gravity = acceleration;
}


void Simulation::SetViscosity( double viscosity )
{
	dynamicViscosity = viscosity;
}


void Simulation::SetBoundaries(Vec<3,double> boundsMin, Vec<3,double> boundsMax)
{
	gridMin = boundsMin;
	gridMax = boundsMax;
	gridSize = gridMax - gridMin;
}


bool Simulation::LoadSubprogram(const std::string& name, const std::string& source)
{
	CLSubProgram *sp;
	std::map<std::string,CLSubProgram*>::iterator found = subprograms.find(name);

	if(found == subprograms.end())
	{
		sp = new CLSubProgram();
		subprograms[name] = sp;
	}
	else
		sp = found->second;

   bool success = sp->SetSource(source);
	program->AddSubprogram(sp);
	return success;
}


bool Simulation::EnqueueSubprogram(const std::string& name, size_t globalSize, size_t localSize)
{
	std::map<std::string,CLSubProgram*>::iterator found = subprograms.find(name);

	if(found != subprograms.end())
	{
		CLSubProgram *sp = found->second;

		if(!sp->Enqueue(globalSize, localSize))
		{
			Log::Send(Log::Error, "Error while executing subprogram: " + sp->KernelName());
			return false;
		}
		return true;

		/*if(sp->Enqueue(globalSize, localSize)) // for debugging
		{
			if(!program->Finish())
			{
				Log::Send(Log::Error, "Error while waiting for subprogram: " + sp->KernelName());
				return false;
			}

			return true;
		}
		else
		{
			Log::Send(Log::Error, "Error while executing subprogram: " + sp->KernelName());
			return false;
		}*/

	}
	
	Log::Send(Log::Error, "Subprogram you want to enqueue doesn't exist");
	return false;
}


bool Simulation::InitSimulationVariable(const std::string& semantic, VariableDataType dataType, bool constant)
{
	if(constant)
	{
		CLProgramConstant *var = program->Constant(semantic);
		if(!var)
			var = new CLProgramConstant(program, semantic);
		var->SetSpace(dataType, 1);
	}
	else
	{
		CLKernelArgument *var = program->Argument(semantic);
		if(!var)
			var = new CLKernelArgument(program, semantic);
		var->SetSpace(dataType, 1);
	}
	return true;
}

bool Simulation::InitSimulationVariable(const std::string& semantic, VariableDataType dataType, double data, bool constant)
{
	if(constant)
	{
		CLProgramConstant *var = program->Constant(semantic);
		if(!var)
			var = new CLProgramConstant(program, semantic);
		var->SetSpace(dataType, 1);
		return var->SetScalar(data);
	}
	else
	{
		CLKernelArgument *var = program->Argument(semantic);
		if(!var)
			var = new CLKernelArgument(program, semantic);
		var->SetSpace(dataType, 1);
		return var->SetScalar(data);
	}
}

bool Simulation::InitSimulationVariable(const std::string& semantic, VariableDataType dataType, Vec<3,double> data, bool constant)
{
	if(constant)
	{
		CLProgramConstant *var = program->Constant(semantic);
		if(!var)
			var = new CLProgramConstant(program, semantic);
		var->SetSpace(dataType, 1);
		return var->SetVector(data);
	}
	else
	{
		CLKernelArgument *var = program->Argument(semantic);
		if(!var)
			var = new CLKernelArgument(program, semantic);
		var->SetSpace(dataType, 1);
		return var->SetVector(data);
	}
}


CLGlobalBuffer* Simulation::InitSimulationBuffer(const std::string& semantic, VariableDataType dataType, unsigned int elementCount)
{
	CLGlobalBuffer *var = program->Buffer(semantic);
	if(!var)
		var = new CLGlobalBuffer(program, semantic);
	var->SetSpace(dataType, elementCount);
	return var;
}


void Simulation::SetDevices( CLLink* linkToDevices )
{
	program->SetLink(linkToDevices);
}


bool Simulation::Initialize()
{
	Log::Send(Log::Info, "Initializing the simulation");

	// release
	program->ClearBuildOptions();
	program->ClearSubprograms();

   if (scalarType == FloatType)
      program->AddBuildOption("-cl-single-precision-constant");

	// create

	if(!InitGeneral())
	{
		Log::Send(Log::Error, "Setting up general simulation stuff failed");
		return false;
	}

	if(!InitGrid())
	{
		Log::Send(Log::Error, "Initializing simulation grid failed");
		return false;
	}

	// kernel correction inited after grid functions
	if(smoothingKernelCorrection)
	{
      //LoadSubprogram("kernel correction", "kernels/correction.cl");
      LoadSubprogram("kernel correction",
                     #include "kernels/correction.cl"
                     );
		InitSimulationBuffer("KERNEL_CORRECTION", dimensions == 2 ? Scalar4DataType() : Scalar8DataType(), deviceParticleCount);
		program->AddBuildOption("-D CORRECT_KERNEL");
	}
	else
		InitSimulationBuffer("KERNEL_CORRECTION", ScalarDataType(), 1); // todo dummy buffer, gotta implement ifdef for kernel args

	// general particle variables
	classBuffer = InitSimulationBuffer("CLASS", CharType, deviceParticleCount);
	massesBuffer = InitSimulationBuffer("MASSES", ScalarDataType(), deviceParticleCount);
	densitiesBuffer = InitSimulationBuffer("DENSITIES", ScalarDataType(), deviceParticleCount);
	pressuresBuffer = InitSimulationBuffer("PRESSURES", ScalarDataType(), deviceParticleCount);
	positionsBuffer = InitSimulationBuffer("POSITIONS", VectorDataType(), deviceParticleCount);
	velocitiesBuffer = InitSimulationBuffer("VELOCITIES", VectorDataType(), deviceParticleCount);
	normalsBuffer = InitSimulationBuffer("NORMALS", VectorDataType(), deviceParticleCount);  // needed for shit
	InitSimulationBuffer("INITIAL_POSITIONS", VectorDataType(), deviceParticleCount); // needed for moving objects
	// for coalesced memory access
	/*InitSimulationBuffer("SORTED_MASSES", ScalarDataType(), deviceParticleCount);
	InitSimulationBuffer("SORTED_POSITIONS", VectorDataType(), deviceParticleCount);
	InitSimulationBuffer("SORTED_VELOCITIES", VectorDataType(), deviceParticleCount);
	InitSimulationBuffer("SORTED_DENSITIES", ScalarDataType(), deviceParticleCount);*/

	// build geometry expressions
	std::multimap<std::string,Geometry*>::iterator it;

	for(it=models.begin(); it != models.end(); it++)
	{
		// expressions
		if(it->second->ExpressionInit())
		{
			CLSubProgram *sp = new CLSubProgram();
			subprograms["init_" + it->second->Name()] = sp;
			sp->SetSource(it->second->GetInitCode());
			program->AddSubprogram(sp);
		}
		if(it->second->ExpressionMovement())
		{
			CLSubProgram *sp = new CLSubProgram();
			subprograms["move_" + it->second->Name()] = sp;
			sp->SetSource(it->second->GetMovementCode());
			program->AddSubprogram(sp);
		}
	}

	// init SPH
	if(!InitSph())
	{
		Log::Send(Log::Error, "Initializing SPH stuff failed");
		return false;
	}

	// prepare exporters
	for(std::list<Writer*>::iterator i = exporters.begin(); i != exporters.end(); i++)
		if(!(*i)->Prepare())
		{
			Log::Send(Log::Error, "Initializing one of the exporters failed.");
			return false;
		}

	// build
	if(!program->Build())
	{
		Log::Send(Log::Error, "Building the simulation program failed");
		return false;
	}

	// particle/cell sorter
	clppSetup = new clppContext();
	clppSetup->setup(program->Link()->Platform()->ID(), program->Link()->Device(0)->ID(), program->Link()->Context(), program->Link()->Queue(0));
	clppSorter = clpp::createBestSortKV(clppSetup, deviceParticleCount, 32);
	clppSorter->pushCLDatas(program->Buffer("HASHES")->Buffer(0), deviceParticleCount);

	// precompute tensile correction kernel dP constant
	EnqueueSubprogram("deltaP", 1, 1);
	if(!program->Finish())
	{
		Log::Send(Log::Error, "Precomputing the tensile correction failed");
		return false;
	}
	double dp = 1.0 / program->Buffer("DELTA_P")->GetScalar();
	program->Argument("DELTA_P_INV")->SetScalar(dp);

	// setup geometry particles
	InitGeometry();

	PostInitSph();

	program->Buffer("INITIAL_POSITIONS")->CopyFrom(program->Buffer("POSITIONS"), true);

	return true;
}


bool Simulation::InitGrid()
{
	LogDebug("Initializing simulation grid");

	// to be sure extend boundaries by particle spacing
	SetBoundaries(gridMin - Vec<3,double>(gridCellSize), gridMax + Vec<3,double>(gridCellSize));

	gridCellCount.x = static_cast<int>(ceil(gridSize.x / gridCellSize));
	gridCellCount.y = static_cast<int>(ceil(gridSize.y / gridCellSize));
	unsigned int cells = gridCellCount.x * gridCellCount.y;
	if(dimensions == 3)
	{
		gridCellCount.z = static_cast<int>(ceil(gridSize.z / gridCellSize));
		cells *= gridCellCount.z;
	}

	// test if all needed parameters are set
	if(!gridCellCount.x || !gridCellCount.y)
	{
		Log::Send(Log::Error, "You need to correctly set simulation boundaries");
		return false;
	}

	// variables
	InitSimulationBuffer("CELLS_START", UintType, Utils::NearestMultiple(cells, 1024));
	InitSimulationBuffer("HASHES", Int2Type, deviceParticleCount);
	InitSimulationVariable("GRID_START", VectorDataType(), gridMin, true);
	InitSimulationVariable("GRID_END", VectorDataType(), gridMax, true);
	InitSimulationVariable("CELL_SIZE", ScalarDataType(), gridCellSize, true);
	InitSimulationVariable("CELL_SIZE_INV", ScalarDataType(), 1.0 / gridCellSize, true);
	InitSimulationVariable("CELL_COUNT", VectorDataType(false), gridCellCount, true);
	InitSimulationVariable("CELL_COUNT_1", VectorDataType(false), gridCellCount - Vec<3,int>(1), true);

	// subprograms
   LoadSubprogram("grid utils",
                  #include "scene/grid_utils.cl"
                  );
   LoadSubprogram("clear grid",
                  #include "scene/grid_clear.cl"
                  );
   LoadSubprogram("set cell ids",
                  #include "scene/grid_cellids.cl"
                  );
   LoadSubprogram("set cell start",
                  #include "scene/grid_cellstart.cl"
                  );
   LoadSubprogram("out of bounds",
                  #include "scene/out_of_bounds.cl"
                  );

	return true;
}


bool Simulation::InitGeneral()
{
	if(!smoothingLength)
	{
		Log::Send(Log::Error, "You need to set smoothing length");
		return false;
	}

	if(density < DBL_EPSILON)
	{
		Log::Send(Log::Error, "You need to set density");
		return false;
	}

	if(particleSpacing < DBL_EPSILON)
	{
		Log::Send(Log::Error, "You need to set particle spacing");
		return false;
	}

	// Set kernel support radius
   double supportRadius;
	switch(smoothingKernel)
	{
	case QuadraticKernel:
	case CubicSplineKernel:
	case GaussKernel:
	case WendlandKernel:
      supportRadius = 2 * smoothingLength;
		break;
	case QuinticSplineKernel:
	case ModifiedGaussKernel:
      supportRadius = 3 * smoothingLength;
		break;
	}

   gridCellSize = supportRadius;

	if(!PreProcessGeometry())
		return false;

	// general simulation variables
	timeStepCount = 0;
	timeOverall = 0;
	timeStep = 0;

	double particleVolume = pow(particleSpacing, (int)dimensions);
	particleMass[FluidParticle] = particleMass[DummyParticle] = particleVolume * density;
	particleMass[BoundaryParticle] = particleMass[FluidParticle] / 2;

	InitSimulationVariable("PARTICLE_COUNT", UintType, particleCount, true);
	InitSimulationVariable("FLUID_PARTICLE_COUNT", UintType, particleCountByType[FluidParticle], true);
	InitSimulationVariable("BOUNDARY_PARTICLE_COUNT", UintType, particleCountByType[BoundaryParticle], true);
	InitSimulationVariable("DUMMY_PARTICLE_COUNT", UintType, particleCountByType[DummyParticle], true);
	InitSimulationVariable("PARTICLE_SPACING", ScalarDataType(), particleSpacing, true);

	InitSimulationVariable("MASS", ScalarDataType(), particleMass[FluidParticle], true);
	InitSimulationVariable("VOLUME", ScalarDataType(), particleVolume, true);

	InitSimulationVariable("GRAVITY", VectorDataType(), gravity, true);

   InitSimulationVariable("KERNEL_SUPPORT_RADIUSES", VectorDataType(), Vec<3,double>(supportRadius), true);
   InitSimulationVariable("KERNEL_SUPPORT_RADIUS", ScalarDataType(), supportRadius, true);

	InitSimulationVariable("SMOOTHING_LENGTH", ScalarDataType(), smoothingLength, true);
	InitSimulationVariable("SMOOTHING_LENGTH_SQ", ScalarDataType(), smoothingLength*smoothingLength, true);
	InitSimulationVariable("SMOOTHING_LENGTH_INV", ScalarDataType(), 1.0/smoothingLength, true);
	InitSimulationVariable("SMOOTHING_LENGTH_INV_SQ", ScalarDataType(), 1.0/smoothingLength/smoothingLength, true);
	InitSimulationVariable("SMOOTHING_LENGTH_INV_2", ScalarDataType(), 1.0/smoothingLength/smoothingLength, true);
	InitSimulationVariable("SMOOTHING_LENGTH_INV_3", ScalarDataType(), 1.0/pow(smoothingLength, 3), true);
	InitSimulationVariable("SMOOTHING_LENGTH_INV_4", ScalarDataType(), 1.0/pow(smoothingLength, 4), true);
	InitSimulationVariable("SMOOTHING_LENGTH_INV_5", ScalarDataType(), 1.0/pow(smoothingLength, 5), true);

	InitSimulationVariable("DENSITY", ScalarDataType(), density, true);
	InitSimulationVariable("DENSITY_INV", ScalarDataType(), 1.0 / density, true);
	InitSimulationVariable("DENSITY_INV_SQ", ScalarDataType(), 1.0 / (density*density), true);
    
	InitSimulationVariable("DIST_EPSILON", ScalarDataType(), 0.001*(smoothingLength * smoothingLength), true);

	InitSimulationVariable("TIME", ScalarDataType(), 0, false);
	InitSimulationVariable("TIME_STEP", ScalarDataType(), 0, false);
	InitSimulationVariable("TIME_STEP_INV", ScalarDataType(), 0, false);
	InitSimulationVariable("HALF_TIME_STEP", ScalarDataType(), 0, false);
	InitSimulationVariable("TIME_STEP_6", ScalarDataType(), 0, false);
	InitSimulationBuffer("NEXT_TIME_STEP", ScalarDataType(), 1);

	// local stuff
	CLLocalBuffer* lvar;
	lvar = new CLLocalBuffer(program, "LOCAL_SCALAR"); lvar->SetSpace(ScalarDataType(), 1);
	lvar = new CLLocalBuffer(program, "LOCAL_VECTOR"); lvar->SetSpace(VectorDataType(), 1);

	// for tensile correction
	InitSimulationBuffer("DELTA_P", ScalarDataType(), 1);
	InitSimulationVariable("DELTA_P_INV", ScalarDataType(), 0, false);

	// program build options
	if(dimensions == 2)
		program->AddBuildOption("-D DIM=2");
	else
		program->AddBuildOption("-D DIM=3");

	if(scalarType == FloatType)
		program->AddBuildOption("-D FP=32");
	else
		program->AddBuildOption("-D FP=64");

	// viscosity
	InitSimulationVariable("VISCOSITY", ScalarDataType(), dynamicViscosity, true);
	InitSimulationVariable("DYNAMIC_VISCOSITY", ScalarDataType(), dynamicViscosity, true);
	InitSimulationVariable("KINEMATIC_VISCOSITY", ScalarDataType(), dynamicViscosity / density, true);
	
	// subprograms
   LoadSubprogram("types",
                  #include "general/types.cl"
                  );
   LoadSubprogram("max velocity",
                  #include "general/max_vel.cl"
                  );
	InitSimulationBuffer("OUT_MAX_VELOCITIES", ScalarDataType(), 2 * Devices()->Device(0)->ComputeUnits());
	CLLocalBuffer *var = new CLLocalBuffer(program, "LOCAL_MAX_VELOCITIES");  var->SetSpace(ScalarDataType(), 2 * 256);

	switch(smoothingKernel)
	{
	case QuadraticKernel:
      LoadSubprogram("kernel",
                     #include "kernels/quadratic.cl"
                     );
		program->AddBuildOption("-D QUADRATIC");
		break;
	case CubicSplineKernel:
      LoadSubprogram("kernel",
                     #include "kernels/cubic.cl"
                     );
		program->AddBuildOption("-D CUBIC");
		break;
	case QuinticSplineKernel:
      LoadSubprogram("kernel",
                     #include "kernels/quintic.cl"
                     );
		program->AddBuildOption("-D QUINTIC");
		break;
	case GaussKernel:
      LoadSubprogram("kernel",
                     #include "kernels/gauss.cl"
                     );
		program->AddBuildOption("-D GAUSS");
		break;
	case ModifiedGaussKernel:
      LoadSubprogram("kernel",
                     #include "kernels/gaussmod.cl"
                     );
		program->AddBuildOption("-D GAUSS");
		break;
	case WendlandKernel:
      LoadSubprogram("kernel",
                     #include "kernels/wendland.cl"
                     );
		program->AddBuildOption("-D WENDLAND");
		break;
	}

   LoadSubprogram("deltaP",
                  #include "kernels/delta_p.cl"
                  );
	
	return true;
}


void Simulation::SetParticleSpacing( double spacing )
{
	particleSpacing = spacing;
}


bool Simulation::RunGrid()
{
	LogDebug("Refresing uniform grid");

	// clear grid
	EnqueueSubprogram("clear grid");
	EnqueueSubprogram("set cell ids");

	// sort
	if(clppSorter)
		clppSorter->sort();

	// set cell start
	EnqueueSubprogram("set cell start");

	return true;
}


void RunNewExportThread(void* exporterData)
{
	Writer *exporter = (Writer*)exporterData;
	Simulation* sim = exporter->ParentSimulation();
	tthread::lock_guard<tthread::mutex> lock(sim->exporterMutex);
	exporter->WriteData();
}


bool Simulation::Advance( double advanceTimeStep )
{
	LogDebug("Advancing simulation");

	if(!program->IsBuilt())
	{
		Log::Send(Log::Error, "Cannot run unbuilt simulation");
		return false;
	}

	if(Time() < DBL_EPSILON)
	{
		for(std::list<Writer*>::iterator i = exporters.begin(); i != exporters.end(); i++)
		{
			(*i)->PrepareData();
			(*i)->WriteData();
		}
	}

	// for profiling this time step
	timer.Start();

	// upload particle data if it changed
	if(!UploadModifiedBuffers())
	{
		Log::Send(Log::Error, "Failed to send particle data to devices.");
		return false;
	}

	// update time step vars
	timeStep = advanceTimeStep;
	program->Argument("TIME_STEP")->SetScalar(timeStep);
	program->Argument("TIME_STEP_INV")->SetScalar(1.0 / timeStep);
	program->Argument("HALF_TIME_STEP")->SetScalar(timeStep / 2.0);
	program->Argument("TIME_STEP_6")->SetScalar(timeStep / 6.0);

	// move objects if needed
	for(std::multimap<std::string,Geometry*>::iterator it=models.begin(); it != models.end(); it++)
		if(it->second->ExpressionMovement())
		{
			if(timeOverall >= it->second->startMovementTime && (timeOverall <= it->second->endMovementTime || it->second->endMovementTime < DBL_EPSILON))
			EnqueueSubprogram("move_" + it->second->Name(), Utils::NearestMultiple(it->second->ParticleCount(), 256), 256);
		}

	// run the SPH simulation on devices
	if(!RunSph())
		return false;

	if(!EnqueueSubprogram("out of bounds"))
		return false;

	// advance sim time
	timeOverall += advanceTimeStep; 
	program->Argument("TIME")->SetScalar(timeOverall);
	timeStepCount++;

	// auto manage export
	for(std::list<Writer*>::iterator i = exporters.begin(); i != exporters.end(); i++)
	{
		Writer *w = *i;
		if((abs(w->ExportTimeStep()) > DBL_EPSILON && timeOverall >= w->ExportTimeStep() * w->exportedTimeStepsCount - 1e-9)
			|| (!w->exportTimes.empty() && timeOverall >= w->exportTimes.front() - 1e-9))
		{
			if(w->enqueuedThread)
			{
				w->enqueuedThread->join();
				delete w->enqueuedThread;
				w->enqueuedThread = NULL;
			}

			program->Finish();
			w->PrepareData();

			if(asyncExport)
				w->enqueuedThread = new tthread::thread(RunNewExportThread, w);
			else
				w->WriteData();
		}
	}

	// profile this time step
	timeStepTimer = timer.Time();
	
	return true;
}


bool Simulation::UploadModifiedBuffers()
{
	bool positionsHaveChanged = false;
	bool success = true;

	if(positionsBuffer->HostDataChanged()) 
		positionsHaveChanged = true;

	for (std::map<std::string, CLGlobalBuffer*>::const_iterator i = program->Buffers().begin(); i != program->Buffers().end(); i++)
		success &= i->second->Upload();

	// since we changed particles, refresh the grid
	if(success && positionsHaveChanged)
		success &= RunGrid();

	return success;
}


size_t Simulation::UsedMemorySize()
{
	return program->UsedMemorySize();
}


double Simulation::SuggestTimeStep()
{
	return timeStep;
}


Geometry* Simulation::GetGeometry( const std::string& name )
{
	std::multimap<std::string,Geometry*>::iterator it = models.find(name);
	if(it == models.end())
	{
		Log::Send(Log::Warning, "Couldn't find geometry: " + name);
		return NULL;
	}
	return it->second;
}


VariableDataType Simulation::Scalar2DataType()
{
	return (scalarType == DoubleType) ? Double2Type : Float2Type;
}


VariableDataType Simulation::Scalar4DataType()
{
	return (scalarType == DoubleType) ? Double4Type : Float4Type;
}


VariableDataType Simulation::Scalar8DataType()
{
	return (scalarType == DoubleType) ? Double8Type : Float8Type;
}


VariableDataType Simulation::VectorDataType(bool realNumbers)
{
	if(realNumbers)
	{
		if(dimensions == 2 && scalarType == FloatType)
			return Float2Type;
		else if(dimensions == 2 && scalarType == DoubleType)
			return Double2Type;
		else if(dimensions == 3 && scalarType == FloatType)
			return Float4Type;
		else
			return Double4Type;
	}
	else
	{
		return (dimensions == 2) ? Int2Type : Int4Type;
	}
}

VariableDataType Simulation::ScalarDataType()
{
	return scalarType;
}

void Simulation::SetName(const std::string& simName)
{
	name = simName;
}

void Simulation::SetDescription( const std::string& simDescription )
{
	desc = simDescription;
}

CLLink* Simulation::Devices()
{
	if(program)
		return program->Link();
	return NULL;
}

bool Simulation::Run()
{
	while(Time() < maxTime)
	{
		if(wantedTimeStep > DBL_EPSILON)
		{
			if(!Advance(wantedTimeStep))
				return false;
		}
		else
		{
			if(!Advance(cflFactor * SuggestTimeStep()))
				return false;
		}
	}
	Finish();
	return true;
}

void Simulation::SetRunTime(double time, double timeStep, double autoFactor)
{
	maxTime = time;
	wantedTimeStep = timeStep;
	cflFactor = autoFactor;
}

void Simulation::SetAsyncExport( bool enabled )
{
	asyncExport = enabled;
}

void Simulation::Finish()
{
	for(std::list<Writer*>::iterator i = exporters.begin(); i != exporters.end(); i++)
	{
		Writer *w = *i;
		if(w->enqueuedThread)
		{
			w->enqueuedThread->join();
			delete w->enqueuedThread;
			w->enqueuedThread = NULL;
		}
	}

	if(program)
		program->Finish();
}

double Simulation::MaximumVelocity()
{
	CLGlobalBuffer* out = program->Buffer("OUT_MAX_VELOCITIES");

	// enqueue reduction kernel and get the result
	int localSize = Devices()->Device(0)->IsCPU() ? 1 : 256;
	if(!this->EnqueueSubprogram("max velocity", localSize * out->Elements(), localSize))
		return 0.0;
	if(!out->Download(false, true))
		return 0.0;
	program->Finish();

	// get maximum from downloaded data
	double maxVel = 0.0;
   for(size_t i=0; i < out->Elements(); i++)
		maxVel = (std::max)(maxVel, out->GetScalar(i));

	return sqrt(maxVel);
}

struct QueuedParticle 
{
	int xMin, xMax, y, upDownDir;
	bool goLeft, goRight;
};

unsigned int Simulation::MakeFluid(Vec<3,double> source, bool onlyCountParticles)
{
	std::stack<QueuedParticle> analyzeQueue;
	std::set<int> createdLocations;
	QueuedParticle q = {0, 0, 0, 0, true, true};
	analyzeQueue.push(q);
	int hash;
	if(ParticleCollision(source))
		return 0;

	while (!analyzeQueue.empty())
	{
		q = analyzeQueue.top();
		analyzeQueue.pop();

		hash = Utils::PackIntegerPair(q.xMin, q.y);
		if(!createdLocations.count(hash))
		{
			if(!onlyCountParticles)
				InitFluidParticle(createdLocations.size(), source + particleSpacing * Vec<3,double>(q.xMin, q.y));
			createdLocations.insert(hash);

			if(!ParticleCollision(source + particleSpacing * Vec<3,double>(q.xMin+1, q.y)))
			{
				QueuedParticle w = {q.xMin+1, 0, q.y, 0, true, true};
				analyzeQueue.push(w);
			}    
			if(!ParticleCollision(source + particleSpacing * Vec<3,double>(q.xMin-1, q.y)))
			{
				QueuedParticle w = {q.xMin-1, 0, q.y, 0, true, true};
				analyzeQueue.push(w);
			}    
			if(!ParticleCollision(source + particleSpacing * Vec<3,double>(q.xMin, q.y+1)))
			{
				QueuedParticle w = {q.xMin, 0, q.y+1, 0, true, true};
				analyzeQueue.push(w);
			}    
			if(!ParticleCollision(source + particleSpacing * Vec<3,double>(q.xMin, q.y-1)))
			{
				QueuedParticle w = {q.xMin, 0, q.y-1, 0, true, true};
				analyzeQueue.push(w);
			}    
		}
	}

	/*if(!onlyCountParticles)
		InitFluidParticle(0, source);
	createdLocations.insert(0);

	while (!analyzeQueue.empty())
	{
		q = analyzeQueue.back();
		analyzeQueue.pop();

		int xMin = q.xMin;
		int xMax = q.xMax;

		if(q.goLeft) {
			while(!ParticleCollision(source + particleSpacing * Vec<3,double>(xMin-1, q.y)))
			{
				xMin--;
				hash = Utils::PackIntegerPair(xMin, q.y);
				if(!createdLocations.count(hash))
				{
					if(!onlyCountParticles)
						InitFluidParticle(createdLocations.size(), source + particleSpacing * Vec<3,double>(xMin, q.y));
					createdLocations.insert(hash);
				}
			}
		}

		if(q.goRight) {
			while(!ParticleCollision(source + particleSpacing * Vec<3,double>(xMax+1, q.y)))
			{
				xMax++;
				hash = Utils::PackIntegerPair(xMax, q.y);
				if(!createdLocations.count(hash))
				{
					if(!onlyCountParticles)
						InitFluidParticle(createdLocations.size(), source + particleSpacing * Vec<3,double>(xMax, q.y));
					createdLocations.insert(hash);
				}
			}
		}

		// extend range ignored from previous line
		q.xMin--; q.xMax++;

		int xMinR;
		bool inRange;
		int x;

		// going in one direction
		xMinR = xMin;
		inRange = false;
		for(x = xMin; x <= xMax; x++)
		{
			// skip testing, if testing previous line within previous range
			bool empty = (q.upDownDir>=0 || (x<q.xMin || x>q.xMax)) && !ParticleCollision(source + particleSpacing * Vec<3,double>(x, q.y-1));
			if(!inRange && empty)
			{
				xMinR = x;
				inRange = true;
			}
			else if(inRange && !empty)
			{
				QueuedParticle new_q = {xMinR, x-1, q.y-1, 1, xMinR==xMin, false};
				analyzeQueue.push(new_q);
				inRange = false;
			}
			if(inRange)
			{
				hash = Utils::PackIntegerPair(x, q.y-1);
				if(!createdLocations.count(hash))
				{
					if(!onlyCountParticles)
						InitFluidParticle(createdLocations.size(), source + particleSpacing * Vec<3,double>(x, q.y-1));
					createdLocations.insert(hash);
				}
			}
			// skip
			if(q.upDownDir<0 && x == q.xMin)
				x = q.xMax;
		}

		if(inRange)
		{
			QueuedParticle new_q = {xMinR, x-1, q.y-1, 1, xMinR==xMin, true};
			analyzeQueue.push(new_q);
		}

		// going in different direction
		xMinR = xMin;
		inRange = false;
		for(x = xMin; x <= xMax; x++)
		{
			// skip testing, if testing previous line within previous range
			bool empty = (q.upDownDir<=0 || (x<q.xMin || x>q.xMax)) && !ParticleCollision(source + particleSpacing * Vec<3,double>(x, q.y+1));
			if(!inRange && empty)
			{
				xMinR = x;
				inRange = true;
			}
			else if(inRange && !empty)
			{
				QueuedParticle new_q = {xMinR, x-1, q.y+1, -1, xMinR==xMin, false};
				analyzeQueue.push(new_q);
				inRange = false;
			}
			if(inRange)
			{
				hash = Utils::PackIntegerPair(x, q.y+1);
				if(!createdLocations.count(hash))
				{
					if(!onlyCountParticles)
						InitFluidParticle(createdLocations.size(), source + particleSpacing * Vec<3,double>(x, q.y+1));
					createdLocations.insert(hash);
				}
			}
			// skip
			if(q.upDownDir>0 && x == q.xMin)
				x = q.xMax;
		}

		if(inRange)
		{
			QueuedParticle new_q = {xMinR, x-1, q.y+1, -1, xMinR==xMin, true};
			analyzeQueue.push(new_q);
		}

	}*/

	return createdLocations.size();
}

bool Simulation::ParticleCollision(const Vec<3,double>& position)
{
	if(position.x > gridMax.x || position.y > gridMax.y)
		return true;
	if(position.x < gridMin.x || position.y < gridMin.y)
		return true;

	double radius = particleSpacing * 0.51;

	for(std::multimap<std::string,Geometry*>::iterator it=models.begin(); it != models.end(); it++)
		if(it->second->ParticleCollision(position, radius))
			return true;

	return false;
}

void Simulation::SetBucketFillLocation( Vec<3,double> location )
{
	floodFillLocation = location;
}

void Simulation::InitFluidParticle(unsigned int id, const Vec<3,double>& position)
{
	Particle p = GetParticle(id);
	p.SetType(FluidParticle);
	p.SetPosition(position);
	p.SetDensity(Density());
	p.SetMass(particleMass[FluidParticle]);
	p.SetPressure(0.);
	p.SetVelocity(Vec<3,double>());
}

void Simulation::InitDummyParticle(unsigned int id, const Vec<3,double>& position)
{
	Particle p = GetParticle(id);
	p.SetType(DummyParticle);
	p.SetPosition(position);
	p.SetDensity(Density());
	p.SetMass(particleMass[FluidParticle]);
	p.SetPressure(0.);
	p.SetVelocity(Vec<3,double>());
	/*if(position.y >= 0.9999)
		p.SetVelocity(Vec<3,double>(1, 0));*/
}

unsigned int Simulation::InitDummies(unsigned int wallId, const Vec<3,double>& pos, const Vec<3,double>& normal, bool onlyCountParticles)
{
	unsigned int layers = (unsigned int)(gridCellSize / particleSpacing);

	if(!onlyCountParticles)
		for (unsigned int i=1; i<=layers; i++)
			InitDummyParticle(wallId + i, pos - normal * (i*particleSpacing));

	return layers;
}

unsigned int Simulation::InitDummyCorner(unsigned int wallId, const Vec<2,double>& pos, const Vec<2,double>& n1, const Vec<2,double>& n2, bool onlyCountParticles)
{
	unsigned int count = 0;
	double angle = acos(n1.Dot(n2));
	int layers = (int)(gridCellSize / particleSpacing);

	for (int i=1; i<=layers; i++)
	{
		double arcLength = angle * i * particleSpacing;
		int arcSegments = (int)(arcLength / particleSpacing + 0.5); // round to nearest integer
		double segmentAngle = angle / arcSegments;

		Vec<2,double> rotVec = -n1 * (i*particleSpacing);
		Vec<2,double> p1 = pos + rotVec;
		Vec<2,double> p2 = pos - n2 * (i*particleSpacing);

		if((p1-p2).Length() < 0.5 * particleSpacing)
		{
			count++;
			if(!onlyCountParticles) InitDummyParticle(wallId + count, (p1+p2)*0.5);
		}
		else 
		{
			count += 2;
			if(!onlyCountParticles)
			{
				InitDummyParticle(wallId + count - 1, p1);
				InitDummyParticle(wallId + count, p2);
			}
		}

		if(arcSegments < 2)
			continue;

		Vec<2,double> pj = pos + rotVec.Rotate(segmentAngle);
		int increment = (pj-p2).LengthSq() < (p1-p2).LengthSq() ? 1 : -1;

		for (int j=1; j<arcSegments; j++)
		{
			pj = pos + rotVec.Rotate(increment * j * segmentAngle);
			count++;
			if(!onlyCountParticles) InitDummyParticle(wallId + count, pj);
		}
	}
	
	return count;
}


bool Simulation::InitGeometry()
{
	MakeFluid(floodFillLocation, false);

	std::multimap<std::string,Geometry*>::iterator it;
	for(it=models.begin(); it != models.end(); it++)
		if(it->second->Type() == BoundaryParticle)
			it->second->Build(false);

	InitOverlappingCorners(false);

	if(!UploadModifiedBuffers())
	{
		Log::Send(Log::Error, "Failed to send particles positions to devices.");
		return false;
	}

	for(it=models.begin(); it != models.end(); it++)
		if(it->second->ExpressionInit())
		{
			if(it->second->Type() == FluidParticle)
				EnqueueSubprogram("init_" + it->second->Name());
			else
				EnqueueSubprogram("init_" + it->second->Name(), Utils::NearestMultiple(it->second->ParticleCount(), 256), 256);
		}

	RunGrid();

	if(!program->Finish())
	{
		Log::Send(Log::Error, "Initializing particles in OpenCL failed.");
		return false;
	}

	return true;
}

bool Simulation::PreProcessGeometry()
{
	std::multimap<std::string,Geometry*>::iterator it;

	// find overlapping corners
	std::vector<Geometry::Corner> corners;
	for(it=models.begin(); it != models.end(); it++)
	{
		if(it->second->Type() != BoundaryParticle) // it's free surface geometry
			continue;
		std::vector<Geometry::Corner> objCorners = it->second->Corners();
      for(size_t i=0; i<objCorners.size(); i++)
		{
         for(size_t j=0; j<corners.size(); j++)
			{
				if((objCorners[i].position - corners[j].position).Length() < 1e-9)
				{
					OverlappingCorner c = {objCorners[i], corners[j]};
					overlappingCorners.push_back(c);
					break;
				}
			}
			corners.push_back(objCorners[i]);
		}
	}

	// count the particles
	for (int i=0; i<(int)ParticleTypeCount; i++)
		particleCountByType[i] = 0;

	particleCount = MakeFluid(floodFillLocation, true);
	particleCountByType[FluidParticle] += particleCount;

	for(it=models.begin(); it != models.end(); it++)
		if(it->second->Type() == BoundaryParticle)
		{
			it->second->startId = particleCount;
			particleCount += it->second->ParticleCount();
		}

	overlappedCornersStart = particleCount;
	particleCount += InitOverlappingCorners(true);

	if(!particleCount || !particleCountByType[FluidParticle])
	{
		Log::Send(Log::Error, "You need to make some geometry for the simulation");
		return false;
	}

	deviceParticleCount = Utils::NearestMultiple(particleCount, 1024);

	return true;
}

bool Simulation::IsCornerOverlapping( const Vec<3,double>& position )
{
   for(size_t i=0; i<overlappingCorners.size(); i++)
		if ((overlappingCorners[i].c1.position - position).Length() < 1e-9)
			return true;

	return false;
}

unsigned int Simulation::InitOverlappingCorners(bool onlyCountParticles)
{
	unsigned int count = 0;
   for(size_t i=0; i<overlappingCorners.size(); i++)
	{
		OverlappingCorner oc = overlappingCorners[i];

		if(!onlyCountParticles)
		{
			Particle p = GetParticle(overlappedCornersStart + count);
			p.SetType(BoundaryParticle);
			p.SetPosition(oc.c1.position);
			p.SetDensity(Density());
			p.SetMass(particleMass[FluidParticle]);
			p.SetPressure(0.);
			p.SetVelocity(Vec<3,double>());
			/*if(oc.c1.position.y >= 0.9999)
				p.SetVelocity(Vec<3,double>(1, 0));*/
		}

		double dot = oc.c1.normal.Dot(oc.c2.tangent);
		
		if(dot <= 0.)
			count += 1 + InitDummyCorner(overlappedCornersStart + count, oc.c1.position, oc.c1.normal, oc.c2.normal, onlyCountParticles);
		//else
			// put dummies on bisector
	}
	return count;
}
