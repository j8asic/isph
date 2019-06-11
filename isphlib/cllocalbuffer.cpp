#include "isph.h"
#include <string>
#include <cmath>
#include <cstring>

using namespace isph;

CLLocalBuffer::CLLocalBuffer(CLProgram *program, const std::string &semantic)
	: CLVariable(program, semantic, LocalBuffer)
{

}

CLLocalBuffer::~CLLocalBuffer()
{

}

bool CLLocalBuffer::Allocate()
{
	Release();

	if(!parentProgram)
	{
		Log::Send(Log::Error, "Cannot allocate OpenCL variable without parameters set.");
		return false;
	}

	if(!parentProgram->Link())
	{
		Log::Send(Log::Error, "Cannot allocate OpenCL variable without link.");
		return false;
	}

	needsUpdate = false;
	return true; // for local memory, no buffer is needed
}

bool CLLocalBuffer::SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int /*deviceID*/, size_t kernelLocalSize)
{
	if(!kernel)
		return false;

	if(!kernel->Kernel())
		return false;

	size_t localMemSize;

	if(kernel->Semantic(argID).find("LOCAL_SIZE_") != std::string::npos)
		localMemSize = DataTypeSize() * (kernelLocalSize + 1);
	else
		localMemSize = memorySize;

	cl_int status = clSetKernelArg(kernel->Kernel(), (cl_uint)argID, localMemSize, NULL);

	if(status)
	{
		Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		return false;
	}

	return true;
}

void CLLocalBuffer::Release()
{

}
