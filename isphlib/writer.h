#ifndef ISPH_WRITER_H
#define ISPH_WRITER_H

#include <string>
#include <list>
#include "particle.h"
#include "extern/tinythread/tinythread.h"

namespace isph {

	class Simulation;


	/*!
	 *	\class	Writer
	 *	\brief	Abstract class for exporting simulated data.
	 */
	class Writer
	{
	public:

		Writer(Simulation* simulation);

		virtual ~Writer();

		/*!
		 *	\brief	Get the simulation object which exporter belongs to.
		 */
		inline Simulation* ParentSimulation() { return sim; }

		/*!
		 *	\brief	Set the output file path.
		 *	\param	outputPath	File path without extension.
		 */
		void SetOutput(const std::string& outputPath);

		/*!
		 *	\brief	Set the output file extension.
		 */
		void SetFileExtension(const std::string& ext);

		/*!
		 *	\brief	Add the particle attribute you want to export;
		 */
		virtual void AddAttribute(const std::string& attName);

		/*!
		 *	\brief	Set time for which to export simulation data.
		 *
		 *	If you don't want to depend on constant export time step or specified time step
		 *	doesn't include time for which you need the results; use this function to specify
		 *	list of times.
		 */
		void AddExportTime(double time);

		/*!
		 *	\brief	Simulated data gets written after each time step passes.
		 *	\param	timeStep	Export time step in seconds. Leave as zero for no export (you can manually manage it).
		 */
		inline void SetExportTimeStep(double timeStep) { exportTimeStep = timeStep; }

		/*!
		 *	\brief	Get export time step.
		 */
		inline double ExportTimeStep() { return exportTimeStep; }

		/*!
		*	\brief	When was last time data exported.
		*/
		inline double LastExportedTime() { return lastExportedTime; }

		/*!
		 *	\brief	How many times was simulated data exported.
		 */
		inline unsigned int ExportsCount() { return exportedTimeStepsCount + exportedTimesCount; }

		/*!
		 *	\brief	Finish writing the file.
		 *
		 *	Override the function to close the output file after writing. It can also be used for writing recorded
		 *	particle properties. This means that if you do not want time-based, but particle-based exported
		 *	file for example, with WriteData you can track particle, and with this function you can
		 *	write all recorded data.
		 */
		virtual bool Finish() { return true; }

		/*!
		 *	\brief	Prepare data-set to write for current time (download, change endian, etc).
		 */
		virtual void PrepareData();

		/*!
		 *	\brief	Write simulated data-set for current time.
		 */
		virtual void WriteData() = 0;

	protected:

		/*!
		 *	\brief	Prepare the file for writing.
		 *
		 *	Override the function to create the output file and prepare it for writing.
		 */
		virtual bool Prepare();

		/*!
		 *	\brief	Update exported steps count and exported time.
		 *
		 *	Should be called internally at the beginning of WriteData.
		 */
		void UpdateStats();

		friend class Simulation;

		std::string path;
		std::string extension;
		Simulation *sim;

		// stats
		unsigned int exportedTimeStepsCount;
		unsigned int exportedTimesCount;
		double lastExportedTime;

		// attributes
		std::list<std::string> attributeNameList;
		std::list<CLGlobalBuffer*> attributeList;

		// for auto managed export
		std::list<double> exportTimes;
		double exportTimeStep;
		tthread::thread *enqueuedThread;
	};


} // namespace isph

#endif
