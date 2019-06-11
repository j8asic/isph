// Adapted from the Eric Bainville code.
//
// Copyright (c) Eric Bainville - June 2011
// http://www.bealto.com/gpu-sorting_intro.html

//#define BENCHMARK
#include "clpp/clppSort_BitonicSortGPU.h"
#include "clpp/clpp.h"

#include "clpp/clppScan_Default.h"

#include <list>

#include "clpp/clppSort_BitonicSortGPU_CLKernel.h"

// Next :
// 1 - Allow templating
// 2 - Allow to sort on specific bits only

enum Kernels {
  PARALLEL_BITONIC_B2_KERNEL,
  PARALLEL_BITONIC_B4_KERNEL,
  PARALLEL_BITONIC_B8_KERNEL,
  PARALLEL_BITONIC_B16_KERNEL,
  PARALLEL_BITONIC_C4_KERNEL,
  NB_KERNELS
};
const char * KernelNames[NB_KERNELS+1] = {
  "ParallelBitonic_B2",
  "ParallelBitonic_B4",
  "ParallelBitonic_B8",
  "ParallelBitonic_B16",
  "ParallelBitonic_C4",
  0 };

#pragma region Constructor

clppSort_BitonicSortGPU::clppSort_BitonicSortGPU(clppContext* context, unsigned int maxElements, bool keysOnly)
{
	_keysOnly = keysOnly;
	_valueSize = 4;
	_keySize = 4;
	_clBuffer_dataSet = 0;
	_clBuffer_dataSetOut = 0;

	if (!compile(context, clCode_clppSort_BitonicSortGPU))
		return;

	//if (!compile(context, string("clppSort_BitonicSortGPU.cl")))
	//	return;

	//---- Prepare all the kernels
	cl_int clStatus;

	for(int i = 0; i < NB_KERNELS; i++)
	{
		_kernels.push_back( clCreateKernel(_clProgram, KernelNames[i], &clStatus) );
		checkCLStatus(clStatus);
	}

	_datasetSize = 0;
	_is_clBuffersOwner = false;
}

clppSort_BitonicSortGPU::~clppSort_BitonicSortGPU()
{
	if (_is_clBuffersOwner)
	{
		if (_clBuffer_dataSet)
			clReleaseMemObject(_clBuffer_dataSet);
	}

	//if (_clBuffer_dataSetOut)
	//	clReleaseMemObject(_clBuffer_dataSetOut);
}

#pragma endregion

#pragma region compilePreprocess

string clppSort_BitonicSortGPU::compilePreprocess(string kernel)
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

// Allowed "Bx" kernels (bit mask)
#define ALLOWB (2+4+8)

void clppSort_BitonicSortGPU::sort()
{
	int keyValueSize = _keysOnly ? _keySize : (_valueSize+_keySize);

	for(int length = 1; length < _datasetSize; length <<= 1)
    {
		int inc = length;
		std::list<int> strategy; // vector defining the sequence of reductions
		{
			int ii = inc;
			while (ii>0)
			{
				if (ii==128 || ii==32 || ii==8) { strategy.push_back(-1); break; } // C kernel
				int d = 1; // default is 1 bit
				if (0) d = 1;
	#if 1
				// Force jump to 128
				else if (ii==256) d = 1;
				else if (ii==512 && (ALLOWB & 4)) d = 2;
				else if (ii==1024 && (ALLOWB & 8)) d = 3;
				else if (ii==2048 && (ALLOWB & 16)) d = 4;
	#endif
				else if (ii>=8 && (ALLOWB & 16)) d = 4;
				else if (ii>=4 && (ALLOWB & 8)) d = 3;
				else if (ii>=2 && (ALLOWB & 4)) d = 2;
				else d = 1;

				strategy.push_back(d);
				ii >>= d;
			}
		}

		while (inc > 0)
		{
			int ninc = 0;
			int kid = -1;
			int doLocal = 0;
			int nThreads = 0;
			int d = strategy.front(); strategy.pop_front();

			switch (d)
			{
			case -1:
				kid = PARALLEL_BITONIC_C4_KERNEL;
				ninc = -1; // reduce all bits
				doLocal = 4;
				nThreads = _datasetSize >> 2;
				break;
			case 4:
				kid = PARALLEL_BITONIC_B16_KERNEL;
				ninc = 4;
				nThreads = _datasetSize >> ninc;
				break;
			case 3:
				kid = PARALLEL_BITONIC_B8_KERNEL;
				ninc = 3;
				nThreads = _datasetSize >> ninc;
				break;
			case 2:
				kid = PARALLEL_BITONIC_B4_KERNEL;
				ninc = 2;
				nThreads = _datasetSize >> ninc;
				break;
			case 1:
				kid = PARALLEL_BITONIC_B2_KERNEL;
				ninc = 1;
				nThreads = _datasetSize >> ninc;
				break;
			default:
				printf("Strategy error!\n");
				break;
			}

			//---- Execute the kernel
			size_t wg;
			clGetKernelWorkGroupInfo(_kernels[kid], _context->clDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &wg, 0);
			wg = std::min<size_t>(wg, (size_t)256);
			wg = std::min<size_t>(wg, (size_t)nThreads);

			cl_int clStatus = 0;
			unsigned int pId = 0;
			clStatus |= clSetKernelArg(_kernels[kid], pId++, sizeof(cl_mem), (const void*)&_clBuffer_dataSet);
			clStatus |= clSetKernelArg(_kernels[kid], pId++, sizeof(int), &inc);		// INC passed to kernel
			int lenght2 = length << 1;
			clStatus |= clSetKernelArg(_kernels[kid], pId++, sizeof(int), &lenght2);	// DIR passed to kernel
			if (doLocal>0)
				clStatus |= clSetKernelArg(_kernels[kid], pId++, doLocal * wg * keyValueSize, 0);
			clStatus |= clSetKernelArg(_kernels[kid], pId++, sizeof(unsigned int), (const void*)&_datasetSize);

         size_t global[1] = {(size_t)nThreads};
         size_t local[1] = {(size_t)wg};
			clStatus |= clEnqueueNDRangeKernel(
									_context->clQueue,
									_kernels[kid],
									1,						// work_dim
									0,						// global_work_offset
									global,					// global_work_size
									local,					// local_work_size
									0, NULL, NULL);

			// Sync
			clStatus |= clEnqueueBarrier(_context->clQueue);

			if (ninc < 0) break; // done
			inc >>= ninc;
		}
    }
}

#pragma endregion

#pragma region pushDatas

void clppSort_BitonicSortGPU::pushDatas(void* dataSet, size_t datasetSize)
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
			//clReleaseMemObject(_clBuffer_dataSetOut);
		}

		//---- Copy on the device
		if (_keysOnly)
		{
			_clBuffer_dataSet = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, _keySize * _datasetSize, _dataSet, &clStatus);
			checkCLStatus(clStatus);

			//_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, _keySize * _datasetSize, NULL, &clStatus);
			//checkCLStatus(clStatus);
		}
		else
		{
			_clBuffer_dataSet = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, (_valueSize+_keySize) * _datasetSize, _dataSet, &clStatus);
			checkCLStatus(clStatus);

			//_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, (_valueSize+_keySize) * _datasetSize, NULL, &clStatus);
			//checkCLStatus(clStatus);
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

void clppSort_BitonicSortGPU::pushCLDatas(cl_mem clBuffer_dataSet, size_t datasetSize)
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
			//clReleaseMemObject(_clBuffer_dataSetOut);
		}

		//---- Allocate
		unsigned int numBlocks = roundUpDiv(_datasetSize, _workgroupSize * 4);
	}

	// ISSUE : We need 2 different buffers, but
	// a) when using 32 bits sort(by example) the result buffer is _clBuffer_dataSet
	// b) when using 28 bits sort(by example) the result buffer is _clBuffer_dataSetOut
	// Without copy, how can we do to put the result in _clBuffer_dataSet when using 28 bits ?

	_clBuffer_dataSet = clBuffer_dataSet;
	
	//if (_keysOnly)
	//	_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, _keySize * _datasetSize, NULL, &clStatus);
	//else
	//	_clBuffer_dataSetOut = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, (_valueSize+_keySize) * _datasetSize, NULL, &clStatus);
	//checkCLStatus(clStatus);
}

#pragma endregion

#pragma region popDatas

void clppSort_BitonicSortGPU::popDatas()
{
	popDatas(_dataSetOut);
}

void clppSort_BitonicSortGPU::popDatas(void* dataSet)
{
	cl_int clStatus;


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
