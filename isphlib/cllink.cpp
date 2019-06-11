#include "isph.h"
using namespace isph;

CLLink::CLLink(std::vector<CLDevice*>& devices)
	: context(NULL)
	, deviceCount(0)
	, devices(NULL)
	, queues(NULL)
   , deviceFactors(NULL)
{
	Initialize(&devices.front(), (unsigned int)devices.size());
}

CLLink::CLLink(CLDevice* device) :
	context(NULL),
	deviceCount(0),
	devices(NULL),
   queues(NULL),
   deviceFactors(NULL)
{
	if(device)
		Initialize(&device, 1);
}

CLLink::CLLink( CLPlatform* platform ) :
	context(NULL),
	deviceCount(0),
	devices(NULL),
   queues(NULL),
   deviceFactors(NULL)
{
	if(!platform)
		platform = CLSystem::Instance()->FirstPlatform();

	if(!platform)
	{
		Log::Send(Log::Error, "No platform to connect to.");
		return;
	}

	std::vector<CLDevice*> devs = platform->Devices();
	Initialize(&devs.front(), (unsigned int)devs.size());
}

bool CLLink::Initialize(CLDevice** devices, unsigned int size)
{
	LogDebug("Connecting to OpenCL enabled device(s)");

	if(!size || !devices)
	{
		Log::Send(Log::Error, "No devices to connect to.");
		return false;
	}

	deviceCount = size;
	this->devices = new CLDevice*[size];
	deviceFactors = new double[size];
	queues = new cl_command_queue[size];

	cl_device_id *devicesIDs = new cl_device_id[size];

	cl_uint totalPerformance = 0; 

	unsigned int i;
	for (i=0; i<size; i++)
	{
		this->devices[i] = devices[i];
		devicesIDs[i] = devices[i]->ID();
		totalPerformance += devices[i]->PerformanceIndex();
	}

	cl_int status;
	context = clCreateContext(NULL, (cl_uint)size, devicesIDs, NULL, NULL, &status);
	if(status)
	{
		Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		return false;
	}

	for (i=0; i<size; i++)
	{
		Log::Send(Log::Info, "Linking to device: " + devices[i]->Name());

		if(CLSystem::Instance()->Profiling())
			queues[i] = clCreateCommandQueue(context, devices[i]->ID(), CL_QUEUE_PROFILING_ENABLE, &status);
		else
			queues[i] = clCreateCommandQueue(context, devices[i]->ID(), 0, &status);

		deviceFactors[i] = (double)devices[i]->PerformanceIndex() / totalPerformance;
	}

	delete [] devicesIDs;
	return true;
}

CLLink::~CLLink()
{
	cl_int status;

	if(deviceCount)
	{
		for (unsigned int i=0; i<deviceCount; i++)
		{
			status = clReleaseCommandQueue(queues[i]);
		}
		delete [] devices;
		delete [] deviceFactors;
		delete [] queues;
	}

	if(context)
	{
		status = clReleaseContext(context);
	}
}

bool CLLink::BuildProgram(cl_program& program, const std::string& buildOptions)
{
	// build program on each device
	for(unsigned int i=0; i<deviceCount; i++)
	{
		std::string finalBuildOptions = buildOptions;

		if(devices[i]->IsGPU())
		{
			finalBuildOptions.append(" -D GPU");
			if(Platform()->Name().find("ATI") != std::string::npos)
				finalBuildOptions.append(" -D ATI");
			else
				finalBuildOptions.append(" -D NVIDIA");
		}
		else if(devices[i]->IsCPU())
		{
			finalBuildOptions.append(" -D CPU");
		}

		cl_int status = clBuildProgram(program, 1, &devices[i]->ID(), finalBuildOptions.c_str(), NULL, NULL);
		

      size_t log_size;
      clGetProgramBuildInfo(program, devices[i]->ID(), CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
      char *buf = new char[log_size];
      cl_int status2 = clGetProgramBuildInfo(program, devices[i]->ID(), CL_PROGRAM_BUILD_LOG, log_size, buf, NULL);
		if(status2)
         Log::Send(Log::Error, "Error while fetching OpenCL compile log");
		else if(status)
			Log::Send(Log::Error, buf);
		else
			Log::Send(Log::DebugInfo, buf);
		delete [] buf;

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	return true;
}

bool CLLink::Finish()
{
	cl_int status;
	bool success = true;

	for(unsigned int i=0; i<deviceCount; i++)
	{
		status = clFinish(queues[i]);
		if(status)
		{
			success = false;
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		}
	}

	return success;
}

unsigned int isph::CLLink::MinMaxWorgroupSize()
{
	unsigned int minValue = 100000;

	for(unsigned int i=0; i<deviceCount; i++)
	{
		if(devices[i]->MaxWorkGroupSize() < minValue)
			minValue = devices[i]->MaxWorkGroupSize();
	}

	return minValue;
}

CLDevice* CLLink::BestDevice( cl_device_type type )
{
	unsigned int maxPoints = 0;
	CLDevice* device = NULL;

	for (unsigned int i=0; i<deviceCount; i++)
		if(devices[i]->Type() == type || type == CL_DEVICE_TYPE_ALL)
			if (devices[i]->PerformanceIndex() > maxPoints)
			{
				maxPoints = devices[i]->PerformanceIndex();
				device = devices[i];
			}

	return device;
}

CLPlatform* CLLink::Platform()
{
	return devices ? devices[0]->Platform() : NULL;
}
