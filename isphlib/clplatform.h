#ifndef ISPH_CLPLATFORM_H
#define ISPH_CLPLATFORM_H

#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace isph
{
	class CLSystem;
	class CLDevice;
	class CLProgram;

	/*!
	 *	\class	CLPlatform
	 *	\brief	Platform that has OpenCL enabled devices.
	 */
	class CLPlatform
	{
	public:

		~CLPlatform();

		/*!
		 *	\brief	Get the OpenCL ID of the platform.
		 */
      cl_platform_id ID() { return id; }

		/*!
		 *	\brief	Get the name of the platform.
		 */
      const std::string& Name() { return name; }

		/*!
		 *	\brief	Get the vendor of the platform.
		 */
      const std::string& Vendor() { return vendor; }

		/*!
		 *	\brief	Get the OpenCL version.
		 */
      const std::string& CLVersion() { return clVersion; }

		/*!
		 *	\brief	Get the number of OpenCL enabled devices in platform.
		 */
      unsigned int DeviceCount() { return deviceCount; }

		/*!
		 *	\brief	Get one of the devices in the platform.
		 */
      CLDevice* Device(unsigned int i) { return devices[i]; }

		/*!
		 *	\brief	Get the best device of specified type on platform.
		 */
		CLDevice* BestDevice(cl_device_type type = CL_DEVICE_TYPE_ALL);

		/*!
		 *	\brief	Get vector of devices of specified type on platform.
		 */
		std::vector<CLDevice*> Devices(cl_device_type type = CL_DEVICE_TYPE_ALL);

	private:
		friend class CLSystem;

      /*!
       *	\brief	Called only by CLSystem.
       */
      CLPlatform(cl_platform_id platform);

		cl_platform_id id;
		std::string name;
		std::string vendor;
		std::string clVersion;

		cl_uint deviceCount;
		CLDevice** devices;
	};
}

#endif
