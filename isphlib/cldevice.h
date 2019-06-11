#ifndef ISPH_CLDEVICE_H
#define ISPH_CLDEVICE_H

#include <string>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace isph
{
	class CLSystem;
	class CLPlatform;

	/*!
	 *	\class	CLDevice
	 *	\brief	OpenCL enabled device.
	 */
	class CLDevice
	{
	public:

      ~CLDevice() {}

		/*!
		 *	\brief	Get OpenCL platform this device belongs to.
		 */
      CLPlatform* Platform() { return platform; }

		/*!
		 *	\brief	Get the device type (CPU, GPU, etc).
		 */
      cl_device_type Type() { return type; }

		/*!
		 *	\brief	Check is it CPU device.
		 */
      bool IsCPU() { return type == CL_DEVICE_TYPE_CPU; }

		/*!
		 *	\brief	Check is it GPU device.
		 */
      bool IsGPU() { return type == CL_DEVICE_TYPE_GPU; }

		/*!
		 *	\brief	Get the OpenCL ID of the device.
		 */
      const cl_device_id& ID() { return id; }

		/*!
		 *	\brief	Get the name of the device.
		 */
      const std::string& Name() { return name; }

		/*!
		 *	\brief	Get the vendor name of the device.
		 */
      const std::string& Vendor() { return vendor; }

		/*!
		 *	\brief	Get the number of compute units.
		 */
      unsigned int ComputeUnits() { return (unsigned int)maxComputeUnits; }

		/*!
		 *	\brief	Get the maximum clock frequency of device.
		 */
      unsigned int MaxFrequency() { return (unsigned int)maxClock; }

		/*!
		 *	\brief	Check if device supports double precision.
		 */
      bool DoublePrecision() { return fp64; }

		/*!
		 *	\brief	Check if device supports half precision.
		 */
      bool HalfPrecision() { return fp16; }

		/*!
		 *	\brief	Check if device supports atomic functions.
		 */
      bool Atomics() { return globalAtomics && localAtomics; }

		/*!
		 *	\brief	Check if device supports atomic functions in global memory.
		 */
      bool GlobalAtomics() { return globalAtomics; }

		/*!
		 *	\brief	Check if device supports atomic functions in local memory.
		 */
      bool LocalAtomics() { return localAtomics; }

		/*!
		 *	\brief	Get the size in bytes of global memory that device has.
		 */
      unsigned int GlobalMemorySize() { return (unsigned int)globalMemSize; }

		/*!
		 *	\brief	Get the size in bytes of local memory that device has.
		 */
      unsigned int LocalMemorySize() { return (unsigned int)localMemSize; }

		/*!
		 *	\brief	Get the performance index of the device.
		 */
      unsigned int PerformanceIndex() { return (unsigned int)performanceIndex; }

		/*!
		 *	\brief	Get device maximum work-group size.
		 */
      unsigned int MaxWorkGroupSize() { return (unsigned int)maxWorkGroupSize; }

		/*!
		 *	\brief	Get device maximum work-item size for a dimension.
		 */
      unsigned int MaxWorkItemSize(unsigned int dimension) { return (unsigned int)maxWorkItemSize[dimension]; }

		/*!
		 *	\brief	Get maximum size that can be allocated on the device.
		 */
      unsigned int MaxAllocationSize() { return (unsigned int)maxAllocSize; }

	private:
		friend class CLSystem;

      /*!
       *	\brief	Called only by CLSystem.
       */
      CLDevice(CLPlatform* parentPlatform, cl_device_id device) : platform(parentPlatform), id(device) {}

		CLPlatform* platform;
		cl_device_id id;
      cl_device_type type = CL_DEVICE_TYPE_DEFAULT;
		std::string name;
		std::string vendor;
      cl_uint maxComputeUnits = 0;
      cl_uint maxClock = 0;
      cl_uint performanceIndex = 0;
      size_t maxWorkGroupSize = 0;
		size_t maxWorkItemSize[3];
      cl_ulong globalMemSize = 0;
      cl_ulong localMemSize = 0;
      cl_ulong maxAllocSize = 0;
      bool fp16 = false;
      bool fp64 = false;
      bool globalAtomics = false;
      bool localAtomics = false;
	};

}

#endif
