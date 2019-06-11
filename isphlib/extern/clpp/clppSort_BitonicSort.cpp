#include "clpp/clppSort_BitonicSort.h"
#include "clpp/clpp.h"

#include "clpp/clppScan_Default.h"

#include "clpp/clppSort_BitonicSort_CLKernel.h"

// Next :
// 1 - Allow templating
// 2 - Allow to sort on specific bits only

#pragma region Constructor

clppSort_BitonicSort::clppSort_BitonicSort(clppContext* context, unsigned int maxElements, bool keysOnly)
{
	_keysOnly = keysOnly;
	_valueSize = 4;
	_keySize = 4;
	_clBuffer_dataSet = 0;
	_clBuffer_dataSetOut = 0;

	if (!compile(context, clCode_clppSort_BitonicSort))
		return;

	//if (!compile(context, string("clppSort_BitonicSort.cl")))
	//	return;

	//---- Prepare all the kernels
	cl_int clStatus;

	_kernel__BitonicSort = clCreateKernel(_clProgram, "kernel__BitonicSort", &clStatus);
	checkCLStatus(clStatus);

	clGetKernelWorkGroupInfo(_kernel__BitonicSort, _context->clDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &_workgroupSize, 0);

	_datasetSize = 0;
	_is_clBuffersOwner = false;
}

clppSort_BitonicSort::~clppSort_BitonicSort()
{
	if (_is_clBuffersOwner)
	{
		if (_clBuffer_dataSet)
			clReleaseMemObject(_clBuffer_dataSet);
	}

	if (_clBuffer_dataSetOut)
		clReleaseMemObject(_clBuffer_dataSetOut);
}

#pragma endregion

#pragma region compilePreprocess

string clppSort_BitonicSort::compilePreprocess(string kernel)
{
	string source;

	//if (_templateType == Int)
	{
		source = _keysOnly ? "#define KV_TYPE uint\n" : "#define KV_TYPE uint2\n";

		if (_keysOnly)
			source += "#define KEYS_ONLY 1\n";
	}
	/*else if (_templateType == UInt)
	{
		source = "#define MAX_KV_TYPE ((int2)0xFFFFFFFF)\n";
		source += "#define K_TYPE uint\n";
		source += "#define KV_TYPE uint2\n";
		source += "#define K_TYPE_IDENTITY 0\n";
	}*/

	return clppSort::compilePreprocess(source + kernel);
}

#pragma endregion

#pragma region sort

inline int roundUpDiv(int A, int B) { return (A + B - 1) / (B); }

void clppSort_BitonicSort::sort()
{
	const size_t global[1] = { _datasetSize / 2 };
	size_t local[1] = { _workgroupSize };
	while(local[0] > _datasetSize*0.25) local[0] *= 0.5f;
	
    cl_int clStatus;
    clStatus  = clSetKernelArg(_kernel__BitonicSort, 0, sizeof(cl_mem), (const void*)&_clBuffer_dataSet);
    //clStatus |= clSetKernelArg(_kernel__BitonicSort, 1, sizeof(cl_mem), (const void*)dataOut);
	
	cl_uint numStages = 0;
	for(unsigned int temp = _datasetSize; temp > 1; temp >>= 1)
		++numStages;
	
	for(cl_uint stage = 0; stage < numStages; ++stage)
	{
		clStatus = clSetKernelArg(_kernel__BitonicSort, 1, sizeof(int), (const void*)&stage);

		for(cl_uint passOfStage = 0; passOfStage < stage + 1; ++passOfStage)
		{
			clStatus = clSetKernelArg(_kernel__BitonicSort, 2, sizeof(int), (const void*)&passOfStage);

			clEnqueueNDRangeKernel(_context->clQueue, _kernel__BitonicSort, 1, NULL, global, local, 0, NULL, NULL);
			
#ifdef BENCHMARK
    clStatus |= clFinish(_context->clQueue);
    checkCLStatus(clStatus);
#endif
		}
	}
}

#pragma endregion

#pragma region pushDatas

void clppSort_BitonicSort::pushDatas(void* dataSet, size_t datasetSize)
{
	cl_int clStatus;

	//---- Store some values
	_dataSet = dataSet;
	_dataSetOut = dataSet;
	bool reallocate = datasetSize > _datasetSize || !_is_clBuffersOwner;
	_datasetSize = datasetSize;

	//---- Prepare some buffers
	if (reallocate)
	{
		//---- Release
		if (_clBuffer_dataSet)
		{
			clReleaseMemObject(_clBuffer_dataSet);
			clReleaseMemObject(_clBuffer_dataSetOut);
		}

		//---- Copy on the device
		if (_keysOnly)
		{
			_clBuffer_dataSet = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, _keySize * _datasetSize, _dataSet, &clStatus);
			checkCLStatus(clStatus);

			_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, _keySize * _datasetSize, NULL, &clStatus);
			checkCLStatus(clStatus);
		}
		else
		{
			_clBuffer_dataSet = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, (_valueSize+_keySize) * _datasetSize, _dataSet, &clStatus);
			checkCLStatus(clStatus);

			_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, (_valueSize+_keySize) * _datasetSize, NULL, &clStatus);
			checkCLStatus(clStatus);
		}

		_is_clBuffersOwner = true;
	}
	else
	{
		// Just resend
		if (_keysOnly)
			clEnqueueWriteBuffer(_context->clQueue, _clBuffer_dataSet, CL_FALSE, 0, (_keySize) * _datasetSize, _dataSet, 0, 0, 0);
		else
			clEnqueueWriteBuffer(_context->clQueue, _clBuffer_dataSet, CL_FALSE, 0, (_valueSize+_keySize) * _datasetSize, _dataSet, 0, 0, 0);
	}
}

void clppSort_BitonicSort::pushCLDatas(cl_mem clBuffer_dataSet, size_t datasetSize)
{
	cl_int clStatus;

	_is_clBuffersOwner = false;

	//---- Store some values
	bool reallocate = datasetSize > _datasetSize;
	_datasetSize = datasetSize;

	//---- Prepare some buffers
	if (reallocate)
	{
		//---- Release
		if (_clBuffer_dataSet)
		{
			clReleaseMemObject(_clBuffer_dataSet);
			clReleaseMemObject(_clBuffer_dataSetOut);
		}

		//---- Allocate
		unsigned int numBlocks = roundUpDiv(_datasetSize, _workgroupSize * 4);
	}

	// ISSUE : We need 2 different buffers, but
	// a) when using 32 bits sort(by example) the result buffer is _clBuffer_dataSet
	// b) when using 28 bits sort(by example) the result buffer is _clBuffer_dataSetOut
	// Without copy, how can we do to put the result in _clBuffer_dataSet when using 28 bits ?

	_clBuffer_dataSet = clBuffer_dataSet;
	
	if (_keysOnly)
		_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, _keySize * _datasetSize, NULL, &clStatus);
	else
		_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, (_valueSize+_keySize) * _datasetSize, NULL, &clStatus);
	checkCLStatus(clStatus);
}

#pragma endregion

#pragma region popDatas

void clppSort_BitonicSort::popDatas()
{
	popDatas(_dataSetOut);
}

void clppSort_BitonicSort::popDatas(void* dataSet)
{
	if (_keysOnly)
	{
		clEnqueueReadBuffer(_context->clQueue, _clBuffer_dataSet, CL_TRUE, 0, _keySize * _datasetSize, dataSet, 0, NULL, NULL);
	}
	else
	{
			clEnqueueReadBuffer(_context->clQueue, _clBuffer_dataSet, CL_TRUE, 0, (_valueSize + _keySize) * _datasetSize, dataSet, 0, NULL, NULL);
	}
}

#pragma endregion