#ifndef ISPH_CLPROGRAMCONSTANT_H
#define ISPH_CLPROGRAMCONSTANT_H

#include "clkernelargument.h"

namespace isph
{
	/*!
	 *	\class	CLProgramConstant
	 *	\brief	OpenCL program constant.
	 */
	class CLProgramConstant : public CLKernelArgument
	{
	public:

		CLProgramConstant(CLProgram* program, const std::string& semantic);
		virtual ~CLProgramConstant();

		std::string CLSource();

	private:


	};
}

#endif
