#ifndef BODY_FORCE_WRITER_H
#define BODY_FORCE_WRITER_H

#include "particle.h"
#include "stdwriter.h"
#include "geometry.h"
#include <string>
#include <list>

namespace isph {

	class Simulation;


	/*!
	 *	\class	BodyForceWriter
	 *	\brief	Class to integrate pressure over body and write it to file.
	 */
	class BodyForceWriter : public StdWriter
	{
	public:

		BodyForceWriter(Simulation* simulation);

		~BodyForceWriter();

		virtual bool Prepare();

		virtual void PrepareData();
	     
		virtual void WriteData();

		/// \todo add directly by geo pointer
		void AddBody(const std::string& name);

		void SetSeparationChar(char separationChar);

	private:

		char separation;
		std::list<std::string> bodyNames;
		std::list<Geometry*> bodies;

	};


} // namespace isph

#endif
