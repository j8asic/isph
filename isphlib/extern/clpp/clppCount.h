#ifndef __CLPP_COUNT_H__
#define __CLPP_COUNT_H__

#include "clpp/clppProgram.h"
#include "clpp/clppScan.h"

class clppCount : public clppProgram
{
public:
	
	// Create a new counting program.
	// maxElements : the maximum number of elements to scan.
	clppCount(clppContext* context, size_t valueSize, unsigned int countings, unsigned int maxElements);
	~clppCount();

	// Returns the algorithm name
	string getName();

	// Start the counting operation
	void count();

	// Send a Host data set to the device
	void pushDatas(void* values, size_t datasetSize);

	// Push a buffer that is already on the device side. (Data are not sended)
	void pushCLDatas(cl_mem clBuffer_values, size_t datasetSize);

	// Retreive the datas (To the same zone as the source).
	void popDatas();
	void popDatas(void* dataSet);

protected:
	size_t _datasetSize;	// The number of values to scan

	void* _values;			// The associated data set to scan
	size_t _valueSize;		// The size of a value in bytes

	cl_mem _clBuffer_values;
	bool _is_clBuffersOwner;

	size_t _workgroupSize;
	
	// The number of parallel countings
	unsigned int _countings;

	// The temporary buffer used to count
	cl_mem _clBuffer_CountingBlocks;

	// The buffers that will contains the results
	unsigned int* _countingsBuffer;
	cl_mem _clBuffer_Countings;

	cl_kernel _kernel_Count;

	clppScan* _scan;
};

#endif