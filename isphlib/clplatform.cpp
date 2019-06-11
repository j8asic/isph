#include "isph.h"
using namespace isph;

CLPlatform::CLPlatform(cl_platform_id platform)
	: id(platform)
	, deviceCount(0)
	, devices(NULL)
{

}

CLPlatform::~CLPlatform()
{
	for (unsigned int i=0; i<deviceCount; i++)
		delete devices[i];
}

CLDevice* CLPlatform::BestDevice(cl_device_type type)
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

std::vector<CLDevice*> CLPlatform::Devices(cl_device_type type)
{
	std::vector<CLDevice*> v;
	for (unsigned int i=0; i<deviceCount; i++)
		if(devices[i]->Type() == type || type == CL_DEVICE_TYPE_ALL)
			v.push_back(devices[i]);
	return v;
}
