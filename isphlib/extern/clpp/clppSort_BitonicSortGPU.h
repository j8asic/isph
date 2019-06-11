#ifndef __CLPP_SORT_BITONIC_GPU_H__
#define __CLPP_SORT_BITONIC_GPU_H__

#include <vector>

#include "clpp/clppSort.h"

using namespace std;

class clppSort_BitonicSortGPU : public clppSort
{
public:
	clppSort_BitonicSortGPU(clppContext* context, unsigned int maxElements, bool keysOnly);
	~clppSort_BitonicSortGPU();

	string getName() { return "Bitonic sort GPU"; }

	void sort();

	void pushDatas(void* dataSet, size_t datasetSize);
	void pushCLDatas(cl_mem clBuffer_dataSet, size_t datasetSize);

	void popDatas();
	void popDatas(void* dataSet);

	string compilePreprocess(string kernel);

private:
	bool _keysOnly;			// Key-Values or Keys-only
	size_t _datasetSize;	// The number of keys to sort

	void* _dataSetOut;
	cl_mem _clBuffer_dataSetOut;

	//cl_kernel _kernel__BitonicSort;

	std::vector<cl_kernel> _kernels;

	size_t _workgroupSize;

	bool _is_clBuffersOwner;
};

#endif