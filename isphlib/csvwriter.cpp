#include "csvwriter.h"
#include "simulation.h"
#include "log.h"
#include "utils.h"
#include "clprogram.h"

using namespace isph;

CsvWriter::CsvWriter(Simulation* simulation)
	: Writer(simulation)
	, separation(',')
{
	SetFileExtension("csv");
}


CsvWriter::~CsvWriter()
{

}


bool CsvWriter::Prepare()
{
	if(!Writer::Prepare())
		return false;

	attributeList.push_front(this->sim->Program()->Buffer("POSITIONS"));

	// prepare header that's always the same
	header.clear();
	for (std::list<CLGlobalBuffer*>::iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
		if(iter != attributeList.begin())
			header += separation;

		if((*iter)->IsScalar())
			header += (*iter)->Semantic();
		else
		{
			header += (*iter)->Semantic() + ":X" + separation + (*iter)->Semantic() + ":Y";
			if(sim->Dimensions() == 3)
				header += separation + (*iter)->Semantic() + ":Z";
		}
	}

	header += '\n';

	return true;
}


void CsvWriter::WriteData()
{
	// one CSV file per export time
	std::string curPath = this->path + '_' + Utils::IntegerString(this->ExportsCount()) + '.' + this->extension;

	this->UpdateStats();

	Log::Send(Log::Info, "Exporting data to: " + curPath + ", for simulation time: " + Utils::DoubleString(this->LastExportedTime()));

	stream.open(curPath.c_str());

	if(!stream.is_open())
	{
		Log::Send(Log::Error, "Couldn't open new CSV export file: " + curPath);
		return;
	}

	stream.setf(std::ios::scientific);
    
	// write header
    stream << header;

	// write data
	std::list<CLGlobalBuffer*>::iterator iter;
	bool z_coord = (this->sim->Dimensions() == 3);
	for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
	{
		for (iter = attributeList.begin(); iter != attributeList.end(); ++iter)
		{
			if(iter != attributeList.begin())
				stream << separation;

			CLGlobalBuffer* att = (*iter);

			if(att->IsScalar())
			{
				stream << att->GetScalar(i);
			}
			else
			{
				Vec<3,double> v = att->GetVector(i);
				stream << v.x << separation << v.y;
				if(z_coord)
					stream << separation << v.z;
			}
		}

		stream << std::endl;
	}

	// close file
	if(stream.is_open())
		stream.close();
}

void CsvWriter::SetSeparationChar( char separationChar )
{
	separation = separationChar;
}
