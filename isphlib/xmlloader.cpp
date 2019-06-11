#include "isph.h"
#include <clocale>
#include <cctype>
#include <algorithm>

using namespace isph;
using namespace pugi;
using namespace std;


XmlLoader::XmlLoader()
	: Loader()
{

}

XmlLoader::~XmlLoader()
{

}

bool IsStringTrue(const std::string& s)
{
	if(!s.length())
		return false;
	return (s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y');
}

std::string XmlLoader::ParseString(const pugi::xml_node& node, const std::string& def)
{
	if(!node)
		return def;
	xml_attribute a = node.attribute("value");

	if(a.empty())
		return std::string(node.child_value());
	else
		return std::string(a.value());
}

bool XmlLoader::ParseBoolean(const pugi::xml_node& node, bool def)
{
	if(!node)
		return def;
	return IsStringTrue(ParseString(node));
}

int XmlLoader::ParseInt(const pugi::xml_node& node, int def)
{
	if(!node)
		return def;
	return atoi(ParseString(node).c_str());
}

double XmlLoader::ParseScalar(const pugi::xml_node& node, double def)
{
	if(!node)
		return def;
	return atof(ParseString(node).c_str());
}

Vec<3,double> XmlLoader::ParseVector(const pugi::xml_node& node)
{
	Vec<3,double> v;
	if(!node)
		return v;
	std::string str = ParseString(node);
	str.append(1, ' ');
	unsigned int start=0, count=0;
	for(unsigned int i=0; i<str.size(); i++)
	{
		if(isspace(str[i]) || str[i]==';')
		{
			if(start < i)
			{
				v[count++] = atof(str.substr(start, i-start).c_str());
				if(count == 3)
					break;
			}
			start = i+1;
		}
	}
	return v;
}

std::vector<std::string> XmlLoader::ParseStringArray(const pugi::xml_node& node)
{
	std::vector<std::string> vec;
	if(!node)
		return vec;
	std::string str = ParseString(node);
	str.append(1, ';');
	unsigned int start=0;
	for(unsigned int i=0; i<str.size(); i++)
	{
	    // Skip newlines
	    if (str[i]=='\r' || str[i]=='\n') str[i]=' ';
		if(str[i]==';')
		{
			if(start < i)
			{
				vec.push_back(str.substr(start, i-start));
				if(vec.size() == 3)
					break;
			}
			start = i+1;
		}
	}
	return vec;
}


Simulation* XmlLoader::Read()
{
	Log::Send(Log::Info, "Reading XML file: " + path);

	setlocale(LC_ALL, "C");

	xml_document xmlDoc;
	xml_parse_result result = xmlDoc.load_file(path.c_str());

	if(!result)
	{
		Log::Send(Log::Error, "Error while parsing file: " + path + " ["+ result.description() +"]");
		return NULL;
	}

	xml_node xmlSim = xmlDoc.child("simulation");
	if(!xmlSim)
	{
		Log::Send(Log::Error, "XML file is missing 'simulation' node.");
		return NULL;
	}

	// get basic info for creating simulation

	xml_node xmlDim = xmlSim.child("dimensions");
	unsigned int dimensions = xmlDim ? ParseInt(xmlDim) : 2;

	xml_node xmlAsync = xmlSim.child("async_output");
	bool asyncOutput = xmlAsync.attribute("enable").as_bool() || ParseBoolean(xmlAsync);

	xml_node xmlPrecision = xmlSim.child("precision");
	VariableDataType scalarType;
	if(ParseString(xmlPrecision).find("double") != std::string::npos)
		scalarType = DoubleType;
	else
		scalarType = FloatType;


	// solver
	xml_node xmlSolver = xmlSim.child("solver");
	std::string solverType(xmlSolver.attribute("type").value());

	Simulation* sim = NULL;

	// create specific solver and set its specific parameters

	if(solverType == "wcsph")
	{
		WcsphSimulation* wcsphSim = new WcsphSimulation(dimensions, scalarType);
		sim = wcsphSim;

		// sound speed
		xml_node xmlSoundSpeed = xmlSolver.child("sound_speed");
		double soundSpeed = xmlSoundSpeed ? ParseScalar(xmlSoundSpeed) : 100;
		wcsphSim->SetWcsphParameters(soundSpeed, 7.0);

		// XSPH
		xml_node xmlXsph = xmlSolver.child("xsph");
		double xsph = xmlXsph ? ParseScalar(xmlXsph) : 0.5;
		wcsphSim->SetXsphFactor(xsph);

		// integrators
        xml_node xmlIntegratorType =  xmlSolver.child("integrator");
        if (xmlIntegratorType)
		{
			std::string integratorType(xmlIntegratorType.attribute("type").value());

			if (integratorType.find("predict") != std::string::npos || integratorType == "pc") // allow partial find
				wcsphSim->SetIntegratorType(PredictorCorrector);
    		else if (integratorType.find("rk4") != std::string::npos || integratorType == "rk")
				wcsphSim->SetIntegratorType(RungeKutta4);
			else if (integratorType == "euler")
				wcsphSim->SetIntegratorType(Euler);
		    else
		    {
				Log::Send(Log::Error, "Integrator type not supported.");
				return NULL;
		    }

			if(xmlIntegratorType.child("grid_refresh"))
				wcsphSim->SetIntegratorGridRefreshing(ParseBoolean(xmlIntegratorType.child("grid_refresh")));
		}

		// viscosity
		xml_node xmlViscosityFormulation = xmlSolver.child("viscosity_formulation");
		if (xmlViscosityFormulation)
		{
			std::string formulationType(xmlViscosityFormulation.attribute("type").value());
			if (formulationType.find("artificial") != std::string::npos) 
			{
				wcsphSim->SetViscosityFormulationType(ArtificialViscosity);
				wcsphSim->SetArtificialViscosity(ParseScalar(xmlViscosityFormulation.child("alpha")), ParseScalar(xmlViscosityFormulation.child("beta")));
			} 
			else if (formulationType.find("laminar") != std::string::npos)
			{
				wcsphSim->SetViscosityFormulationType(LaminarViscosity);
				wcsphSim->SetArtificialViscosity(-1.0, 0.0);
			}
			else
			{
				Log::Send(Log::Error, "Viscosity formulation type not supported.");
				return NULL;
			}
		}

		// density reinit
		xml_node xmlDensityReinit = xmlSolver.child("density_reinitialization");
		if (xmlDensityReinit) 
		{
		  std::string reinitType(xmlDensityReinit.attribute("type").value());
		  if (reinitType.find("mls") != std::string::npos) 
		  {
			  wcsphSim->SetDensityReinitMethod(MovingLeastSquares);
			  int frequency = xmlDensityReinit ? ParseInt(xmlDensityReinit ) : -1;
			  wcsphSim->SetDensityReinitFrequency(frequency);
		  } 
		  else if (reinitType.find("shepard") != std::string::npos)
		  {
			  wcsphSim->SetDensityReinitMethod(ShepardFilter);
			  int frequency = xmlDensityReinit ? ParseInt(xmlDensityReinit ) : -1;
			  wcsphSim->SetDensityReinitFrequency(frequency);
		  }
		  else if (reinitType.find("none") != std::string::npos)
		  {
        	  wcsphSim->SetDensityReinitMethod(None);
			  wcsphSim->SetDensityReinitFrequency(-1);
		  }
		  else
		  {
			  Log::Send(Log::Error, "Density reinitialization method not supported.");
			  return NULL;
		  }
		}
		else 
		{
			wcsphSim->SetDensityReinitMethod(None);
			wcsphSim->SetDensityReinitFrequency(-1);
		}

		// tensile correction
        xml_node xmlTensileCorrection = xmlSolver.child("tensile_correction");
		if (xmlTensileCorrection) 
		{
			if (xmlTensileCorrection.child("epsilon1"))
			{
				wcsphSim->SetTCEpsilon1(ParseScalar(xmlTensileCorrection.child("epsilon1")));
			}
			if (xmlTensileCorrection.child("epsilon2"))
			{
				wcsphSim->SetTCEpsilon2(ParseScalar(xmlTensileCorrection.child("epsilon2")));
			}
		}

		xml_node xmlDensityInitFromPressure= xmlSolver.child("init_density_from_pressure");
		bool initDensityFromPressure = xmlDensityInitFromPressure ? ParseBoolean(xmlDensityInitFromPressure) : false;
		wcsphSim->SetInitDensityFromPressure(initDensityFromPressure);

		xml_node xmlMassInitFromDensity= xmlSolver.child("init_mass_from_density");
		bool initMassFromDensity = xmlMassInitFromDensity ? ParseBoolean(xmlMassInitFromDensity) : false;
		wcsphSim->SetInitMassFromDensity(initMassFromDensity);

	}
	else if(solverType == "isph")
	{
		IsphSimulation* isphSim = new IsphSimulation(dimensions, scalarType);
		sim = isphSim;

		// free surface factor
		xml_node xmlFreeSurfaceFactor = xmlSolver.child("free_surface");
		if(xmlFreeSurfaceFactor)
		{
			isphSim->SetFreeSurfaceFactor(ParseScalar(xmlFreeSurfaceFactor));
			std::string dbc = xmlFreeSurfaceFactor.attribute("dirichlet").as_string("weak");
			isphSim->SetStrongDirichletBC(dbc.at(0)=='s' || dbc.at(0)=='S' ? true : false);
		}

		// particle shifting
		xml_node xmlShift = xmlSolver.child("particle_shifting");
		if(xmlShift)
			isphSim->SetParticleShifting(xmlShift.attribute("enable").as_bool(), xmlShift.attribute("factor").as_double(), xmlShift.attribute("frequency").as_uint(1));

		// projection
		xml_node xmlProj = xmlSolver.child("projection");
		if(xmlProj)
		{
			unsigned int order = xmlProj.attribute("order").empty() ? 2 : xmlProj.attribute("order").as_uint();

			std::string projName(xmlProj.attribute("type").value());
			if(projName.find("non") != std::string::npos)
				isphSim->SetProjection(NonIncremental, 1);
			else if(projName == "standard")
				isphSim->SetProjection(Standard, order);
			else if(projName == "rotational")
				isphSim->SetProjection(Rotational, order);
			else
				Log::Send(Log::Warning, "Projection method '" + projName + "' not supported, using default.");
		}

		// solver
		xml_node xmlPPESolver = xmlSolver.child("ppe_solver");
		if (xmlPPESolver)
		{
			std::string solverName(xmlPPESolver.attribute("type").value());
			if(solverName == "cg")
				isphSim->SetSolver(CG);
			else if(solverName == "bicgstab")
				isphSim->SetSolver(BiCGSTAB);
			else
				Log::Send(Log::Warning, "PPE solver method '" + solverName + "' not supported, using default.");

			// solver max iterations
			if(!xmlPPESolver.attribute("max_iterations").empty())
				isphSim->SetMaxSolverIterations(xmlPPESolver.attribute("max_iterations").as_uint());

			xml_node xmlMaxSolverIterations = xmlPPESolver.child("max_iterations");
			if (xmlMaxSolverIterations)
				isphSim->SetMaxSolverIterations(ParseInt(xmlMaxSolverIterations));

			// solver tolerance
			if(!xmlPPESolver.attribute("tolerance").empty())
				isphSim->SetSolverTolerance(xmlPPESolver.attribute("tolerance").as_double());

			xml_node xmlSolvingTolerance = xmlPPESolver.child("tolerance");
			if (xmlSolvingTolerance)
				isphSim->SetSolverTolerance(ParseScalar(xmlSolvingTolerance));
		}
	}
	else
	{
		Log::Send(Log::Error, "Solver type not supported.");
		return NULL;
	}

	sim->SetName(ParseString(xmlSim.child("name")));
	sim->SetDescription(ParseString(xmlSim.child("description")));

	// set running info
	xml_node xmlTimeStep = xmlSim.child("time_step");
	sim->SetRunTime(ParseScalar(xmlSim.child("run_time")), ParseScalar(xmlTimeStep), xmlTimeStep.attribute("auto_factor").as_double(1.0));

	// export management
	sim->SetAsyncExport(asyncOutput);

	for (xml_node xmlExport = xmlSim.child("export"); xmlExport; xmlExport = xmlExport.next_sibling("export"))
	{
		Writer* writer;
		
		std::string exporterType;
		if(!xmlExport.attribute("format").empty())
			exporterType = xmlExport.attribute("format").value();
		else
			exporterType = xmlExport.attribute("type").value();

		// VTK legacy writer
		if(exporterType == "vtk")
		{
			VtkWriter *vtk = new VtkWriter(sim);
			writer = vtk;

			if(!xmlExport.attribute("binary").empty())
				vtk->SetBinaryOutput(xmlExport.attribute("binary").as_bool());
		}
		// Comma separated file format
		else if(exporterType == "csv")
		{
			CsvWriter *csv = new CsvWriter(sim);
			writer = csv;
		}
		// Luca probes writer
		else if(exporterType.find("probe") != std::string::npos)
		{
			ProbeManager* probesManager = new ProbeManager(sim);
			writer = probesManager;

			for (xml_node xmlAttr = xmlExport.child("location"); xmlAttr; xmlAttr = xmlAttr.next_sibling("location"))
				probesManager->AddProbe(ParseVector(xmlAttr));
		}
		else if(exporterType.find("force") != std::string::npos)
		{
			BodyForceWriter *force = new BodyForceWriter(sim);
			writer = force;

			for (xml_node xmlAttr = xmlExport.child("body"); xmlAttr; xmlAttr = xmlAttr.next_sibling("body"))
				force->AddBody(ParseString(xmlAttr));
		}
		else
		{
			Log::Send(Log::Error, "Exporter type not supported.");
			return NULL;
		}

		// time step
		if(xmlExport.child("time_step"))
			writer->SetExportTimeStep(ParseScalar(xmlExport.child("time_step")));
		else if(!xmlExport.attribute("time_step").empty())
			writer->SetExportTimeStep(xmlExport.attribute("time_step").as_double());

		// list of times
		for (xml_node xmlAttr = xmlExport.child("time"); xmlAttr; xmlAttr = xmlAttr.next_sibling("time"))
			writer->AddExportTime(ParseScalar(xmlAttr));

		// extension
		if(!xmlExport.attribute("extension").empty())
			writer->SetFileExtension(xmlExport.attribute("extension").value());

		// variables
		for (xml_node xmlAttr = xmlExport.child("variable"); xmlAttr; xmlAttr = xmlAttr.next_sibling("variable"))
			writer->AddAttribute(ParseString(xmlAttr));

	}

	// choose devices simulation will run on

	xml_node xmlDevices = xmlSim.child("devices");

	std::string strDevType = xmlDevices.attribute("type").value();
	cl_device_type devType = CL_DEVICE_TYPE_ALL;
	if(strDevType == "cpu")
		devType = CL_DEVICE_TYPE_CPU;
	else if(strDevType == "gpu")
		devType = CL_DEVICE_TYPE_GPU;

	std::string devName = xmlDevices.attribute("name").value();

	std::vector<CLDevice*> devs = CLSystem::Instance()->FilterDevices(devType, devName);

	if(devs.empty())
	{
		Log::Send(Log::Error, "Couldn't find any devices with specified filters.");
		return NULL;
	}

	std::string strDevCount = xmlDevices.attribute("count").value();
	int intDevCount = xmlDevices.attribute("count").as_int(-1);

	if(!strDevCount.length() || strDevCount == "all" || !intDevCount)
		sim->SetDevices(new CLLink(devs));
	else if(strDevCount == "best")
		sim->SetDevices(new CLLink(CLSystem::Instance()->BestDevice(devs)));
	else if(intDevCount > 0)
	{
      if(intDevCount < (int)devs.size())
			devs.resize(intDevCount, NULL);
		sim->SetDevices(new CLLink(devs));
	}
	else // by default take the first device in list
		sim->SetDevices(new CLLink(devs[0]));


	// boundary options

	xml_node xmlBounds = xmlSim.child("boundary");
	if(!xmlBounds)
		xmlBounds = xmlSim.child("bounds");
	xml_node xmlBoundMin = xmlBounds.child("min");
	xml_node xmlBoundMax = xmlBounds.child("max");
	if(!xmlSim || !xmlBoundMin || !xmlBoundMax)
	{
		Log::Send(Log::Error, "XML file is missing boundary information.");
		return NULL;
	}
	sim->SetBoundaries(ParseVector(xmlBoundMin), ParseVector(xmlBoundMax));

	// particle spacing

	xml_node xmlSpacing = xmlSim.child("particle_spacing");
	if(!xmlSpacing)
	{
		Log::Send(Log::Error, "XML file is missing particle spacing information.");
		return NULL;
	}
	
	sim->SetParticleSpacing(ParseScalar(xmlSpacing));

	// gravity
	sim->SetGravity(ParseVector(xmlSim.child("gravity")));

	// smoothing kernel

	xml_node xmlKernel = xmlSim.child("smoothing_kernel");
	if(xmlKernel)
	{
		double h = ParseScalar(xmlKernel);
		std::string kernelType = xmlKernel.attribute("type").value();
		bool kernelCorr = xmlKernel.attribute("correction").as_bool();

		if(kernelType.find("cubic") != std::string::npos)
			sim->SetSmoothingKernel(CubicSplineKernel, h, kernelCorr);
		else if(kernelType.find("quintic") != std::string::npos)
			sim->SetSmoothingKernel(QuinticSplineKernel, h, kernelCorr);
		else if(kernelType.find("quad") != std::string::npos)
			sim->SetSmoothingKernel(QuadraticKernel, h, kernelCorr);
		else if(kernelType.find("gauss") != std::string::npos)
			sim->SetSmoothingKernel(GaussKernel, h, kernelCorr);
		else if(kernelType.find("wendland") != std::string::npos)
			sim->SetSmoothingKernel(WendlandKernel, h, kernelCorr);
		else
		{
			sim->SetSmoothingKernel(CubicSplineKernel, h, kernelCorr);
			Log::Send(Log::Warning, "Smoothing kernel '" + kernelType + "' not supported. Cubic spline kernel will be used.");
		}
	}

	// fluid(s)

	for (xml_node xmlFluid = xmlSim.child("fluid"); xmlFluid; xmlFluid = xmlFluid.next_sibling("fluid"))
	{
		/// \todo Read multiple fluids
		//std::string name = xmlFluid.attribute("name").value();
		//create new fluid with name

		if(xmlFluid.child("density"))
			sim->SetDensity(ParseScalar(xmlFluid.child("density")));
		else if(!xmlFluid.attribute("density").empty())
			sim->SetDensity(xmlFluid.attribute("density").as_double());

		if(xmlFluid.child("viscosity"))
			sim->SetViscosity(ParseScalar(xmlFluid.child("viscosity")));
		else if(!xmlFluid.attribute("viscosity").empty())
			sim->SetViscosity(xmlFluid.attribute("viscosity").as_double());
	}

	// scene setup

	xml_node xmlSetup = xmlSim.child("setup");
	if(!xmlSetup)
	{
		Log::Send(Log::Error, "XML file is missing object setup information.");
		return NULL;
	}

	for (xml_node xmlObj = xmlSetup.first_child(); xmlObj; xmlObj = xmlObj.next_sibling())
	{
		std::string objTypeStr = xmlObj.name();
		std::string name = xmlObj.attribute("name").value();
		
		std::string typeStr = xmlObj.attribute("type").value();
		ParticleType type;

		if(typeStr.find("boundary") != std::string::npos)
			type = BoundaryParticle;
		else
			type = FluidParticle;

		if(objTypeStr == "bucket_fill")
		{
			sim->SetBucketFillLocation(ParseVector(xmlObj.child("location")));
		}
		else if (objTypeStr == "line")
		{
			std::string n = xmlObj.attribute("normal").value(); n += "l";
			geo::Line *l = new geo::Line(sim, type, name);
			l->Define(ParseVector(xmlObj.child("start")), ParseVector(xmlObj.child("end")), (n[0] == 'L' || n[0] == 'l') ? true : false);
		}
		else if (objTypeStr == "circle")
		{
			geo::Circle *s = new geo::Circle(sim, type, name);
			std::string n = xmlObj.attribute("normal").value(); n += "o";
			s->Define(ParseVector(xmlObj.child("center")), ParseScalar(xmlObj.child("radius")), (n[0] == 'o' || n[0] == 'U') ? true : false);
		}
		else if (objTypeStr == "box")
		{
			geo::Box *b = new geo::Box(sim, type, name);
			if(xmlObj.child("faces"))
			{
				unsigned char faces = 0x00;
				if(ParseString(xmlObj.child("faces")).find("E") != std::string::npos) faces |= geo::Box::E;
				if(ParseString(xmlObj.child("faces")).find("W") != std::string::npos) faces |= geo::Box::W;
				if(ParseString(xmlObj.child("faces")).find("N") != std::string::npos) faces |= geo::Box::N;
				if(ParseString(xmlObj.child("faces")).find("S") != std::string::npos) faces |= geo::Box::S;
				if(ParseString(xmlObj.child("faces")).find("U") != std::string::npos) faces |= geo::Box::U;
				if(ParseString(xmlObj.child("faces")).find("D") != std::string::npos) faces |= geo::Box::D;
    			b->Define(ParseVector(xmlObj.child("origin")), ParseVector(xmlObj.child("east")),ParseVector(xmlObj.child("north")),ParseVector(xmlObj.child("up")),faces);
			}
			else
    			b->Define(ParseVector(xmlObj.child("origin")), ParseVector(xmlObj.child("east")),ParseVector(xmlObj.child("north")),ParseVector(xmlObj.child("up")));

             
			//	if(ParseString(xmlObj.child("filled")).find("true") != std::string::npos)
			//		b->Fill();
		}
		else if (objTypeStr == "sphere")
		{
			geo::Sphere *s = new geo::Sphere(sim, type, name);
			s->Define(ParseVector(xmlObj.child("center")), ParseScalar(xmlObj.child("radius")));
			if(xmlObj.child("filled"))
				if(ParseBoolean(xmlObj.child("filled")))
					s->Fill();
		}
		else if (objTypeStr == "patch")
		{
			geo::Patch *s = new geo::Patch(sim, type, name);
			int rows = (std::max)(1, ParseInt(xmlObj.child("thickness")));
			s->Define(ParseVector(xmlObj.child("start")), ParseVector(xmlObj.child("end1")), ParseVector(xmlObj.child("end2")), rows);
		}
		else
		{
			Log::Send(Log::Error, "Geometry type '"+objTypeStr+"' not supported.");
			return NULL;
		}


		xml_node xmlInitialization = xmlObj.child("initialization");
		if(xmlInitialization)
		{
			for (xml_node xmlVariable = xmlInitialization.first_child(); xmlVariable ; xmlVariable = xmlVariable.next_sibling())
			{
				std::string varName = xmlVariable.attribute("name").value();
				sim->GetGeometry(name)->SetInitExpression(varName, ParseStringArray(xmlVariable)); 
			}
		}

		xml_node xmlMovement = xmlObj.child("movement");
		if(xmlMovement)
		{
			sim->GetGeometry(name)->SetMovementExpression(ParseStringArray(xmlMovement.child("position")),
				ParseStringArray(xmlMovement.child("velocity")),
				xmlMovement.attribute("start").as_double(),
				xmlMovement.attribute("end").as_double());
		}
	}

	return sim;
}
