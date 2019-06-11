#ifndef ISPH_CLSUBPROGRAM_H
#define ISPH_CLSUBPROGRAM_H

#include <vector>
#include <string>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace isph {

	class CLProgram;
	class CLVariable;

	/*!
	 *	\class	CLSubProgram
	 *	\brief	Part of source code of CLProgram.
	 *
	 *	The class loads, parses, and holds the parsed code. When subprogram is added to CLProgram, parsed semantics
	 *	are directly connected to the program variables, and will be automatically managed. Semantic/variable
	 *	connection will be made only if program has variable that is specified, on disposal.
	 *	Semantics are defined as a word after the colon ':', after the parameter declaration in kernel function.
	 *	Subprograms are allowed to have any number of functions (even none at all), but maximum one kernel function.
	 *	When subprogram has a kernel function, it can be called by CLProgram. Subprograms without kernel function (e.g.
	 *	auxiliary functions, global variables, etc) can be called/accessed by other subprograms, but not from CLProgram. 
	 */
	class CLSubProgram
	{
	public:

		/*!
		 *	\brief	Create new subprogram object.
		 */
		CLSubProgram();

		virtual ~CLSubProgram();

		/*!
		 *	\brief	Set source code directly from string.
		 */
		bool SetSource(const std::string& sourceCode);

		/*!
		 *	\brief	Get source code.
		 */
		inline const std::string& Source() { return source; }

		/*!
		 *	\brief	Check if subprogram is a kernel function.
		 */
		inline bool IsKernel() { return isKernel; }

		/*!
		 *	\brief	Get OpenCL kernel object.
		 */
		inline cl_kernel& Kernel() { return kernel; }

		/*!
		 *	\brief	Get function name of the kernel.
		 */
		inline const std::string& KernelName() { return kernelName; }

		/*!
		 *	\brief	Enqueue/run kernel.
		 */
		bool Enqueue(size_t globalSize=0, size_t localSize=0);

		/*!
		 *	\brief	Get kernel semantic count.
		 */
		inline unsigned int SemanticCount() { return (unsigned int)semantics.size(); }

		/*!
		 *	\brief	Get kernel semantic.
		 */
		inline const std::string& Semantic(unsigned int i) { return semantics[i]; }

		/*!
		 *	\brief	Get if kernel depends on a semantic.
		 */
		int SemanticIndex(const std::string& semantic);

		/*!
		 *	\brief	Get parallelization semantic.
		 */
		inline const std::string& ParallelizeSemantic() { return parallelizeSemantic; }

	private:

		friend class CLProgram;
		friend class CLVariable;

		void ParseSemantics(size_t startStringPos);
		inline bool IsLiteral(char c);
		bool CreateKernel();
		void ReleaseKernel();

		bool SetPersistentArguments();

		CLProgram* program;
		std::string source;
		bool isKernel;
		std::string kernelName;
		std::vector<std::string> semantics;
		std::string parallelizeSemantic;

		// kernel stuff
		cl_kernel kernel; /// \todo make cl_kernel array for each device ?
		size_t* workGroupSizes;
	};

}

#endif
