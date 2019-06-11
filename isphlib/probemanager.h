#ifndef PROBEMANAGER_WRITER_H
#define PROBEMANAGER_WRITER_H

#include "particle.h"
#include "stdwriter.h"
#include <string>
#include <list>
#include <vector>

namespace isph {

	class Simulation;


	/*!
	 *	\class	ProbeManager
	 *	\brief	Class to manage probe data.
	 */
	class ProbeManager : public StdWriter
	{
	public:

		ProbeManager(Simulation* simulation);

		~ProbeManager();

		/*!
		 *	\brief	Initializes Probes buffer and file
		 */
		virtual bool Prepare();

		/*!
		 *	\brief	Closes probes file
		 */
		virtual bool Finish();

		/*!
		 *	\brief	Initializes Probes devices variable and kernels
		 */
		void InitKernels();

		/*!
		 *	\brief	Enqueues kernels to average required particle attributes at probes location
		 *          every n times steps as specified by sampling frequency.
		 */
		virtual void PrepareData();
	     
		/*!
		 *	\brief	Writes the read data.
		 */
		virtual void WriteData();

		/*!
		 *	\brief	Add a new probe at specified location.
		 */
		void AddProbe(Vec<3,double> location);

		/*!
		 *	\brief	Add probes along a line defined by two points with specified spacing
		 *          the first probe will be located on the start point the next one 
		 *          at spacing distance from the first along the line.
		 */
		void AddProbesString(Vec<3,double> start, Vec<3,double> end, double spacing);

		/*!
		 *	\brief	Get the total number of probes.
		 */
		inline unsigned int GetProbesCount() { return (unsigned int)locationList.size(); }

		/*!
		 *	\brief	Get the size of the probes buffer.
		 */
		inline unsigned int GetBufferSize() { return bufferSize; }

		/*!
		 *	\brief	A reference to the array representation of locations.
		 */
		inline void* GetProbesLocation() { return locations; }

		/*!
		 *	\brief	Writes the content of the buffer to the output.
		 */
		void WriteBuffer();

		/*!
		 *	\brief	Increase the number of buffered steps.
		 */
		void RecordSamplingTime( double time );

		inline unsigned int RecordedSteps() { return recordedSteps; }

		inline unsigned int BufferingSteps() { return bufferingSteps; }

		/*!
		 *	\brief	Increase the number of buffered steps.
		 */
		inline bool BufferFull() { return ( recordedSteps >= bufferingSteps); };

	private:

		/*!
		 *	\brief	Writes probes file header lines.
		 */
		virtual void WriteHeader();

		unsigned int totalScalarValues;
		unsigned int singleBufferSize;
		unsigned int bufferingSteps;
		unsigned int bufferSize;    
		unsigned int recordedSteps;
		unsigned int recordedValues[2];
		
		union
		{
			void* locations;
			cl_double* locations_d;
			cl_float* locations_f;
		};

		union
		{
			void* times;
			cl_double* times_d;
			cl_float* times_f;
		};

		std::vector< Vec<3,double> > locationList;
		bool initialized;
		bool headerWritten;

	};


} // namespace isph

#endif
