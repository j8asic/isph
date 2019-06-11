#include "isph.h"
#include <string>
#include <cmath>
#include <cstring>

using namespace isph;

CLKernelArgument::CLKernelArgument(CLProgram* program, const std::string& semantic, bool /*dummy*/)
	: CLVariable(program, semantic, ProgramConstant)
{
}

CLKernelArgument::CLKernelArgument(CLProgram* program, const std::string& semantic)
	: CLVariable(program, semantic, KernelArgument)
{
}


CLKernelArgument::~CLKernelArgument()
{
	Release();
}


bool CLKernelArgument::Allocate()
{
	Release();

	data = new char[memorySize]();
	needsUpdate = false;
	return true;
}


bool CLKernelArgument::SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int /*deviceID*/, size_t /*kernelLocalSize*/)
{
	if(!kernel)
		return false;

	if(!kernel->Kernel())
		return false;

	cl_int status = clSetKernelArg(kernel->Kernel(), (cl_uint)argID, DataTypeSize(), data);

	if(status)
	{
		Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		return false;
	}

	return true;
}


void CLKernelArgument::Release()
{
	if(data)
	{
		delete [] data;
		data = NULL;
	}
}
