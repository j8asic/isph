#include "particle.h"
#include "simulation.h"
#include "utils.h"
#include "log.h"
#include <cstring>
#include <cfloat>
#include <climits>

using namespace std;
using namespace isph;

Particle::Particle(Simulation* simulation, unsigned int index)
   : id(index)
   , sim(simulation)
{
}

Particle::~Particle()
{
}

void Particle::SetType( ParticleType type )
{
	sim->classBuffer->SetScalar(id, (unsigned int)type);
}

ParticleType Particle::Type()
{
	int typ = (int)sim->classBuffer->GetScalar(id);
	return (ParticleType)typ;
}

double Particle::Mass()
{
	return sim->massesBuffer->GetScalar(id);
}

double Particle::Density()
{
	return sim->densitiesBuffer->GetScalar(id);
}

double Particle::Pressure()
{
	return sim->pressuresBuffer->GetScalar(id);
}

Vec<3,double> Particle::Position()
{
	return sim->positionsBuffer->GetVector(id);
}

Vec<3,double> Particle::Velocity()
{
	return sim->velocitiesBuffer->GetVector(id);
}

Vec<3,double> Particle::Normal()
{
	return sim->normalsBuffer->GetVector(id);
}

void Particle::SetMass( double mass )
{
	sim->massesBuffer->SetScalar(id, mass);
}

void Particle::SetDensity( double density )
{
	sim->densitiesBuffer->SetScalar(id, density);
}

void Particle::SetPressure( double pressure )
{
	sim->pressuresBuffer->SetScalar(id, pressure);
}

void Particle::SetPosition( const Vec<3,double>& position )
{
	sim->positionsBuffer->SetVector(id, position);
}

void Particle::SetVelocity( const Vec<3,double>& velocity )
{
	sim->velocitiesBuffer->SetVector(id, velocity);
}

void Particle::SetNormal( const Vec<3,double>& normal )
{
	sim->normalsBuffer->SetVector(id, normal);
}
