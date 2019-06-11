#ifndef ISPH_PARTICLE_H
#define ISPH_PARTICLE_H

#include "vec.h"
#include "clglobalbuffer.h"
#include <cstring>
#include <string>

namespace isph {

	class Simulation;


	/*!
	 *	\enum	ParticleType
	 *	\brief	Enumeration of every type of particle.
	 */
	enum ParticleType
	{
		FluidParticle,		//!< Particle that's part of fluid
		BoundaryParticle,	//!< Particle that's part of rigid structure
		DummyParticle,		//!< Ghost particle behind rigid boundary
		ParticleTypeCount	//!< Counts how many particle types exist
	};


	/*!
	 *	\class	Particle
	 *	\brief	Particle class.
	 *	\todo	inline functions
	 *
	 *	This class serves as a particle properties communicator, so
	 *	calling destructor will not delete particle itself.
	 */
	class Particle
	{
	public:

		/*!
		 *	\brief	Constructor.
		 */
		Particle(Simulation* simulation, unsigned int index);

		/*!
		 *	\brief	Destructor.
		 *
		 *	Calling destructor will not delete particle itself,
		 *	only this object that serves as a particle properties communicator.
		 */
		~Particle();

		/*!
		 *	\brief	Get the classification/type of the particle.
		 */
		ParticleType Type();

		/*!
		 *	\brief	Set the classification/type of the particle.
		 */
		void SetType(ParticleType type);

		/*!
		 *	\brief	Get the particle mass.
		 */
		double Mass();

		/*!
		 *	\brief	Get the particle density.
		 */
		double Density();

		/*!
		 *	\brief	Get the particle pressure.
		 */
		double Pressure();

		/*!
		 *	\brief	Get the particle current position vector.
		 */
		Vec<3,double> Position();

		/*!
		 *	\brief	Get the particle current velocity vector.
		 */
		Vec<3,double> Velocity();

		/*!
		 *	\brief	Get the solid particle normal.
		 */
		Vec<3,double> Normal();

		/*!
		 *	\brief	Set the particle mass.
		 */
		void SetMass(double mass);

		/*!
		 *	\brief	Set the particle density.
		 */
		void SetDensity(double density);

		/*!
		 *	\brief	Set the particle pressure.
		 *
		 *	Pressure is value that's calculated from other values and boundary conditions,
		 *	thus manually setting it serves only for cosmetic purposes.
		 */
		void SetPressure(double pressure);

		/*!
		 *	\brief	Set the particle position vector.
		 */
		void SetPosition(const Vec<3,double>& position);

		/*!
		 *	\brief	Set the particle velocity vector.
		 */
		void SetVelocity(const Vec<3,double>& velocity);

		/*!
		 *	\brief	Set the particle normal vector.
		 */
		void SetNormal(const Vec<3,double>& normal);

	protected:

		unsigned int id;
		Simulation *sim;

	};

} // namespace isph

#endif
