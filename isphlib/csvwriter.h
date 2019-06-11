#ifndef ISPH_CSVWRITER_H
#define ISPH_CSVWRITER_H

#include "writer.h"
#include <fstream>

namespace isph {

	/*!
	 *	\class	CsvWriter
	 *	\brief	Exporting simulated data to coma-separated values.
	 */
	class CsvWriter : public Writer
	{
	public:

		CsvWriter(Simulation* simulation);

		virtual ~CsvWriter();

		void SetSeparationChar(char separationChar);

		virtual bool Prepare();

		virtual void WriteData();

	protected:

		std::string header;
		char separation;
		std::ofstream stream;

	};

}

#endif
