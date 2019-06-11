#ifndef ISPH_CLGLOBALBUFFER_H
#define ISPH_CLGLOBALBUFFER_H

#include "clvariable.h"

namespace isph
{

	/*!
	 *	\class	CLGlobalBuffer
	 *	\brief	OpenCL global buffer.
	 */
	class CLGlobalBuffer : public CLVariable
	{
	public:
		CLGlobalBuffer(CLProgram* program, const std::string& semantic);
		virtual ~CLGlobalBuffer();

		inline void* HostData() { return data; }

		inline bool HostHasData() { return hostHasData; }

		inline bool HostDataChanged() { return hostDataChanged; }

		/*!
		 *	\brief	Read the data from devices to the host.
		 *	\param	waitToFinish Wait for reading to finish before returning from function.
		 */
		bool Download(bool waitToFinish = true, bool forceDownload = false);

		/*!
		 *	\brief	Write the data from host to devices.
		 *	\param	waitToFinish Wait for writing to finish before returning from function.
		 */
		bool Upload(bool waitToFinish = true);

		/*!
		 *	\brief	Copy data from another buffer.
		 *	\param	var	Variable data that will be copied to this variable.
		 *	\param	waitToFinish Wait for copying to finish before returning from function.
		 */
		bool CopyFrom(CLGlobalBuffer* var, bool waitToFinish = true);

		virtual bool SetScalar(unsigned int id, double var);
		bool SetScalar(double var) { return SetScalar(0, var); }

		virtual bool SetVector(unsigned int id, Vec<3,double> var);
		bool SetVector(Vec<3,double> var) { return SetVector(0, var); }

		virtual double GetScalar(unsigned int id = 0);

		virtual Vec<3,double> GetVector(unsigned int id = 0);

		/*!
		 *	\brief	Return OpenCL memory object of the variable.
		 */
		inline cl_mem Buffer(unsigned int device) { return clBuffers[device]; }

	private:

		inline bool IsSplit() { return bufferCount > 1; }
		inline size_t Offset(unsigned int deviceID) { return IsSplit() ? offsets[deviceID] : 0; }
		virtual size_t ElementCount(unsigned int deviceID) { return IsSplit() ? partElementCount[deviceID] : elementCount; }

		virtual bool Allocate();
		virtual void Release();
		virtual bool SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int deviceID = 0, size_t kernelLocalSize = 0);

		cl_mem* clBuffers;
		unsigned int bufferCount;
		size_t* partElementCount;
		size_t* offsets;

		// host
		bool AllocateHostData();
		bool hostHasData;
		bool hostDataChanged;

	};

}

#endif
