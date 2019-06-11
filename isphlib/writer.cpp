#include "isph.h"
#include <cctype>
#include <clocale>
#include <cfloat>
using namespace isph;

Writer::Writer( Simulation* simulation )
	: sim(simulation)
	, exportedTimeStepsCount(0)
	, exportedTimesCount(0)
	, lastExportedTime(0)
	, exportTimeStep(0)
	, enqueuedThread(NULL)
{
	if(sim)
	{
		// add this exporter to active simulation exporters list
		sim->exporters.push_back(this);

		// set default output path
		std::string name = sim->Name();
		if(name.empty())
			path = "results";
		else
		{
			for (size_t i=0; i<name.size(); i++)
			{
				if(isspace(name[i]) || ispunct(name[i]))
					path += '_';
				else if (isalnum(name[i]))
					path += name[i];
			}
		}
	}

	setlocale(LC_ALL, "C");
}

Writer::~Writer()
{
	Finish();

	/*if(sim)
		sim->exporters.remove(this); */
}

void Writer::SetOutput( const std::string& outputPath )
{
	path = outputPath;
}

void Writer::UpdateStats()
{
	if(!sim)
		return;

	lastExportedTime = sim->Time();

	if(!exportTimes.empty() && lastExportedTime >= exportTimes.front() - 1e-9)
	{
		exportTimes.pop_front();
		exportedTimesCount++;

		if((abs(exportTimeStep) > DBL_EPSILON && lastExportedTime >= exportTimeStep * exportedTimeStepsCount - 1e-9))
			exportedTimeStepsCount++;
	}
	else
		exportedTimeStepsCount++;
}

bool Writer::Prepare()
{
	// semantic names -> real atribute buffers
	for (std::list<std::string>::const_iterator iter = attributeNameList.begin(); iter != attributeNameList.end(); ++iter)
	{
		CLGlobalBuffer* att = this->sim->Program()->Buffer(*iter);
		if(att)
			attributeList.push_back(att);
		else
			Log::Send(Log::Warning, "Couldn't find to export: " + *iter);
	}

	// sort times
	exportTimes.sort();

	return true;
}

void Writer::PrepareData()
{
	for (std::list<CLGlobalBuffer*>::iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
		(*iter)->Download();
	}
}

void Writer::SetFileExtension( const std::string& ext )
{
	extension = ext;
}

void Writer::AddAttribute( const std::string& attName )
{
	attributeNameList.push_back(attName);
}

void Writer::AddExportTime( double time )
{
	exportTimes.push_back(time);
}
