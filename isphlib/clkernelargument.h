#ifndef ISPH_CLKERNELARGUMENT_H
#define ISPH_CLKERNELARGUMENT_H

#include "clvariable.h"

namespace isph
{
	/*!
	 *	\class	CLKernelArgument
	 *	\brief	OpenCL kernel argument.
	 */
	class CLKernelArgument : public CLVariable
	{
	public:
		CLKernelArgument(CLProgram* program, const std::string& semantic, bool dummy);
		CLKernelArgument(CLProgram* program, const std::string& semantic);
		virtual ~CLKernelArgument();

	protected:

		virtual bool Allocate();
		virtual void Release();
		virtual bool SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int deviceID = 0, size_t kernelLocalSize = 0);
      virtual size_t ElementCount(unsigned int /*deviceID*/) { return 0; }

	};
}

#endif
