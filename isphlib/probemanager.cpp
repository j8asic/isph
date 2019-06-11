#include "probemanager.h"
#include "isph.h"

#include <sstream>
#include <iomanip>
#include <cfloat>

using namespace isph;

ProbeManager::ProbeManager(Simulation* simulation)
	: StdWriter(simulation)
	, initialized(false)
	, totalScalarValues(0)
	, headerWritten(false)
{
	SetFileExtension("prbs");
}


ProbeManager::~ProbeManager()
{
	delete [] locations_f;
	delete [] times_f;
} 


bool ProbeManager::Prepare()
{
	if(!StdWriter::Prepare())
		return false;
	InitKernels();
	WriteHeader();
	return true;
}


bool ProbeManager::Finish()
{
	if (BufferFull())
		WriteBuffer();
	return StdWriter::Finish();
}


void ProbeManager::AddProbe(Vec<3,double> location)
{
	if (initialized) return;
	locationList.push_back( location );
	singleBufferSize = totalScalarValues * (unsigned int)locationList.size();
}


void ProbeManager::AddProbesString(Vec<3,double> start, Vec<3,double> end, double spacing)
{
	if (initialized) return;
    Vec<3,double> diff = end - start;
	double length = diff.Length();
	double cumulate = 0;
	while (cumulate<length) 
	{
		Vec<3,double> location = start + cumulate * diff / length; 
		AddProbe(location);
		cumulate += spacing;
	}
}


void ProbeManager::WriteHeader()
{
	if(!attributeList.size())
		return;

	// write header
	stream << "# ISPH Probes file Version 1.0" << std::endl;
    stream << "# Monitored attributes " << std::endl;
    stream << "n = " << attributeList.size() << std::endl;

	// write data
	for (std::list<CLGlobalBuffer*>::const_iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
        stream << (*iter)->Semantic();
		if ((*iter)->DataType() == sim->ScalarDataType()) 
			stream << " components: 1" << std::endl; 
		else
			stream << " components: " << sim->Dimensions() << std::endl; 
	}
   
	stream << "# Monitored locations " << std::endl;
    stream << "m = " << locationList.size() << std::endl;
    
	std::vector< Vec<3,double> >::iterator it;
	for (it = locationList.begin(); it != locationList.end(); ++it)
	{
		stream << (*it).x << " " << (*it).y << " " << (*it).z << std::endl;
	}

	stream << "# Sampling frequency" << std::endl;
	stream << "t = " << this->ExportTimeStep() << " s" << std::endl;
    // Write format decription lines
	// Line 1 locations
    stream << "#" << std::left << std::endl;
	// Line 2 time, attributes
	stream << "#" << std::left << std::setw(15) << "time";

	for (std::list<CLGlobalBuffer*>::const_iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
		if ((*iter)->DataType() == sim->ScalarDataType()) 
		{
			stream << "|" << std::left << std::setw(16*(locationList.size())) << (*iter)->Semantic();
		}
		else
		{
			std::string tmp;
			tmp = (*iter)->Semantic() + "(x)";
	        stream <<  "|" << std::left << std::setw(16*(locationList.size())) << tmp;
			tmp = (*iter)->Semantic() + "(y)";
	        stream <<  "|" << std::left << std::setw(16*(locationList.size())) << tmp;
		}
	}
	stream << std::endl;
}


void ProbeManager::WriteBuffer()
{
	if(!attributeList.size())
		return;

   CLGlobalBuffer* probesBuffer = sim->program->Buffer("PROBES_BUFFER");
   probesBuffer->Download();

   for (unsigned int h=0; h<recordedSteps; h++ )
   {
		if(sim->ScalarDataType() == DoubleType)
			stream << std::scientific << std::setprecision(6)<< std::left << std::setw(15) << times_d[h] << " ";
		else
			stream << std::scientific << std::setprecision(6)<< std::left << std::setw(15) << times_f[h] << " ";  

		for ( unsigned int i=0; i<totalScalarValues; i++ )
			for ( unsigned int j=0; j<locationList.size(); j++ )
			{
				stream << std::scientific << std::setprecision(6) << std::right<< std::setw(15);
				if(sim->ScalarDataType() == DoubleType)
				{
					double* data_d = (double*)probesBuffer->HostData();
					stream << data_d[h*singleBufferSize +  i*locationList.size() + j] << " ";
				}
				else
				{
					float* data_f = (float*)probesBuffer->HostData();
					stream << data_f[h*singleBufferSize +  i*locationList.size()+ j] << " ";
				}
			}
		stream << std::endl;
	}
	recordedSteps = 0;
}


void ProbeManager::InitKernels() 
{
	for (std::list<CLGlobalBuffer*>::const_iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
		CLGlobalBuffer* att = (*iter);

		// Update total number of scalar values per probe
		if (att->DataType() == sim->ScalarDataType()) 
			totalScalarValues++;
		else
			totalScalarValues += sim->Dimensions();
	}

	if(!attributeList.size())
		return;

	singleBufferSize = totalScalarValues * (unsigned int)locationList.size();

    bufferingSteps = 10;//32768 / singleBufferSize;
    bufferSize = singleBufferSize * bufferingSteps;
		
	// Allocate host buffers
	if(sim->ScalarDataType() == DoubleType)
	{
		times = new double[bufferingSteps];
		locations = new double[locationList.size()*sim->Dimensions()];
	}
	else
	{
		times = new float[bufferingSteps];
		locations = new float[locationList.size()*sim->Dimensions()];
	}

 
	// Copy locations to array
	sim->InitSimulationBuffer("PROBES_LOCATION", sim->VectorDataType(), (unsigned int)locationList.size());

	unsigned int cnt = 0;
	for (std::vector< Vec<3,double> >::iterator it = locationList.begin(); it != locationList.end(); ++it)
		sim->program->Buffer("PROBES_LOCATION")->SetVector(cnt++, *it);

	sim->program->Buffer("PROBES_LOCATION")->Upload();

	recordedValues[0] = 0;
	recordedValues[1] = 0;
    recordedSteps = 0;

	// Allocate device variables
    sim->InitSimulationBuffer("PROBES_BUFFER", sim->ScalarDataType(), bufferSize);

	sim->program->ConnectSemantic("PROBES_SCALAR", sim->program->Variable("PRESSURES"));
    sim->program->ConnectSemantic("PROBES_VECTOR", sim->program->Variable("VELOCITIES"));

	sim->InitSimulationVariable("PROBES_BUFFERING_STEPS", UintType, bufferingSteps, true);
	sim->InitSimulationVariable("PROBES_SINGLE_BUFFER_SIZE", UintType, singleBufferSize, true);
	
	cnt = (unsigned int)locationList.size();
	sim->InitSimulationVariable("PROBES_COUNT", UintType, cnt, true);
    cnt = (unsigned int)attributeList.size();
	sim->InitSimulationVariable("PROBES_ATT_COUNT", UintType, totalScalarValues, true);

	sim->InitSimulationBuffer("PROBES_RECORDED_VALUES", UintType, 2);
	sim->program->Buffer("PROBES_RECORDED_VALUES")->SetScalar(0, 0);
	sim->program->Buffer("PROBES_RECORDED_VALUES")->SetScalar(1, 0);
	sim->program->Buffer("PROBES_RECORDED_VALUES")->Upload();

	// Load kernels
   sim->LoadSubprogram("read probes scalar",
                       #include "wcsph/read_probes_scalar.cl"
                       );
   sim->LoadSubprogram("read probes vector",
                       #include "wcsph/read_probes_vector.cl"
                       );

    initialized = true;
}


void ProbeManager::PrepareData()
{
	if(!attributeList.size())
		return;

	RecordSamplingTime(sim->Time());

	// Cycle trough all required attributes
	for (std::list<CLGlobalBuffer*>::const_iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
		if ((*iter)->DataType() == sim->ScalarDataType()) 
		{
			// TODO Force execution on single workgroup do we need this?
			sim->program->ConnectSemantic("PROBES_SCALAR", (*iter));
			sim->EnqueueSubprogram("read probes scalar", GetProbesCount(), 1);
		}
		else
		{
			// Force execution on singe workgroup
			sim->program->ConnectSemantic("PROBES_VECTOR", (*iter));
			sim->EnqueueSubprogram("read probes vector", GetProbesCount(), 1);
		}
	}

	// wait for the data to be read
	sim->program->Finish();
}


void ProbeManager::WriteData() 
{
	if(!attributeList.size())
		return;

	this->UpdateStats();

	// download data and write to file if buffer full
	if (BufferFull())
		WriteBuffer();
}

void ProbeManager::RecordSamplingTime( double time )
{
	if(sim->ScalarDataType() == DoubleType)
		times_d[recordedSteps++] = time;
	else
		times_f[recordedSteps++] = (float)time;
}
