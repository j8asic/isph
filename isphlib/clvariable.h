#ifndef ISPH_CLVARIABLE_H
#define ISPH_CLVARIABLE_H

#include <string>
#include <list>
#include "vec.h"
#include "clsystem.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace isph
{
	class CLProgram;
	class CLSubProgram;

	/*!
	 *	\enum	VariableType
	 *	\brief	Different types of OpenCL variables.
	 */
	enum VariableType
	{
		ProgramConstant,	//!< Constant OpenCL variable type
		KernelArgument,		//!< OpenCL variable type that can be used as kernel argument
		GlobalBuffer,		//!< OpenCL global buffer
		LocalBuffer			//!< OpenCL local buffer
	};

	/*!
	 *	\class	CLVariable
	 *	\brief	OpenCL variable (buffer, scalar, etc); used across CLProgram, manipulated in CLSubProgram.
	 */
	class CLVariable
	{
	public:

		/*!
		 *	\brief	Create variable to be used in CLProgram.
		 */
		CLVariable(CLProgram* program, const std::string& semantic, VariableType type);

		/*!
		 *	\brief	Class destructor.
		 */
		virtual ~CLVariable();

		/*!
		 *	\brief	Get the type of the variable.
		 */
		inline VariableType Type() { return varType; }

		/*!
		 *	\brief	Get the semantic associated to the variable.
		 */
		inline const std::string& Semantic() { return semantics.front(); }

		/*!
		 *	\brief	Set variable memory space in bytes.
		 *	\param	elements	Number of elements to allocate. 
		 *	\param	dataType	Type of data.
		 */
		void SetSpace(VariableDataType dataType, unsigned int elements);

		/*!
		 *	\brief	Get the data type of the variable.
		 */
		inline VariableDataType DataType() { return varDataType; }

		/*!
		 *	\brief	Get if the variable is scalar type.
		 */
		bool IsScalar();

		/*!
		 *	\brief	Get the size of variable data type in bytes.
		 */
		size_t DataTypeSize();

		/*!
		 *	\brief	Get the number of elements.
		 */
		inline size_t Elements() { return elementCount; }

		/*!
		 *	\brief	Get the allocated memory: ElementCount()*TypeSize().
		 */
		inline size_t MemorySize() { return memorySize; }

		/*!
		 *	\brief	Assign scalar to a variable.
		 */
		virtual bool SetScalar(unsigned int id, double var);
		bool SetScalar(double var) { return SetScalar(0, var); }

		/*!
		 *	\brief	Assign vector to a variable.
		 */
		virtual bool SetVector(unsigned int id, Vec<3,double> var);
		bool SetVector(Vec<3,double> var) { return SetVector(0, var); }

		/*!
		 *	\brief	Read variable as a scalar.
		 */
		virtual double GetScalar(unsigned int id = 0);

		/*!
		 *	\brief	Read variable as a vector.
		 */
		virtual Vec<3,double> GetVector(unsigned int id = 0);

	protected:

		/*!
		 *	\brief	Allocate on host/device data needed for variable.
		 */
		virtual bool Allocate() = 0;

		/*!
		 *	\brief	Release from host/device variable data.
		 */
		virtual void Release() = 0;

		/*!
		 *	\brief	Pass the OpenCL variable to a kernel.
		 */
		virtual bool SetAsArgument(CLSubProgram *kernel, unsigned int argID, unsigned int deviceID = 0, size_t kernelLocalSize = 0) = 0;
		
		/*!
		 *	\brief	Get the number of elements for device.
		 *	\todo	Better solve for inheriting classes. Only global buffers need it?
		 */
		virtual size_t ElementCount(unsigned int deviceID) = 0;

		/*!
		 *	\brief	Get pointer on host for the specific element of array 
		 */
		void* Get(unsigned int id);

		friend class CLProgram;
		friend class CLSubProgram;

		CLProgram *parentProgram;
		
		// general data
		char* data;
		VariableType varType;
		VariableDataType varDataType;
		size_t elementCount;
		size_t memorySize;
		bool needsUpdate;
		std::list<std::string> semantics;
	};

}

#endif
