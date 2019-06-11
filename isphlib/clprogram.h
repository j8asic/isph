#ifndef ISPH_CLPROGRAM_H
#define ISPH_CLPROGRAM_H

#include <string>
#include <vector>
#include <list>
#include <map>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace isph {

	class CLSubProgram;
	class CLPlatform;
	class CLDevice;
	class CLVariable;
	class CLGlobalBuffer;
	class CLLocalBuffer;
	class CLKernelArgument;
	class CLProgramConstant;
	class CLLink;

	/*!
	 *	\class	CLProgram
	 *	\brief	Compound of CLSubProgram and CLVariable objects ready to build and run on devices.
	 */
	class CLProgram
	{
	public:

		CLProgram();
		~CLProgram();

		/*!
		 *	\brief	Set link to devices that will run the program.
		 *	\param	linkToDevices	Link to OpenCL enabled devices that will run the program.
		 */
		bool SetLink(CLLink* linkToDevices);

		/*!
		 *	\brief	Get link to devices that will run the program.
		 */
		inline CLLink* Link() { return link; }

		/*!
		 *	\brief	Add a part of program.
		 *	\param	subprogram	Pointer to the part of the source code.
		 */
		bool AddSubprogram(CLSubProgram* subprogram);

		/*!
		 *	\brief	Remove all parts of program.
		 */
		void ClearSubprograms();

		/*!
       *	\brief	Set which math optimizations to use.
		 */
      void SetOptimizations(bool multiplyAdd, bool unsafeMathOpts, bool finiteMathOnly, bool disableDenormals);

		/*!
		 *	\brief	Add custom build options (e.g. custom preprocessor define).
		 */
		void AddBuildOption(const std::string& option);

		/*!
		 *	\brief	Remove added custom build options.
		 */
		void ClearBuildOptions();

		/*!
		 *	\brief	Compile and link the program on devices.
		 *	\remarks Set devices (SetLink) before calling this function.
		 */
		bool Build();

		/*!
		 *	\brief	Check if program is successfully compiled and linked.
		 */
		inline bool IsBuilt() { return isBuilt; }

		/*!
		 *	\brief	Get the whole program source code.
		 */
		inline const std::string& Source() { return source; }

		/*!
		 *	\brief	Get the program source in compiled form.
		 */
		std::string CompiledBinary();

		/*!
		 *	\brief	Wait until program has finished.
		 */
		bool Finish();

		/*!
		 *	\brief	Get the map of program's variables (with its semantics as keys).
		 */
		inline const std::map<std::string,CLVariable*>& Variables() { return variables; }

		/*!
		 *	\brief	Get the program variable from its semantic.
		 */
		CLVariable* Variable(const std::string& semantic);

		/*!
		 *	\brief	Get the map of program's variables (with its semantics as keys).
		 */
		inline const std::map<std::string,CLGlobalBuffer*>& Buffers() { return globalBuffers; }

		/*!
		 *	\brief	Get the program global buffer variable from its semantic.
		 */
		CLGlobalBuffer* Buffer(const std::string& semantic);

		/*!
		 *	\brief	Get the kernel argument variable from its semantic.
		 *	\todo	Simulation class change from arguments to constants
		 */
		CLKernelArgument* Argument(const std::string& semantic);

		/*!
		 *	\brief	Get the program constant from its semantic.
		 */
		CLProgramConstant* Constant(const std::string& semantic);

		/*!
		 *	\brief	Set (yet another) semantic that will represent specific variable.
		 */
		bool ConnectSemantic(const std::string& semantic, CLVariable* var, bool autoUpdateKernelsIfNeeded = true);

		/*!
		 *	\brief	Get the amount of used memory in bytes program has allocated on devices.
		 */
		size_t UsedMemorySize();

		/*!
		 *	\brief	Get
		 */
		cl_program& Handle() { return program; }

	private:

		friend class CLVariable;
		friend class CLSubProgram;

		CLLink *link;

      bool madMath, unsafeMath, finiteMath, normalMath;
		std::vector<std::string> buildOptions;
		std::string source;
		bool isBuilt;
		cl_program program;

		std::list<CLVariable*> variablesList;
		std::map<std::string,CLVariable*> variables;
		std::map<std::string,CLGlobalBuffer*> globalBuffers;
		std::map<std::string,CLLocalBuffer*> localBuffers;
		std::map<std::string,CLKernelArgument*> arguments;
		std::map<std::string,CLProgramConstant*> constants;

		std::vector<CLSubProgram*> subprograms;
		
	};

}

#endif
