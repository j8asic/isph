#include "isph.h"
using namespace isph;


WcsphSimulation::WcsphSimulation(unsigned int simDimensions, VariableDataType scalarDataType)
	: Simulation(simDimensions, scalarDataType)
	, wcSpeedOfSound(0)
	, wcGamma(7)
	, xsphFactor(0.5)
	, densityReinitMethod(None)
	, densityReinitFrequency(-1)
	, tcEpsilon1(0.1)
	, tcEpsilon2(0.01)
	, initDensityFromPressure(false)
	, initMassFromDensity(false)
	, initedWcsphStuff(false)
	, integratorType(PredictorCorrector)
	, integratorGridRefresh(true)
	, viscosityFormulation(LaminarViscosity)
{
}


WcsphSimulation::~WcsphSimulation()
{
}


void WcsphSimulation::SetWcsphParameters( double speedOfSound, double gamma )
{
	wcSpeedOfSound = speedOfSound;
	wcGamma = gamma;
}


bool WcsphSimulation::InitSph()
{
	LogDebug("Initializing WCSPH stuff");
	
	if(!wcSpeedOfSound)
	{
		Log::Send(Log::Error, "You need to specify speed of sound inside fluid");
		return false;
	}

	if(!wcGamma)
	{
		Log::Send(Log::Error, "You need to set WCSPH gamma parameter");
		return false;
	}

	// WCSPH specific variables
	double wcConst = this->density * wcSpeedOfSound * wcSpeedOfSound / wcGamma;

	this->InitSimulationVariable("WC_SOUND_SPEED", this->ScalarDataType(), wcSpeedOfSound, true);
	this->InitSimulationVariable("WC_CONST", this->ScalarDataType(), wcConst, true);
	this->InitSimulationVariable("WC_GAMMA", this->ScalarDataType(), wcGamma, true);
	this->InitSimulationVariable("XSPH_FACTOR", this->ScalarDataType(), xsphFactor, true);
	this->InitSimulationVariable("TC_EPSILON1", this->ScalarDataType(), -tcEpsilon1, true);
	this->InitSimulationVariable("TC_EPSILON2", this->ScalarDataType(), tcEpsilon2, true);
	this->InitSimulationVariable("ALPHA_VISCOSITY", ScalarDataType(), alphaViscosity, true);
	this->InitSimulationVariable("BETA_VISCOSITY", ScalarDataType(), betaViscosity, true);

	switch(viscosityFormulation)
	{
	case ArtificialViscosity:
		program->AddBuildOption("-D VISCOSITY_FORMULATION=1");
		break;
	case LaminarViscosity:
		program->AddBuildOption("-D VISCOSITY_FORMULATION=2");
		break;
	case SubParticleScaleViscosity:
		program->AddBuildOption("-D VISCOSITY_FORMULATION=3");
		break;
	default:
		Log::Send(Log::Error, "Viscosity formulation choice is not correct.");
	}

	this->InitSimulationBuffer("ACCELERATIONS", this->VectorDataType(), this->deviceParticleCount); // aux variable accelerations
	this->InitSimulationBuffer("XSPH_VELOCITIES", this->VectorDataType(), this->deviceParticleCount); // xsph corrected velocities
	this->InitSimulationBuffer("XSPH_SORTED", this->VectorDataType(), this->deviceParticleCount);
	this->InitSimulationBuffer("PODS", this->ScalarDataType(), this->deviceParticleCount); // aux variable - pressure over density squared
	this->InitSimulationBuffer("DENSITY_ROC", this->ScalarDataType(), this->deviceParticleCount); // aux variable
    this->InitSimulationBuffer("VELOCITIES_TMP", this->VectorDataType(), this->deviceParticleCount); // aux variable old velocities
    this->InitSimulationBuffer("POSITIONS_TMP", this->VectorDataType(), this->deviceParticleCount); // aux variable old positions
	this->InitSimulationBuffer("DENSITY_TMP", this->ScalarDataType(), this->deviceParticleCount); // aux variable old densities

   	// program build options
	program->AddBuildOption("-D WCSPH");

	if (initDensityFromPressure) 
	{
		 // Todo need to generalize to all EOS
         this->LoadSubprogram("eosInv", "wcsph/tait_eos_inv.cl");	
	}

	if (initMassFromDensity) 
	{
         this->LoadSubprogram("set masses", "wcsph/set_masses.cl");	
	}

    this->LoadSubprogram("acceleration", "wcsph/acceleration.cl");

	switch(densityReinitMethod)
	{
  	case None:
         break;
	case ShepardFilter: 
         this->LoadSubprogram("density reinit", "wcsph/shepard_filter.cl");
		 break;
	case MovingLeastSquares: 
		 program->ConnectSemantic("MLS_1", program->Variable("VELOCITIES_TMP"));
         program->ConnectSemantic("MLS_2", program->Variable("POSITIONS_TMP"));
         program->ConnectSemantic("MLS_3", program->Variable("ACCELERATIONS"));
         this->LoadSubprogram("density reinit pre", "wcsph/MLS_pre.cl");
         this->LoadSubprogram("density reinit post", "wcsph/MLS_post.cl");
		 break;
	default: Log::Send(Log::Error, "Density reinitialization method choice is not correct.");
	}

	this->LoadSubprogram("wcsph init", "wcsph/init.cl");
    this->LoadSubprogram("continuity", "wcsph/continuity.cl");
	this->LoadSubprogram("cfl", "wcsph/cfl.cl");
	this->LoadSubprogram("eos", "wcsph/tait_eos.cl");


	switch(this->integratorType)
	{
	case Euler:
		this->AddIntegrationStep("integrators/wcsph_euler.cl");
		break;
	case PredictorCorrector:
		this->AddIntegrationStep("integrators/wcsph_predictor.cl");
		this->AddIntegrationStep("integrators/wcsph_corrector.cl");
		break;
	case RungeKutta4:
    	 this->AddIntegrationStep("integrators/wcsph_rkstep41.cl");
		 this->AddIntegrationStep("integrators/wcsph_rkstep42.cl");
    	 this->AddIntegrationStep("integrators/wcsph_rkstep43.cl");
		 this->AddIntegrationStep("integrators/wcsph_rkstep44.cl");
		 this->InitSimulationBuffer("XSPH_VELOCITIES_RK", this->VectorDataType(), this->deviceParticleCount);
	     this->InitSimulationBuffer("ACCELERATIONS_RK", this->VectorDataType(), this->deviceParticleCount);
		 this->InitSimulationBuffer("DENSITY_ROC_RK", this->ScalarDataType(), this->deviceParticleCount);
		 break;
	default: Log::Send(Log::Error, "Integrator type choice is not correct.");
	}
	return true;
}

bool WcsphSimulation::PostInitSph()
{
	if (initDensityFromPressure) 
	{
		 // Todo need to generalize to all EOS
         this->EnqueueSubprogram("eosInv");	
	}

	if (initMassFromDensity) 
	{
         this->EnqueueSubprogram("set masses");	
	}

    if(!program->Finish())
	{
		Log::Send(Log::Error, "Post initialization operation failed.");
		return false;
	}

	return true;
}

void WcsphSimulation::CalculateDerivatives()
{
	// calculate particle accelerations
	this->EnqueueSubprogram("acceleration");

	// calculate density rate of change with continuity eq.
	this->EnqueueSubprogram("continuity");
}


bool WcsphSimulation::RunSph()
{
	// density reinit this step needed
	bool doDensityReinit = densityReinitMethod != None && ((this->timeStepCount + 1) % densityReinitFrequency) == 0;

	// copy buffers since we used XSPH_VELOCITIES as a temp buffer
	program->Buffer("VELOCITIES_TMP")->CopyFrom(program->Buffer("VELOCITIES"), false);
    program->Buffer("POSITIONS_TMP")->CopyFrom(program->Buffer("POSITIONS"), false);
    program->Buffer("DENSITY_TMP")->CopyFrom(program->Buffer("DENSITIES"), false);

	for(unsigned int i=0; i<this->IntegrationSteps(); i++)
	{
		CalculateDerivatives();

		// Update particle position, velocities and densities at current integration step
		this->EnqueueIntegrationStep(i);

		// update grid if specified
		if(this->integratorGridRefresh && i < this->IntegrationSteps()-1)
			if(!this->RunGrid())
				return false;

		// Update particle pressure by Equation of State
		if(!doDensityReinit || i < this->IntegrationSteps()-1)
			this->EnqueueSubprogram("eos");
	}

	// reinit grid with new particle positions
	if(!this->RunGrid())
		return false;

	// Density reinitialization every densityReinitFrequency steps
	if (doDensityReinit) 
	{
		if(densityReinitMethod == ShepardFilter) // 0th order
			this->EnqueueSubprogram("density reinit");
		else // MLS 1st order
		{
			this->EnqueueSubprogram("density reinit pre");
			this->EnqueueSubprogram("density reinit post");
		}
		this->EnqueueSubprogram("eos");
	}

	return true;
}

double WcsphSimulation::SuggestTimeStep()
{
	// init it to extra high value
	program->Buffer("NEXT_TIME_STEP")->SetScalar(100000.0);
	
	// find minimum required next time step
	this->EnqueueSubprogram("cfl");
	program->Finish();
	
	// read the result
	return program->Buffer("NEXT_TIME_STEP")->GetScalar();
}
 

bool WcsphSimulation::UploadModifiedBuffers()
{
	bool success = Simulation::UploadModifiedBuffers();
	if(!initedWcsphStuff)
	{
		success &= this->EnqueueSubprogram("wcsph init", Utils::NearestMultiple(this->ParticleCount(), 1024));
		success &= program->Finish();
		if(success)
			initedWcsphStuff = true;
	}
	return success;
}

void WcsphSimulation::SetDensityReinitMethod( DensityReinitMethods method )
{
	if(this->dimensions == 3 && method == MovingLeastSquares)
	{
		Log::Send(Log::Warning, "Ignoring 'Moving least squares' density reinit setting, not yet implemented for 3D.");
		return;
	}

	densityReinitMethod = method;
}

void WcsphSimulation::SetViscosityFormulationType(ViscosityFormulationType type)
{
	viscosityFormulation = type;
}

void WcsphSimulation::SetArtificialViscosity(double alpha, double beta)
{
	alphaViscosity = alpha;
	betaViscosity = beta;
}

void WcsphSimulation::SetIntegratorType( IntegratorType type )
{
	integratorType = type;
}

void WcsphSimulation::SetIntegratorGridRefreshing( bool enable )
{
	integratorGridRefresh = enable;
}

bool WcsphSimulation::AddIntegrationStep( const std::string& subprogram )
{
	integrationStepSubprograms.push_back(subprogram);
	return LoadSubprogram(subprogram, subprogram);
}

bool WcsphSimulation::EnqueueIntegrationStep( unsigned int i )
{
	return EnqueueSubprogram(integrationStepSubprograms[i]);
}
