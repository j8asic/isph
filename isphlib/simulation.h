#ifndef ISPH_SIMULATION_H
#define ISPH_SIMULATION_H

#include <string>
#include <map>
#include <list>
#include "particle.h"
#include "geometry.h"
#include "extern/tinythread/tinythread.h"
#include "timer.h"

class clppContext;
class clppSort;

/*!
 *	\namespace	isph
 *	\brief		Main library namespace.
 */
namespace isph {

	class CLLink;
	class Writer;

	/*!
	 *	\enum	SmoothingKernelType
	 *	\brief	Different smoothing kernel types.
	 */
	enum SmoothingKernelType
	{
		GaussKernel,		//!< Gauss' (exponential based) smoothing kernel.
		QuadraticKernel,	//!< Quadratic (2nd order) smoothing kernel with lower accuracy but without tensile instability.
		CubicSplineKernel,	//!< Cubic (3rd order) spline smoothing kernel, mostly used.
		QuinticSplineKernel,//!< Quintic (5th order) spline smoothing kernel.
		WendlandKernel,		//!< Wendland's quintic (5th order with good performance) smoothing kernel.
		ModifiedGaussKernel	//!< Modified Gauss' (exponential based) smoothing kernel takes into account compact support.
	};


	/*!
	 *	\class	Simulation
	 *	\brief	Abstract class for different SPH kinds of simulation.
	 */
	class Simulation
	{
	public:

		/*!
		 *	\brief	Constructor.
		 *	\param	simDimensions	Number of space dimensions: 2 or 3.
		 *	\param	scalarDataType	Type representing real numbers (affects accuracy and performance): FloatType or DoubleType.
		 */
		Simulation(unsigned int simDimensions, VariableDataType scalarDataType);

		/*!
		 *	\brief	Destructor.
		 */
		virtual ~Simulation();

		/*!
		 *	\brief	Set simulation name.
		 */
		void SetName(const std::string& simName);

		/*!
		 *	\brief	Get simulation name.
		 */
		const std::string& Name() { return name; }

		/*!
		 *	\brief	Set simulation description.
		 */
		void SetDescription(const std::string& simDescription);

		/*!
		 *	\brief	Get simulation description.
		 */
		const std::string& Description() { return desc; }

		/*!
		 *	\brief	Set the initial spacing between particles.
		 */
		void SetParticleSpacing(double spacing);

		/*!
		 *	\brief	Get the initial spacing between particles.
		 */
		inline double ParticleSpacing() { return particleSpacing; }

		/*!
		 *	\brief	Get the number of particles of specific type.
		 */
		inline unsigned int ParticleCount(ParticleType type) { return particleCountByType[type]; }

		/*!
		 *	\brief	Get the number of particles.
		 */
		inline unsigned int ParticleCount(bool includeDummy=false) { return includeDummy ? particleCount : particleCount; }

		/*!
		 *	\brief	Get the particle from "global" id only.
		 */
		inline Particle GetParticle(unsigned int id) { return Particle(this, id); }

		/*!
		 *	\brief	Set the SPH smoothing kernel.
		 *	\param	type		Type of SPH smoothing kernel.
		 *	\param	length		SPH smoothing length.
		 *	\param	correction	Should kernel correction (normalization) be used. Default is false.
		 */
		void SetSmoothingKernel(SmoothingKernelType type, double length, bool correction = false);

		/*!
		 *	\brief	Get the smoothing kernel type.
		 */
		inline SmoothingKernelType SmoothingKernel() { return smoothingKernel; }

		/*!
		 *	\brief	Get the smoothing length.
		 */
		inline double SmoothingLength() { return smoothingLength; }

		/*!
		 *	\brief	Is smoothing kernel correction used.
		 */
		inline bool SmoothingKernelCorrection() { return smoothingKernelCorrection; }

		/*!
		 *	\brief	Define where to start applying bucket fill.
		 */
		void SetBucketFillLocation(Vec<3,double> location);

		/*!
		 *	\brief	Set the fluid density.
		 */
		void SetDensity(double density);

		/*!
		 *	\brief	Get the fluid density.
		 */
		inline double Density() { return density; }

		/*!
		 *	\brief	Set the external global acceleration.
		 */
		void SetGravity(Vec<3,double> acceleration);

		/*!
		 *	\brief	Get the external global acceleration.
		 */
		inline Vec<3,double> Gravity() { return gravity; }

		/*!
		 *	\brief	Set the fluid dynamic viscosity.
		 */
		void SetViscosity(double viscosity);

		/*!
		 *	\brief	Get the fluid dynamic viscosity.
		 */
		inline double DynamicViscosity() { return dynamicViscosity; }

		/*!
		 *	\brief	Sets the simulation container boundaries.
		 */
		void SetBoundaries(Vec<3,double> boundsMin, Vec<3,double> boundsMax);

		/*!
		 *	\brief	Get the simulation container minimum point.
		 */
		inline Vec<3,double> BoundaryMin() { return gridMin; }

		/*!
		 *	\brief	Get the simulation container maximum point.
		 */
		inline Vec<3,double> BoundaryMax() { return gridMax; }

		/*!
		 *	\brief	Get the number of dimensions.
		 *	\return	Dimension count: 2 or 3
		 */
		inline unsigned int Dimensions() { return dimensions; }

		/*!
		 *	\brief	Set link to devices that will run the simulation.
		 */
		void SetDevices(CLLink* linkToDevices);

		/*!
		 *	\brief	Get link to devices that will run the simulation.
		 */
		CLLink* Devices();

		/*!
		 *	\brief	Initialize the simulation with all the parameters set.
		 *	\return	Success.
		 */
		virtual bool Initialize();

		/*!
		 *	\brief	Advance simulation by specified time step, in seconds.
		 *	\param	advanceTimeStep	Time step to advance simulation by, in seconds.
		 *	\return	Success.
		 */
		virtual bool Advance(double advanceTimeStep);

		/*!
		 *	\brief	Wait all enqueued computations to finish.
		 */
		void Finish();

		/*!
		 *	\brief	Get the maximum velocity.
		 *
		 *	Internally used for CFL condition.
		 */
		double MaximumVelocity();

		/*!
		 *	\brief	Suggest next time step based on CFL and viscous conditions.
		 *	\return	Time in seconds.
		 */
		virtual double SuggestTimeStep();

		/*!
		 *	\brief	Set simulation runtime parameters.
		 *	\param	time	Overall time, in seconds, to run simulation.
		 *	\param	timeStep	Time steps to advance simulation by, in seconds. Leave as zero for automatic time stepping with function SuggestTimeStep().
		 *	\param	autoFactor	Factor that multiplies with automatically suggested time steps.
		 */
		void SetRunTime(double time, double timeStep = 0.0, double autoFactor = 1.0);

		/*!
		 *	\brief	Run the simulation for a while, automatically export simulated data.
		 */
		bool Run();

		/*!
		 *	\brief	Current time of simulation.
		 *	\return	Time in seconds.
		 */
		inline double Time() { return timeOverall; }

		/*!
		 *	\brief	Time steps made by now.
		 */
		inline unsigned int TimeStepCount() { return timeStepCount; }

		/*!
		 *	\brief	Get the execution time of a time step for profiling, in milliseconds.
		 */
		double TimeStepExecutionTime() { return timeStepTimer; }

		/*!
		 *	\brief	Get the amount of used memory in bytes simulation has allocated on devices.
		 */
		size_t UsedMemorySize();

		/*!
		 *	\brief	Get geometry model by name.
		 *	\return	If found, pointer to the geometry object, else a NULL pointer.
		 */
		Geometry* GetGeometry(const std::string& name);

		/*!
		 *	\brief	Get list of exporters that will output simulated data.
		 */
		std::list<Writer*>& Exporters() { return exporters; }

		/*!
		*	\brief	Set if exporting of simulated data runs asynchronous with simulation. Default is true.
		*
		*	If you want exporting of simulated data to be run in different thread, while simulation continues to run.
		*/
		void SetAsyncExport(bool enabled);

		/*!
		 *	\brief	Get if exporting of simulated data runs asynchronous with simulation.
		 */
		inline bool AsyncExport() { return asyncExport; }

		/*!
		 *	\brief	Get OpenCL program associated with solver.
		 */
		inline CLProgram* Program() { return program; }

		/*!
		 *	\brief	Get the buffer that contains and manipulates with particles masses.
		 */
		inline CLGlobalBuffer* ParticleMasses()     { return massesBuffer; }

		/*!
		 *	\brief	Get the buffer that contains and manipulates with particles positions.
		 */
		inline CLGlobalBuffer* ParticlePositions()  { return positionsBuffer; }

		/*!
		 *	\brief	Get the buffer that contains and manipulates with particles densities.
		 */
		inline CLGlobalBuffer* ParticleDensities()  { return densitiesBuffer; }

		/*!
		 *	\brief	Get the buffer that contains and manipulates with particles pressures.
		 */
		inline CLGlobalBuffer* ParticlePressures()  { return pressuresBuffer; }

		/*!
		 *	\brief	Get the buffer that contains and manipulates with particles velocities.
		 */
		inline CLGlobalBuffer* ParticleVelocities() { return velocitiesBuffer; }

		/*!
		 *	\brief	Get the simulation internal scalar data type.
		 */
		VariableDataType ScalarDataType();

	protected:

		/*!
		 *	\brief	Get the simulation internal 2-component vector data type.
		 */
		VariableDataType Scalar2DataType();

		/*!
		 *	\brief	Get the simulation internal 4-component vector data type.
		 */
		VariableDataType Scalar4DataType();

		/*!
		 *	\brief	Get the simulation internal 8-component vector data type.
		 */
		VariableDataType Scalar8DataType();

		/*!
		 *	\brief	Get the simulation internal vector data type.
		 *	\param	realNumbers	Set to true if query is for real number vectors, otherwise is for integer vectors.
		 */
		VariableDataType VectorDataType(bool realNumbers = true);

		/*!
		 *	\brief	Send the data from host to devices.
		 */
		virtual bool UploadModifiedBuffers();

		/*!
		 *	\brief	Load a subprogram from .CL file, and set a name for it.
		 */
      bool LoadSubprogram(const std::string& name, const std::string& source);

		/*!
		 *	\brief	Execute subprogram.
		 */
		bool EnqueueSubprogram(const std::string& name, size_t globalSize=0, size_t localSize=0);        

		/*!
		 *	\brief	Insert all particles to grid.
		 */
		virtual bool RunGrid();

		/*!
		 *	\brief	Create boolean simulation property to use it in OpenCL programs
		 */
		bool InitSimulationVariable(const std::string& semantic, VariableDataType dataType, bool constant);

		/*!
		 *	\brief	Create number simulation property to use it in OpenCL programs
		 */
		bool InitSimulationVariable(const std::string& semantic, VariableDataType dataType, double data, bool constant);

		/*!
		 *	\brief	Create vector simulation property to use it in OpenCL programs
		 */
		bool InitSimulationVariable(const std::string& semantic, VariableDataType dataType, Vec<3,double> data, bool constant);

		/*!
		 *	\brief	Init simulation attribute.
		 */
		CLGlobalBuffer* InitSimulationBuffer(const std::string& semantic, VariableDataType dataType, unsigned int elementCount);

		/*!
		 *	\brief	Init general simulation subprograms, variables and build options.
		 */
		virtual bool InitGeneral();

		/*!
		 *	\brief	Init simulation grid subprograms, variables and build options.
		 */
		virtual bool InitGrid();

		/*!
		 *	\brief	
		 */
		virtual bool PreProcessGeometry();

		/*!
		 *	\brief	Convert geometry to particles.
		 */
		virtual bool InitGeometry();

		/*!
		 *	\brief	Init subprograms, variables and build options, specific to the inherited SPH method.
		 */
		virtual bool InitSph() = 0;

		/*!
		 *	\brief	Post initialization kernel execution, specific to the inherited SPH method.
		 */
		virtual bool PostInitSph() = 0;

		/*!
		 *	\brief	Run actual SPH simulation. To be inherited by different SPH method classes.
		 */
		virtual bool RunSph() = 0;

		/*!
		 *	\brief	Flood fill at some location to make fluid.
		 */
		unsigned int MakeFluid(Vec<3,double> source, bool onlyCountParticles);

		/*!
		 *	\brief	Create fluid particle at a location.
		 */
		void InitFluidParticle(unsigned int id, const Vec<3,double>& position);

		void InitDummyParticle(unsigned int id, const Vec<3,double>& position);

		unsigned int InitDummies(unsigned int wallId, const Vec<3,double>& pos, const Vec<3,double>& normal, bool onlyCountParticles);

		unsigned int InitOverlappingCorners(bool onlyCountParticles);

		/*!
		 *	\brief	Fill dummy particles on corners.
		 *	\param	n1	First corner wall normal.
		 *	\param	n2	Second corner wall normal.
		 *	\return	Count of created particles.
		 */
		unsigned int InitDummyCorner(unsigned int wallId, const Vec<2,double>& pos, const Vec<2,double>& n1, const Vec<2,double>& n2, bool onlyCountParticles);

		/*!
		 *	\brief	Check if particle (sphere) intersects any geometry.
		 */
		bool ParticleCollision(const Vec<3,double>& position);

		bool IsCornerOverlapping(const Vec<3,double>& position);


		// variable members:

		friend class Particle;
		friend class Geometry;
      friend class ProbeManager;
		friend class Writer;

		// allocation info
		VariableDataType scalarType;
		unsigned int dimensions;

		// solver
		CLProgram *program;

		// subprograms
		std::map<std::string,CLSubProgram*> subprograms;

		// scene data
		std::multimap<std::string,Geometry*> models;
		Vec<3,double> floodFillLocation;
		
		unsigned int overlappedCornersStart;
		struct OverlappingCorner
		{
			Geometry::Corner c1, c2;
		};
		std::vector<OverlappingCorner> overlappingCorners;

		// particle data
		double particleSpacing;
		unsigned int particleCount;
		unsigned int particleCountByType[ParticleTypeCount];
		unsigned int deviceParticleCount;
		double particleMass[ParticleTypeCount];
		CLGlobalBuffer *classBuffer, *massesBuffer, *positionsBuffer, *velocitiesBuffer, *pressuresBuffer, *densitiesBuffer, *normalsBuffer;

		// kernel
		SmoothingKernelType smoothingKernel;
		double smoothingLength;
		bool smoothingKernelCorrection;
	
		// density & viscosity
		double density;
		double dynamicViscosity;

		// misc
		std::string name;
		std::string desc;
		Vec<3,double> gravity;

		// scene grid
		double gridCellSize;
		Vec<3,double> gridMin;
		Vec<3,double> gridMax;
		Vec<3,double> gridSize;
		Vec<3,int> gridCellCount;
		clppContext* clppSetup;
		clppSort* clppSorter;

		// time
		double maxTime;
		double wantedTimeStep;
		double timeOverall;
		double timeStep;
		double cflFactor;
		unsigned int timeStepCount;
		double timeStepTimer;
		Timer timer;

		// exporters
		std::list<Writer*> exporters;
		bool asyncExport;
public:
		tthread::mutex exporterMutex;

	};
}

#endif
