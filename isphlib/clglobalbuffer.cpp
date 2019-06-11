#include "isph.h"
#include <string>
#include <float.h>
#include <cstring>

using namespace isph;

CLGlobalBuffer::CLGlobalBuffer(CLProgram *program, const std::string &semantic)
	: CLVariable(program, semantic, GlobalBuffer)
	, clBuffers(NULL)
	, bufferCount(0)
	, partElementCount(NULL)
	, offsets(NULL)
	, hostHasData(false)
	, hostDataChanged(false)
{
}


CLGlobalBuffer::~CLGlobalBuffer()
{
	Release();
}


bool CLGlobalBuffer::Download(bool waitToFinish, bool forceDownload)
{
	if(needsUpdate)
		if(!Allocate())
			return false;
	
    LogDebug("Reading variable: " + semantics.front());

	if(!data)
	{
		if(!AllocateHostData())
		{
			Log::Send(Log::Error, "Cannot allocate data on host.");
			return false;
		}
	}

	if(hostHasData && !forceDownload)
		return true;

	if(!memorySize || !parentProgram || !clBuffers)
	{
		Log::Send(Log::Error, "Cannot read uninitialized OpenCL buffer.");
		return false;
		//hostHasData = true;
		//hostDataChanged = false;
		//return true;
	}

	cl_int status = 0;
	cl_event* events = NULL;
	if(CLSystem::Instance()->Profiling() || waitToFinish)
		events = new cl_event[bufferCount];

	// enqueue read data from all devices
	for (unsigned int i=0; i<bufferCount; i++)
	{
		/*size_t offsetBytes = Offset(i) * typeSize;
		char *offsetPos = (char*)memoryPos + offsetBytes;
		if(writeable)
			status = clEnqueueReadBuffer(parentProgram->Link()->Queue(i), clBuffers[i], CL_FALSE, offsetBytes, ElementCount(i)*typeSize, offsetPos, 0, NULL, &events[i]);
		else
			status = clEnqueueReadBuffer(parentProgram->Link()->Queue(i), clBuffers[i], CL_FALSE, 0, ElementCount(i)*typeSize, offsetPos, 0, NULL, &events[i]);*/
		
		if(events)
			status = clEnqueueReadBuffer(parentProgram->Link()->Queue(i), clBuffers[i], CL_FALSE, 0, memorySize, data, 0, NULL, &events[i]);
		else
			status = clEnqueueReadBuffer(parentProgram->Link()->Queue(i), clBuffers[i], CL_FALSE, 0, memorySize, data, 0, NULL, NULL);

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	if(CLSystem::Instance()->Profiling())
	{
		clWaitForEvents(bufferCount, events);
		for (unsigned int i=0; i<bufferCount; i++)
		{
			cl_ulong timeStart, timeEnd;
			clGetEventProfilingInfo(events[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &timeStart, NULL);
			clGetEventProfilingInfo(events[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &timeEnd, NULL);
			Log::Send(Log::DebugInfo, Utils::IntegerString((int)(timeEnd-timeStart)/1000) + " microsecs");
			clReleaseEvent(events[i]);
		}
		delete [] events;
	}
	else if(waitToFinish)
	{
		// wait for all devices
		status = clWaitForEvents(bufferCount, events);
		for (unsigned int i=0; i<bufferCount; i++)
			clReleaseEvent(events[i]);
		delete [] events;

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	hostHasData = true;
	hostDataChanged = false;
	
	return true;
}


bool CLGlobalBuffer::Upload(bool waitToFinish)
{
	if(needsUpdate)
		if(!Allocate())
			return false;

	if(!data || !hostHasData)
		return true;

	if(!hostDataChanged)
	{
		hostHasData = false;
		return true;
	}

	LogDebug("Writing variable: " + semantics.front());

	if(!memorySize || !parentProgram || !clBuffers)
	{
		Log::Send(Log::Error, "Cannot write to uninitialized OpenCL buffer.");
		return false;
	}

	cl_int status;
	cl_event* events = NULL;
	if(CLSystem::Instance()->Profiling() || waitToFinish)
		events = new cl_event[bufferCount];

	// enqueue writing data on all devices
	for (unsigned int i=0; i<bufferCount; i++)
	{
		size_t offsetBytes = Offset(i) * DataTypeSize();
		char *offsetPos = data + offsetBytes;

		if(events)
			status = clEnqueueWriteBuffer(parentProgram->Link()->Queue(i), clBuffers[i], CL_FALSE, offsetBytes, ElementCount(i)*DataTypeSize(), offsetPos, 0, NULL, &events[i]);
		else
			status = clEnqueueWriteBuffer(parentProgram->Link()->Queue(i), clBuffers[i], CL_FALSE, offsetBytes, ElementCount(i)*DataTypeSize(), offsetPos, 0, NULL, NULL);

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	if(CLSystem::Instance()->Profiling())
	{
		clWaitForEvents(bufferCount, events);
		for (unsigned int i=0; i<bufferCount; i++)
		{
			cl_ulong timeStart, timeEnd;
			clGetEventProfilingInfo(events[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &timeStart, NULL);
			clGetEventProfilingInfo(events[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &timeEnd, NULL);
			Log::Send(Log::DebugInfo, Utils::IntegerString((int)(timeEnd-timeStart)/1000) + " microsecs");
			clReleaseEvent(events[i]);
		}
		delete [] events;
	}
	else if(waitToFinish)
	{
		// wait for all devices
		status = clWaitForEvents(bufferCount, events);
		for (unsigned int i=0; i<bufferCount; i++)
			clReleaseEvent(events[i]);
		delete [] events;

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	hostDataChanged = false;
	hostHasData = false;

	return true;
}


bool CLGlobalBuffer::CopyFrom(CLGlobalBuffer* var, bool waitToFinish)
{
	LogDebug("Copying variable: " + var->semantics.front() + ", to variable: " + semantics.front());

	if(!var)
	{
		Log::Send(Log::Error, "Cannot copy from NULL variable.");
		return false;
	}

	if(!memorySize || !parentProgram || !clBuffers)
	{
		Log::Send(Log::Error, "Cannot write to uninitialized OpenCL buffer.");
		return false;
	}

	cl_int status;
	cl_event* events = NULL;
	if(CLSystem::Instance()->Profiling() || waitToFinish)
		events = new cl_event[bufferCount];

	for (unsigned int i=0; i<bufferCount; i++)
	{
		if(events)
			status = clEnqueueCopyBuffer(parentProgram->Link()->Queue(i), var->clBuffers[i], clBuffers[i], 0, 0, var->MemorySize(), 0, NULL, &events[i]);
		else
			status = clEnqueueCopyBuffer(parentProgram->Link()->Queue(i), var->clBuffers[i], clBuffers[i], 0, 0, var->MemorySize(), 0, NULL, NULL);

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	if(CLSystem::Instance()->Profiling())
	{
		clWaitForEvents(bufferCount, events);
		for (unsigned int i=0; i<bufferCount; i++)
		{
			cl_ulong timeStart, timeEnd;
			clGetEventProfilingInfo(events[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &timeStart, NULL);
			clGetEventProfilingInfo(events[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &timeEnd, NULL);
			Log::Send(Log::DebugInfo, Utils::IntegerString((int)(timeEnd-timeStart)/1000) + " microsecs");
			clReleaseEvent(events[i]);
		}
		delete [] events;
	}
	else if(waitToFinish)
	{
		// wait for all devices
		status = clWaitForEvents(bufferCount, events);
		for (unsigned int i=0; i<bufferCount; i++)
			clReleaseEvent(events[i]);
		delete [] events;

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	hostHasData = false;
	hostDataChanged = false;

	return true;
}


bool CLGlobalBuffer::Allocate()
{
	bool allocateHost = data ? false : true;

	Release();

	LogDebug("Allocating on device buffer: " + semantics.front());

	if(!memorySize || !parentProgram)
	{
		Log::Send(Log::Error, "Cannot allocate OpenCL variable without parameters set.");
		return false;
	}

	if(!parentProgram->Link())
	{
		Log::Send(Log::Error, "Cannot allocate OpenCL variable without link.");
		return false;
	}

	cl_mem_flags flag = CL_MEM_READ_WRITE;
	bufferCount = 1; // parentProgram->Link()->DeviceCount();

	cl_int status;

	if (IsSplit()) // split the buffer on devices
	{	
		clBuffers = new cl_mem[bufferCount];
		partElementCount = new size_t[bufferCount];
		offsets = new size_t[bufferCount];
		size_t off = 0;

		for (unsigned int i=0; i<bufferCount; i++)
		{
			offsets[i] = off;
			partElementCount[i] = (size_t)ceil(parentProgram->Link()->PerformanceFactor(i) * elementCount);

			off += partElementCount[i];
			if(off > elementCount) // total size will exceed elementCount cos of ceil
				partElementCount[i] -= off - elementCount; // fix last size

			clBuffers[i] = clCreateBuffer(parentProgram->Link()->Context(), flag, partElementCount[i] * DataTypeSize(), NULL, &status);

			if(status)
			{
				Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
				return false;
			}
		}
	}
	else // no need for splitting buffer
	{
		clBuffers = new cl_mem;
		clBuffers[0] = clCreateBuffer(parentProgram->Link()->Context(), flag, memorySize, NULL, &status);

		if(status)
		{
			Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			return false;
		}
	}

	if(allocateHost)
		AllocateHostData();

	needsUpdate = false;
	return true;
}


bool CLGlobalBuffer::SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int deviceID, size_t /*kernelLocalSize*/)
{
	if(!kernel)
		return false;

	if(!kernel->Kernel())
		return false;

	cl_int status;

	if(IsSplit())
		status = clSetKernelArg(kernel->Kernel(), (cl_uint)argID, sizeof(cl_mem), &clBuffers[deviceID]);
	else
		status = clSetKernelArg(kernel->Kernel(), (cl_uint)argID, sizeof(cl_mem), clBuffers);

	if(status)
	{
		Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		return false;
	}

	return true;
}


void CLGlobalBuffer::Release()
{
	cl_int status;
	if(clBuffers)
	{
		if (IsSplit())
		{
			for (unsigned int i=0; i<bufferCount; i++)
			{
				status = clReleaseMemObject(clBuffers[i]);
				if(status)
					Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
			}
		} 
		else
		{
			status = clReleaseMemObject(clBuffers[0]);
			if(status)
				Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		}

		delete [] clBuffers;
	}

	if(partElementCount)
		delete [] partElementCount;

	if(offsets)
		delete [] offsets;

	bufferCount = 0;
	clBuffers = NULL;
	partElementCount = NULL;
	offsets = NULL;

	if(data)
	{
		delete [] data;
		data = NULL;
	}

	hostDataChanged = false;
	hostHasData = false;
}

bool CLGlobalBuffer::AllocateHostData()
{
	data = new char[MemorySize()]();
	hostHasData = false;
	hostDataChanged = false;
	return data ? true : false;
}

double CLGlobalBuffer::GetScalar( unsigned int id )
{
	if(!hostHasData)
		if(!Download())
			return DBL_MAX;

	return CLVariable::GetScalar(id);
}

Vec<3,double> CLGlobalBuffer::GetVector(unsigned int id)
{
	if(!hostHasData)
		if(!Download())
			return DBL_MAX;

	return CLVariable::GetVector(id);
}

bool CLGlobalBuffer::SetScalar( unsigned int id, double var )
{
	if(!data)
		if(!AllocateHostData())
			return false;

	if(!hostHasData)
		if(!Download())
			return false;

	if(!CLVariable::SetScalar(id, var))
		return false;

	hostDataChanged = true;
	return true;
}

bool CLGlobalBuffer::SetVector( unsigned int id, Vec<3,double> var )
{
	if(!data)
		if(!AllocateHostData())
			return false;

	if(!hostHasData)
		if(!Download())
			return false;

	if(!CLVariable::SetVector(id, var))
		return false;

	hostDataChanged = true;
	return true;
}
