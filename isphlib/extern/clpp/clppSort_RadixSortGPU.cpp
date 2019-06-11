#include "clpp/clppSort_RadixSortGPU.h"
#include "clpp/clpp.h"

#include "clpp/clppScan_Default.h"

#include "clpp/clppSort_RadixSortGPU_CLKernel.h"

// Next :
// 1 - Allow templating
// 2 - Allow to sort on specific bits only

#pragma region Constructor

clppSort_RadixSortGPU::clppSort_RadixSortGPU(clppContext* context, unsigned int maxElements, unsigned int bits, bool keysOnly)
{
	_keysOnly = keysOnly;
	_valueSize = 4;
	_keySize = 4;
	_clBuffer_dataSet = 0;
	_clBuffer_dataSetOut = 0;

	_bits = bits;

	//if (!compile(context, string("clppSort_RadixSortGPU.cl")))
	//	return;

	if (!compile(context, clCode_clppSort_RadixSortGPU))
		return;

	//---- Prepare all the kernels
	cl_int clStatus;

	_kernel_RadixLocalSort = clCreateKernel(_clProgram, "kernel__radixLocalSort", &clStatus);
	checkCLStatus(clStatus);

	_kernel_LocalHistogram = clCreateKernel(_clProgram, "kernel__localHistogram", &clStatus);
	checkCLStatus(clStatus);

	_kernel_RadixPermute = clCreateKernel(_clProgram, "kernel__radixPermute", &clStatus);
	checkCLStatus(clStatus);

	//---- Get the workgroup size
	//clGetKernelWorkGroupInfo(_kernel_RadixLocalSort, _context->clDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &_workgroupSize, 0);
	_workgroupSize = 32;

	_scan = clpp::createBestScan(context, sizeof(int), maxElements);

    _clBuffer_radixHist1 = NULL;
    _clBuffer_radixHist2 = NULL;
	_datasetSize = 0;
	_is_clBuffersOwner = false;
}

clppSort_RadixSortGPU::~clppSort_RadixSortGPU()
{
	if (_is_clBuffersOwner)
	{
		if (_clBuffer_dataSet)
			clReleaseMemObject(_clBuffer_dataSet);
	}

	if (_clBuffer_dataSetOut)
		clReleaseMemObject(_clBuffer_dataSetOut);

	if (_clBuffer_radixHist1)
		clReleaseMemObject(_clBuffer_radixHist1);

	if (_clBuffer_radixHist2)
		clReleaseMemObject(_clBuffer_radixHist2);

	delete _scan;
}

#pragma endregion

#pragma region compilePreprocess

string clppSort_RadixSortGPU::compilePreprocess(string kernel)
{
	string source;

	//if (_templateType == Int)
	//{
		//source = _keysOnly ? "#define MAX_KV_TYPE (int)(0x7FFFFFFF)\n" : "#define MAX_KV_TYPE (int2)(0x7FFFFFFF,0xFFFFFFFF)\n";
		//source += "#define K_TYPE int\n";
		//source += _keysOnly ? "#define KV_TYPE int\n" : "#define KV_TYPE int2\n";
		//source += "#define K_TYPE_IDENTITY 0\n";
	//}
	//else if (_templateType == UInt)
	//{
		source = _keysOnly ? "#define MAX_KV_TYPE (uint)(0xFFFFFFFF)\n" : "#define MAX_KV_TYPE (uint2)(0xFFFFFFFF,0xFFFFFFFF)\n";
		source += "#define K_TYPE uint\n";
		source += _keysOnly ? "#define KV_TYPE uint\n" : "#define KV_TYPE uint2\n";
		source += "#define K_TYPE_IDENTITY 0\n";
	//}

	if (_keysOnly)
		source += "#define KEYS_ONLY 1\n";

	return clppSort::compilePreprocess(source + kernel);
}

#pragma endregion

#pragma region sort

inline int roundUpDiv(int A, int B) { return (A + B - 1) / (B); }

void clppSort_RadixSortGPU::sort()
{
	// Satish et al. empirically set b = 4. The size of a work-group is in hundreds of
	// work-items, depending on the concrete device and each work-item processes more than one
	// stream element, usually 4, in order to hide latencies.

    unsigned int numBlocks = roundUpDiv(_datasetSize, _workgroupSize * 4);
	unsigned int Ndiv4 = roundUpDiv(_datasetSize, 4);

	size_t global[1] = {toMultipleOf(Ndiv4, _workgroupSize)};
    size_t local[1] = {_workgroupSize};

	cl_mem dataA = _clBuffer_dataSet;
    cl_mem dataB = _clBuffer_dataSetOut;
    for(unsigned int bitOffset = 0; bitOffset < _bits; bitOffset += 4)
	{
		// 1) Each workgroup sorts its tile by using local memory
		// 2) Create an histogram of d=2^b digits entries

        radixLocal(global, local, dataA, _clBuffer_radixHist1, _clBuffer_radixHist2, bitOffset);

        localHistogram(global, local, dataA, _clBuffer_radixHist1, _clBuffer_radixHist2, bitOffset);

		_scan->pushCLDatas(_clBuffer_radixHist1, 16 * numBlocks);
		_scan->scan();

		radixPermute(global, local, dataA, dataB, _clBuffer_radixHist1, _clBuffer_radixHist2, bitOffset, numBlocks);

        std::swap(dataA, dataB);
    }

}

void clppSort_RadixSortGPU::radixLocal(const size_t* global, const size_t* local, cl_mem data, cl_mem hist, cl_mem blockHists, int bitOffset)
{
    cl_int clStatus;
    unsigned int a = 0;

   size_t workgroupSize = 128;

	unsigned int Ndiv = roundUpDiv(_datasetSize, 4); // Each work item handle 4 entries
	size_t global_128[1] = {toMultipleOf(Ndiv, workgroupSize)};
	size_t local_128[1] = {workgroupSize};

	/*if (_keysOnly)
		clStatus  = clSetKernelArg(_kernel_RadixLocalSort, a++, _keySize * 2 * 4 * workgroupSize, (const void*)NULL);
	else
		clStatus  = clSetKernelArg(_kernel_RadixLocalSort, a++, (_valueSize+_keySize) * 2 * 4 * workgroupSize, (const void*)NULL);// 2 KV array of 128 items (2 for permutations)*/
    clStatus = clSetKernelArg(_kernel_RadixLocalSort, a++, sizeof(cl_mem), (const void*)&data);
    clStatus |= clSetKernelArg(_kernel_RadixLocalSort, a++, sizeof(int), (const void*)&bitOffset);
    clStatus |= clSetKernelArg(_kernel_RadixLocalSort, a++, sizeof(unsigned int), (const void*)&_datasetSize);
	clStatus |= clEnqueueNDRangeKernel(_context->clQueue, _kernel_RadixLocalSort, 1, NULL, global_128, local_128, 0, NULL, NULL);

#ifdef BENCHMARK
    clStatus |= clFinish(_context->clQueue);
    checkCLStatus(clStatus);	
#endif
}

void clppSort_RadixSortGPU::localHistogram(const size_t* global, const size_t* local, cl_mem data, cl_mem hist, cl_mem blockHists, int bitOffset)
{
	cl_int clStatus;
	clStatus = clSetKernelArg(_kernel_LocalHistogram, 0, sizeof(cl_mem), (const void*)&data);
	clStatus |= clSetKernelArg(_kernel_LocalHistogram, 1, sizeof(int), (const void*)&bitOffset);
	clStatus |= clSetKernelArg(_kernel_LocalHistogram, 2, sizeof(cl_mem), (const void*)&hist);
	clStatus |= clSetKernelArg(_kernel_LocalHistogram, 3, sizeof(cl_mem), (const void*)&blockHists);
	clStatus |= clSetKernelArg(_kernel_LocalHistogram, 4, sizeof(unsigned int), (const void*)&_datasetSize);
	clStatus |= clEnqueueNDRangeKernel(_context->clQueue, _kernel_LocalHistogram, 1, NULL, global, local, 0, NULL, NULL);	

#ifdef BENCHMARK
    clStatus |= clFinish(_context->clQueue);
    checkCLStatus(clStatus);
#endif
}

void clppSort_RadixSortGPU::radixPermute(const size_t* global, const size_t* local, cl_mem dataIn, cl_mem dataOut, cl_mem histScan, cl_mem blockHists, int bitOffset, unsigned int numBlocks)
{
    cl_int clStatus;
    clStatus  = clSetKernelArg(_kernel_RadixPermute, 0, sizeof(cl_mem), (const void*)&dataIn);
    clStatus |= clSetKernelArg(_kernel_RadixPermute, 1, sizeof(cl_mem), (const void*)&dataOut);
    clStatus |= clSetKernelArg(_kernel_RadixPermute, 2, sizeof(cl_mem), (const void*)&histScan);
    clStatus |= clSetKernelArg(_kernel_RadixPermute, 3, sizeof(cl_mem), (const void*)&blockHists);
    clStatus |= clSetKernelArg(_kernel_RadixPermute, 4, sizeof(int), (const void*)&bitOffset);
    clStatus |= clSetKernelArg(_kernel_RadixPermute, 5, sizeof(unsigned int), (const void*)&_datasetSize);
	clStatus |= clSetKernelArg(_kernel_RadixPermute, 6, sizeof(unsigned int), (const void*)&numBlocks);
    clStatus |= clEnqueueNDRangeKernel(_context->clQueue, _kernel_RadixPermute, 1, NULL, global, local, 0, NULL, NULL);

#ifdef BENCHMARK
    clStatus |= clFinish(_context->clQueue);
    checkCLStatus(clStatus);
#endif
}

#pragma endregion

#pragma region pushDatas

void clppSort_RadixSortGPU::pushDatas(void* dataSet, size_t datasetSize)
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
			clReleaseMemObject(_clBuffer_radixHist1);
			clReleaseMemObject(_clBuffer_radixHist2);
		}

		//---- Allocate
		unsigned int numBlocks = roundUpDiv(_datasetSize, _workgroupSize * 4);
	    
		// histogram : 16 values per block
		_clBuffer_radixHist1 = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, sizeof(int) * 16 * numBlocks, NULL, &clStatus);
		checkCLStatus(clStatus);

		// histogram : 16 values per block
		_clBuffer_radixHist2 = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, sizeof(int) * 16 * numBlocks, NULL, &clStatus);
		checkCLStatus(clStatus);

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
			clEnqueueWriteBuffer(_context->clQueue, _clBuffer_dataSet, CL_FALSE, 0, _keySize * _datasetSize, _dataSet, 0, 0, 0);
		else
			clEnqueueWriteBuffer(_context->clQueue, _clBuffer_dataSet, CL_FALSE, 0, (_valueSize+_keySize) * _datasetSize, _dataSet, 0, 0, 0);
	}
}

void clppSort_RadixSortGPU::pushCLDatas(cl_mem clBuffer_dataSet, size_t datasetSize)
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
			clReleaseMemObject(_clBuffer_radixHist1);
			clReleaseMemObject(_clBuffer_radixHist2);
		}

		//---- Allocate
		unsigned int numBlocks = roundUpDiv(_datasetSize, _workgroupSize * 4);
	    
		// column size = 2^b = 16

		// histogram : 16 values per block
		_clBuffer_radixHist1 = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, sizeof(int) * 16 * numBlocks, NULL, &clStatus);
		checkCLStatus(clStatus);

		// histogram : 16 values per block
		_clBuffer_radixHist2 = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, sizeof(int) * 16 * numBlocks, NULL, &clStatus);
		checkCLStatus(clStatus);
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

void clppSort_RadixSortGPU::popDatas()
{
	popDatas(_dataSetOut);
}

void clppSort_RadixSortGPU::popDatas(void* dataSet)
{
	if (_keysOnly)
	{
		if ((_bits/4) % 2 == 0)
			clEnqueueReadBuffer(_context->clQueue, _clBuffer_dataSet, CL_TRUE, 0, _keySize * _datasetSize, dataSet, 0, NULL, NULL);
		else
			clEnqueueReadBuffer(_context->clQueue, _clBuffer_dataSetOut, CL_TRUE, 0, _keySize * _datasetSize, dataSet, 0, NULL, NULL);
	}
	else
	{
		if ((_bits/4) % 2 == 0)
			clEnqueueReadBuffer(_context->clQueue, _clBuffer_dataSet, CL_TRUE, 0, (_valueSize + _keySize) * _datasetSize, dataSet, 0, NULL, NULL);
		else
			clEnqueueReadBuffer(_context->clQueue, _clBuffer_dataSetOut, CL_TRUE, 0, (_valueSize + _keySize) * _datasetSize, dataSet, 0, NULL, NULL);
	}
}

#pragma endregion
