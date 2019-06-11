#include "bodyforcewriter.h"
#include "isph.h"

#include <sstream>
#include <iomanip>
#include <cfloat>

using namespace isph;

BodyForceWriter::BodyForceWriter(Simulation* simulation)
	: StdWriter(simulation)
	, separation(',')
{
	SetFileExtension("force");
}


BodyForceWriter::~BodyForceWriter()
{
} 


bool BodyForceWriter::Prepare()
{
	if(!StdWriter::Prepare())
		return false;

	// header
	stream << "time";

	// names -> bodies
	for (std::list<std::string>::const_iterator it = bodyNames.begin(); it != bodyNames.end(); ++it)
	{
		Geometry *geo = sim->GetGeometry(*it);
		if(geo)
		{
			if(geo->Type() != BoundaryParticle)
				continue;
			stream << separation << *it << ":X" << separation << *it << ":Y";
			if(sim->Dimensions() == 3)
				stream << separation << *it << ":Z";
			bodies.push_back(geo);
		}
	}

	if(bodies.size() > 1)
	{
		stream << separation << "OVERALL:X" << separation << "OVERALL:Y";
		if(sim->Dimensions() == 3)
			stream << separation << "OVERALL:Z";
	}

	// end header line
	stream << std::endl;

	return true;
}

void BodyForceWriter::PrepareData()
{
	//sim->ParticlePressures()->Download(true);
}

void BodyForceWriter::WriteData()
{
	Vec<3,double> forceSum;

	// start writing row by writing the time
	this->UpdateStats();
	stream << this->LastExportedTime();

	// go through bodies
	for (std::list<Geometry*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
	{
		Geometry *geo = *it;
		Vec<3,double> force;
		
		// go through particles and sum pressures
		for(unsigned int i=0; i<geo->ParticleCount(); ++i)
		{
			Particle p = sim->GetParticle(i + geo->ParticleStartId());
			if(p.Type() == BoundaryParticle)
				force -= p.Normal() * (std::max)(p.Pressure(), 0.0);
		}

		// force_vector = SUM(-normal_vector * pressure * area)
		force *= pow(sim->ParticleSpacing(), (int)sim->Dimensions() - 1);
		forceSum += force;

		stream << separation << force.x << separation << force.y;
		if(sim->Dimensions() == 3)
			stream << separation << force.z;
	}

	if(bodies.size() > 1)
	{
		stream << separation << forceSum.x << separation << forceSum.y;
		if(sim->Dimensions() == 3)
			stream << separation << forceSum.z;
	}

	// end row
	stream << std::endl;
}

void BodyForceWriter::AddBody( const std::string& name )
{
	bodyNames.push_back(name);
}

void BodyForceWriter::SetSeparationChar( char separationChar )
{
	separation = separationChar;
}
