#ifndef ISPH_GEOMETRY_H
#define ISPH_GEOMETRY_H

#include <string>
#include <map>
#include <vector>
#include "particle.h"

namespace isph 
{

	/*!
	 *	\class	Geometry
	 *	\brief	2D/3D model finally converted to particles.
	 */
	class Geometry
	{		
	public:
		Geometry(Simulation* parentSimulation, ParticleType particleType);
		Geometry(Simulation* parentSimulation, ParticleType particleType, std::string name);
		virtual ~Geometry();

		inline unsigned int Id() { return objectId; }

		inline const std::string& Name() { return name; }

		/*!
		 *	\brief	Get the number of particles that represent the geometry.
		 */
		unsigned int ParticleCount();

		/*!
		 *	\brief	Get the ID of the first particle.
		 */
		inline unsigned int ParticleStartId() { return startId; }

		/*!
		 *	\brief	Get the type of particles that represent the geometry.
		 */
		inline ParticleType Type() { return type; }

		/*!
		 *	\brief	Set object's velocity.
		 */
		void SetVelocity(Vec<3,double> velocity);

		/*!
		 *	\brief	Add attribute initialization analytic expression. Must be called before simulation init.
		 *	\remarks	Must be called before simulation init.
		 */
		void SetInitExpression(std::string attribute, std::vector<std::string> expression);

		/*!
		 *	\brief	Set object movement analytic expression. Must be called before simulation init.
		 *	\param	position	Expression defining object position based on simulation time.
		 *	\param	velocity	Expression defining object velocity based on simulation time.
		 *	\remarks	Must be called before simulation init.
		 */
		void SetMovementExpression(std::vector<std::string> position, std::vector<std::string> velocity, double startTime=0, double endTime=0);

		struct Corner 
		{
			Vec<3,double> position, normal, tangent;
		};

		virtual std::vector<Corner> Corners() { return std::vector<Corner>(); }

	protected:

		friend class Simulation;

		/*!
		 *	\brief	Get OpenCL run-time made expression init code.
		 */
		std::string GetInitCode();

		inline bool ExpressionInit() { return !initializationExp.empty(); }

		/*!
		 *	\brief	Get OpenCL run-time made expression movement code.
		 */
		std::string GetMovementCode();

		/*!
		 *	\brief	Does it have movement code.
		 */
		inline bool ExpressionMovement() { return !velocityExp.empty(); }

		/*!
		 *	\brief	Build geometry with particles.
		 */
		virtual void Build(bool onlyCountParticles) = 0;

		/*!
		 *	\brief	Init wall particle with its dummies.
		 */
		void InitParticle(const Vec<3,double>& pos, const Vec<3,double>& normal, bool onlyCountParticles, bool isCorner);

		/*!
		*	\brief	Check if particle intersects geometry.
		*/
      virtual bool ParticleCollision(const Vec<3,double>& /*position*/, double /*radius*/) { return false; }


		std::string name;
		unsigned int objectId;
		unsigned int startId;
		unsigned int particleCount;
		Simulation* sim;
		ParticleType type;

		// init + movement
		std::map<std::string, std::vector<std::string> > initializationExp;
		std::vector<std::string> positionExp;
		std::vector<std::string> velocityExp;
		double startMovementTime;
		double endMovementTime;

		static unsigned int objectCount;
	};


	/*!
	 *	\namespace	isph::geo
	 *	\brief		Library of geometric shapes.
	 */
	namespace geo
	{
		/*!
		 *	\class	Line
		 *	\brief	Line of particles between two points.
		 */
		class Line : public Geometry
		{
		public:
			using Geometry::sim;
			using Geometry::particleCount;
			Line(Simulation* parentSimulation, ParticleType particleType) : Geometry(parentSimulation,particleType) {}
			Line(Simulation* parentSimulation, ParticleType particleType, std::string name) : Geometry(parentSimulation, particleType, name) {}
			virtual ~Line() {}
			void Define(Vec<2,double> start, Vec<2,double> end, bool normalStartEndLeft);

		protected:
			virtual bool ParticleCollision(const Vec<3,double>& position, double radius);
			virtual void Build(bool onlyCountParticles);
			virtual std::vector<Corner> Corners();
			Vec<2,double> startPoint, endPoint, normal;

		};


		/*!
		*	\class	Box
		*	\brief	Boundary 3D box.
		 */
		class Box : public Geometry
		{
		public:
			enum Faces {
			   E = 0x01,
			   W = 0x02,
			   N = 0x4,
			   S = 0x8,
			   U = 0x10,
			   D = 0x20
			};
			using Geometry::sim;
			using Geometry::particleCount;
			Box(Simulation* parentSimulation, ParticleType particleType) : Geometry(parentSimulation,particleType), facesFlag(E|W|N|S|U|D) {}
			Box(Simulation* parentSimulation, ParticleType particleType, std::string name) : Geometry(parentSimulation,particleType, name), facesFlag(E|W|N|S|U|D) {}
			virtual ~Box() {}
			inline void Define(const Vec<3,double>& origin, const Vec<3,double>& east,const Vec<3,double>& north,const Vec<3,double>& up) { originPoint=origin; eastVector=east; northVector=north; upVector=up; facesFlag = E|W|N|S|U|D;}
			inline void Define(const Vec<3,double>& origin, const Vec<3,double>& east,const Vec<3,double>& north,const Vec<3,double>& up, const unsigned char faces) { originPoint=origin; eastVector=east; northVector=north; upVector=up; facesFlag = faces;}
		protected:
			virtual unsigned int CreateFace(Vec<3,double> origin,Vec<3,double> edge1,Vec<3,double> edge2, bool initParticles, bool interiorOnly, unsigned int startParticleIndex);
			virtual unsigned int CreateEdge(Vec<3,double> origin,Vec<3,double> edge,bool initParticles, bool interiorOnly,unsigned int startParticleIndex);
			virtual unsigned int CreateSimple(Vec<3,double> originPoint,Vec<3,double>  eastVector,Vec<3,double>  northVector,Vec<3,double>  upVector, unsigned int startParticleIndex, bool initParticles);
			virtual unsigned int Create(bool initParticles);
			virtual void Build(bool onlyCountParticles);
			Vec<3,double> originPoint, eastVector, northVector, upVector;
			unsigned char facesFlag;
		};

    	/*!
		 *	\class	Circle
		 *	\brief	Filled or only boundary circle.
		 */
		class Circle: public Geometry
		{
		public:
			using Geometry::sim;
			using Geometry::particleCount;
			Circle(Simulation* parentSimulation, ParticleType particleType) : Geometry(parentSimulation,particleType) {}
			Circle(Simulation* parentSimulation, ParticleType particleType, std::string name) : Geometry(parentSimulation,particleType, name) {}
			virtual ~Circle() {}
			inline void Define(const Vec<2,double>& centerPoint, double radiusDist, bool outwardNormal) { center=centerPoint; radius=radiusDist; normalOut = outwardNormal; }
		protected:
			virtual bool ParticleCollision(const Vec<3,double>& position, double radius);
			virtual void Build(bool onlyCountParticles);
			Vec<2,double> center;
			double radius;
			bool normalOut;
		};


		/*!
		 *	\class	Sphere
		 *	\brief	Filled or only boundary 3D sphere.
		 */
		class Sphere : public Geometry
		{
		public:
			using Geometry::sim;
			using Geometry::particleCount;
			Sphere(Simulation* parentSimulation, ParticleType particleType) : Geometry(parentSimulation,particleType), filled(false) {}
			Sphere(Simulation* parentSimulation, ParticleType particleType, std::string name) : Geometry(parentSimulation,particleType, name), filled(false) {}
			virtual ~Sphere() {}
			inline void Define(const Vec<3,double>& centerPoint, double radiusDist) { center=centerPoint; radius=radiusDist; }
			inline void Fill() { filled = true; }
		protected:
			virtual void Build(bool onlyCountParticles);
			Vec<3,double> center;
			double radius, realSpacing;
			unsigned int slices;
			bool filled;
		};


		/*!
		 *	\class	Patch
		 *	\brief	2D Patch in 3D spacer.
		 */
		class Patch : public Geometry
		{
		public:
			using Geometry::sim;
			using Geometry::particleCount;
			Patch (Simulation* parentSimulation, ParticleType particleType) : Geometry(parentSimulation,particleType){}
			Patch (Simulation* parentSimulation, ParticleType particleType, std::string name) : Geometry(parentSimulation,particleType, name) {}
			virtual ~Patch () {}

			inline void Define(Vec<3,double> start, Vec<3,double> end1, Vec<3,double> end2, unsigned int rowsOfParticles) { startPoint=start; endPoint1=end1; endPoint2=end2; width = rowsOfParticles;}
		protected:
            virtual unsigned int Create(bool initParticles);
			virtual void Build(bool onlyCountParticles);

			Vec<3,double> startPoint, endPoint1, endPoint2;
			unsigned int width;
			double layerSpacing;
		};

	}
}

#endif
