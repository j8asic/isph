#include "stdwriter.h"

using namespace isph;

StdWriter::StdWriter( Simulation* simulation ) : Writer(simulation)
{
	binary = false;
}

StdWriter::~StdWriter()
{
	Finish();
}

bool StdWriter::Prepare()
{
	if(!Writer::Prepare())
		return false;

	std::ios_base::open_mode openMode = std::ios::out | std::ios::trunc;
	if(binary)
		openMode |= std::ios::binary;

	stream.open((this->path + '.' + this->extension).c_str());

	return stream.is_open();
}

bool StdWriter::Finish()
{
	if(stream.is_open())
		stream.close();
	return true;
}
