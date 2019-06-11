#ifndef ISPH_CLLOCALBUFFER_H
#define ISPH_CLLOCALBUFFER_H

#include "clvariable.h"

namespace isph
{
	/*!
	 *	\class	CLLocalBuffer
	 *	\brief	OpenCL local buffer. Unaccessable from host.
	 */
	class CLLocalBuffer : public CLVariable
	{
	public:

		CLLocalBuffer(CLProgram* program, const std::string& semantic);
		virtual ~CLLocalBuffer();

	private:

		virtual bool Allocate();
		virtual void Release();
		virtual bool SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int deviceID = 0, size_t kernelLocalSize = 0);
      virtual size_t ElementCount(unsigned int /*deviceID*/) { return 0; }
	};
}

#endif
