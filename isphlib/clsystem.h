#ifndef ISPH_CLSYSTEM_H
#define ISPH_CLSYSTEM_H

#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace isph
{
	class CLPlatform;
	class CLDevice;
	class CLProgram;

	/*!
	 *	\enum	VariableDataType
	 *	\brief	Different data types.
	 */
	enum VariableDataType
	{
		FloatType,		//!< 32bit floating precision scalar
		Float2Type,		//!< 32bit 2D floating precision vector
		Float4Type,		//!< 32bit 4D floating precision vector
		Float8Type,		//!< 32bit 8-component floating precision vector
		DoubleType,		//!< 64bit floating precision scalar
		Double2Type,	//!< 64bit 2D floating precision vector
		Double4Type,	//!< 64bit 4D floating precision vector
		Double8Type,	//!< 32bit 8-component floating precision vector
		UintType,		//!< 32bit positive integer
		Uint2Type,		//!< 32bit 2D positive integer vector
		Uint4Type,		//!< 32bit 4D positive integer vector
		IntType,		//!< 32bit integer
		Int2Type,		//!< 32bit 2D integer vector
		Int4Type,		//!< 32bit 4D integer vector
		CharType,		//!< 8bit integer
		UCharType		//!< 8bit positive integer
	};

	/*!
	 *	\class	CLSystem
	 *	\brief	System of OpenCL enabled platforms (singleton class).
	 */
	class CLSystem
	{
	public:
		CLSystem();
		~CLSystem();

		/*!
		 *	\brief	Get the one and only instance of this class.
		 */
		static CLSystem* Instance();

		/*!
		 *	\brief	Get the number of OpenCL enabled platforms.
		 */
		inline unsigned int PlatformCount() { return platformCount; }

		/*!
		 *	\brief	Get one of the platforms in the system.
		 */
		inline CLPlatform* Platform(unsigned int i) { return platforms[i]; }

		/*!
		 *	\brief	Get the first available OpenCL supported platform.
		 */
		inline CLPlatform* FirstPlatform() { return platformCount ? platforms[0] : NULL; }

		/*!
		 *	\brief	Get devices list by its type and/or name, and/or how many of them.
		 *	\param	type	Find devices by type.
		 *	\param	name	Find devices that contain (case insensitive) name. Empty string to ignore.
		 */
		std::vector<CLDevice*> FilterDevices(cl_device_type type = CL_DEVICE_TYPE_ALL, std::string name = std::string());

		/*!
		 *	\brief	Get the best device in specified device list. Handy to use it with FilterDevices.
		 */
		CLDevice* BestDevice(const std::vector<CLDevice*>& devices);

		/*!
		 *	\brief	Get description of OpenCL runtime error.
		 */
		std::string ErrorDesc(cl_int status);

		/*!
		 *	\brief	Get OpenCL data type size.
		 */
		size_t DataTypeSize(VariableDataType type);

		/*!
		 *	\brief	Get OpenCL data type name as a string.
		 */
		std::string DataTypeString(VariableDataType type);

		/*!
		 *	\brief	Enable profiling of OpenCL code. Default is false.
		 */
		inline void SetProfiling(bool enabled) { profiling = enabled; }

		/*!
		 *	\brief	Is profiling of OpenCL code enabled.
		 */
		inline bool Profiling() { return profiling; }

	private:

		friend class CLProgram;

		/*!
		 *	\brief	Initialize OpenCL system.
		 */
		void Initialize();

		static CLSystem* instance;

		cl_uint platformCount;
		CLPlatform** platforms;
		bool profiling;

	};
}

#endif
