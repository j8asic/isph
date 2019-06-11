#include "clpp/clppCount.h"
//#include "clpp/clppCount_CLKernel.h"
#include "clpp/clpp.h"

#pragma region Constructor

clppCount::clppCount(clppContext* context, size_t valueSize, unsigned int countings, unsigned int maxElements) :
	clppProgram() 
{
	_values = 0;
	_context = context;
	_valueSize = valueSize;
	_countings = countings;
	_datasetSize = 0;
	_clBuffer_values = 0;
	_workgroupSize = 0;
	_is_clBuffersOwner = false;
	_clBuffer_CountingBlocks = 0;
	_countingsBuffer = 0;
	_clBuffer_Countings = 0;

	//if (!compile(context, clCode_clppScan_Default))
	//	return;

	if (!compile(context, string("clppCount.cl")))
		return;

	//---- Prepare all the kernels
	cl_int clStatus;

	_kernel_Count = clCreateKernel(_clProgram, "kernel__Count", &clStatus);
	checkCLStatus(clStatus);

	//---- Get the workgroup size
	clGetKernelWorkGroupInfo(_kernel_Count, _context->clDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &_workgroupSize, 0);
	//_workgroupSize = 128;
	//_workgroupSize = 256;
	//_workgroupSize = 512;
	//clGetKernelWorkGroupInfo(_kernel_Scan, _context->clDevice, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &_workgroupSize, 0);

	string scanExpression = "#define OPERATOR_APPLY(DATA,INDEX1,INDEX2) DATA[INDEX1]+DATA[INDEX2]";

//#define T int
//#define OPERATOR_APPLY(A,B) A+B
//#define OPERATOR_IDENTITY 0

	_scan = clpp::createBestScan(context, sizeof(int), maxElements);
}

clppCount::~clppCount()
{
	if (_is_clBuffersOwner && _clBuffer_values)
		clReleaseMemObject(_clBuffer_values);

	if (_clBuffer_CountingBlocks)
		clReleaseMemObject(_clBuffer_CountingBlocks);

	if (_clBuffer_CountingBlocks)
	{
		clReleaseMemObject(_clBuffer_Countings);
		free(_countingsBuffer);
	}
}

#pragma endregion

#pragma region count

void clppCount::count()
{
	cl_int clStatus;

	//---- Local count
	unsigned int valuesPerWorkgroup = _datasetSize / _workgroupSize;
	size_t globalWorkSize = {toMultipleOf(_datasetSize, _workgroupSize)};
	size_t localWorkSize = {_workgroupSize};

	clStatus = clSetKernelArg(_kernel_Count, 0, sizeof(cl_mem), &_clBuffer_values);
	clStatus |= clSetKernelArg(_kernel_Count, 1, sizeof(cl_mem), &_clBuffer_CountingBlocks);
	clStatus |= clSetKernelArg(_kernel_Count, 2, sizeof(int), &_countings);
	clStatus |= clSetKernelArg(_kernel_Count, 3, sizeof(int), &valuesPerWorkgroup);
	clStatus |= clSetKernelArg(_kernel_Count, 4, sizeof(int), &_datasetSize);

	clStatus |= clEnqueueNDRangeKernel(_context->clQueue, _kernel_Count, 1, NULL, &globalWorkSize, &localWorkSize, 0, NULL, NULL);
	checkCLStatus(clStatus);

	//---- Scan to retreive the totals
	_scan->pushCLDatas(_clBuffer_CountingBlocks, globalWorkSize);
	_scan->scan();
}

#pragma endregion

#pragma region pushDatas

void clppCount::pushDatas(void* values, size_t datasetSize)
{
	cl_int clStatus;

	//---- Store some values
	_values = values;
	bool reallocate = datasetSize > _datasetSize || !_is_clBuffersOwner;
	bool recompute =  datasetSize != _datasetSize;
	_datasetSize = datasetSize;

	//---- Allocate the results buffer
	if (!_countingsBuffer)
	{
		_countingsBuffer = (unsigned int*)malloc(_countings * sizeof(int));
		_clBuffer_Countings  = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, _countings * sizeof(int), 0, &clStatus);
	}

	//---- Compute the number of temporary blocks
	if (recompute)
	{
		int workgroupsCount = (_datasetSize + _workgroupSize-1) / _workgroupSize;
		int blocksCount = _countings * workgroupsCount;

		if (_clBuffer_CountingBlocks)
			clReleaseMemObject(_clBuffer_CountingBlocks);

		_clBuffer_CountingBlocks  = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE, blocksCount * sizeof(int), NULL, &clStatus);
	}

	//---- Copy on the device
	if (reallocate)
	{
		//---- Release
		if (_clBuffer_values)
			clReleaseMemObject(_clBuffer_values);

		_clBuffer_values  = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, _valueSize * _datasetSize, _values, &clStatus);
		_is_clBuffersOwner = true;
		checkCLStatus(clStatus);
	}
	else
		// Just resend
		clEnqueueWriteBuffer(_context->clQueue, _clBuffer_values, CL_FALSE, 0, _valueSize * _datasetSize, _values, 0, 0, 0);
}

void clppCount::pushCLDatas(cl_mem clBuffer_values, size_t datasetSize)
{
	cl_int clStatus;

	_values = 0;
	_clBuffer_values = clBuffer_values;
	bool recompute =  datasetSize != _datasetSize;
	_datasetSize = datasetSize;

	//---- Allocate the results buffer
	if (!_countingsBuffer)
	{
		_countingsBuffer = (unsigned int*)malloc(_countings * sizeof(int));
		_clBuffer_Countings  = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, _countings * sizeof(int), 0, &clStatus);
	}

	//---- Compute the number of needed blocks
	if (recompute)
	{
		int blocksCount = _countings * (_datasetSize + _workgroupSize-1) / _workgroupSize;

		if (_clBuffer_CountingBlocks)
			clReleaseMemObject(_clBuffer_CountingBlocks);

		_clBuffer_CountingBlocks  = clCreateBuffer(_context->clContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, blocksCount * sizeof(int), 0, &clStatus);
	}

	_is_clBuffersOwner = false;
}

#pragma endregion

#pragma region popDatas

void clppCount::popDatas()
{
	cl_int clStatus = clEnqueueReadBuffer(_context->clQueue, _clBuffer_values, CL_TRUE, 0, _valueSize * _datasetSize, _values, 0, NULL, NULL);
	checkCLStatus(clStatus);
}

void clppCount::popDatas(void* dataSet)
{
	cl_int clStatus = clEnqueueReadBuffer(_context->clQueue, _clBuffer_values, CL_TRUE, 0, _valueSize * _datasetSize, dataSet, 0, NULL, NULL);
	checkCLStatus(clStatus);
}

#pragma endregion