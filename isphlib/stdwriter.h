#ifndef ISPH_STDWRITER_H
#define ISPH_STDWRITER_H

#include "writer.h"
#include <fstream>

namespace isph {


/*!
 *	\class	StdWriter
 *	\brief	Abstract class for exporting simulated data to a file with STL IO.
 */
class StdWriter : public Writer
{
public:
	
	StdWriter(Simulation* simulation);

	virtual ~StdWriter();

	virtual bool Prepare();

	virtual bool Finish();

	virtual void PrepareData() = 0;

	virtual void WriteData() = 0;

protected:

	bool binary;
	std::ofstream stream;

};


} // namespace isph

#endif
