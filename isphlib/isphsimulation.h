#ifndef ISPH_ISPHSIMULATION_H
#define ISPH_ISPHSIMULATION_H

#include "simulation.h"

namespace isph {

	/*!
	 *	\enum	ProjectionForm
	 *	\brief	Projection methods, based on pressure-correction scheme.
	 */
	enum ProjectionForm
	{
		NonIncremental,	//!< The simplest homogeneous scheme, originally proposed by Chorin and Temam.
		Standard,		//!< Incremental scheme in standard form but still plagued by a numerical boundary layer that prevents it to be fully 1st or 2nd order accurate.
		Rotational		//!< Incremental scheme in rotational form. Overcomes the difficulty caused by the artificial pressure Neumann BC - fully 1st or 2nd order accurate.
	};

	/*!
	 *	\enum	SolverType
	 *	\brief	Different methods for solving linear systems.
	 *
	 *	It's advised to check theoretical background, advantages and disadvantages
	 *	of listed methods to decide which one to use.
	 */
	enum SolverType
	{
		CG,			//!< Conjugate gradient. Faster but not suitable for more complex cases.
		BiCGSTAB	//!< Stabilized BiConjugate gradient. Slower but stable solver.
	};

	/*!
	 *	\class	IsphSimulation
	 *	\brief	Incompressible SPH simulation class.
	 */
	class IsphSimulation : public Simulation
	{
	public:
		using Simulation::program;
		
		/*!
		 *	\brief	Constructor.
		 *	\param	simDimensions	Number of space dimensions: 2 or 3.
		 *	\param	scalarDataType	Type representing real numbers (affects accuracy and performance): FloatType or DoubleType.
		 */
		IsphSimulation(unsigned int simDimensions, VariableDataType scalarDataType);

		/*!
		 *	\brief	Destructor.
		 */
		virtual ~IsphSimulation();

		/*!
		 *	\brief	Set projection method for solving incompressible Navier-Stokes equations.
		 *	\param	form	Projection form that affects accuracy and performance. NonIncremental 1st order is set by default.
		 *	\param	order	Order of scheme: 1st or 2nd order.
		 */
		void SetProjection(ProjectionForm form, unsigned int order);

		/*!
		 *	\brief	Chosen projection method for solving incompressible Navier-Stokes equations.
		 */
		ProjectionForm Projection() { return projectionForm; }

		/*!
		 *	\brief	Chosen order of projection method.
		 */
		unsigned int ProjectionOrder() { return projectionOrder; }

		/*!
		 *	\brief	Set which method is used to solve Poisson pressure equation.
		 *	\param	type	CG solver is set by default. More complex simulations will probably require BiCGSTAB.
		 */
		void SetSolver(SolverType type);

		/*!
		 *	\brief	Get which method is used to solve Poisson pressure equation.
		 */
		inline SolverType Solver() { return solverType; }

		/*!
		 *	\brief	Set maximum number of iterations that solver should try to do. Default is 100.
		 *
		 *	Iterative solver will stop if maximum number of iterations is achieved if no solution is found,
		 *	see SetSolverTolerance.
		 */
		void SetMaxSolverIterations(unsigned int iterations);

		/*!
		 *	\brief	Get maximum number of iterations that solver should try to do.
		 */
		inline unsigned int MaxSolverIterations() { return maxIterations; }

		/*!
		 *	\brief	Set convergence criterion that satisfies solution. Default is 0.001.
		 *
		 *	Convergence criterion is the maximum normalized error that satisfies solution.
		 *	Iterative solver will stop if normalized error is below specified number, or if maximum number of iterations is achieved.
		 */
		void SetSolverTolerance(double tolerance);

		/*!
		 *	\brief	Get convergence criterion that satisfies solution.
		 */
		inline double SolverTolerance() { return solvingTolerance; }

		/*!
		*	\brief	Set whether particle anti-clustering method should be used.
		*	\param	enable	Choose whether to enable or disable the algorithm. It's disabled by default.
		*	\param	factor	Factor that should be large enough to prevent instability, and small enough not to cause inaccuracy, mostly between 0.01 and 0.1. Default is 0.04.
		*	\param	frequency	After how many time-steps should particles be shifted. Default is 1, which means each time-step.
		 */
		void SetParticleShifting(bool enable, double factor = 0.04, unsigned int frequency = 1);

		/*!
		 *	\brief	Get whether anti-clustering method 'particle shifting' is used.
		 */
		bool ParticleShifting() { return shifting; }

		/*!
		 *	\brief	Get particle shifting factor.
		 */
		double ParticleShiftingFactor() { return shiftingFactor; }

		/*!
		 *	\brief	Get particle shifting frequency.
		 */
		double ParticleShiftingFrequency() { return shiftingFrequency; }

		/*!
		 *	\brief	Set the number below which particle is identified as free-surface. Default is 1.5 (2D) / 2.4 (3D).
		 *
		 *	Divergence of a particle position is used to track the surface particles. Usual value for fluid 
		 *	particles in 2D is 2.0, and in 3D is 3.0. Particles on free surface should have much smaller values.
		 */
		void SetFreeSurfaceFactor(double value);

		/*!
		 *	\brief	Get the number that identifies particle as a free-surface.
		 */
		inline double FreeSurfaceFactor() { return freeSurfaceFactor; }

		/*!
		 *	\brief	Set strong or weak Dirichlet boundary condition for free surface.
		 */
		void SetStrongDirichletBC(bool strong);

		/*!
		 *	\brief	Suggest next time step.
		 *	\return	Time in seconds.
		 */
		virtual double SuggestTimeStep();

	protected:

		ProjectionForm projectionForm;
		unsigned int projectionOrder;
		SolverType solverType;
		unsigned int maxIterations;
		double solvingTolerance;
		double freeSurfaceFactor;
		bool shifting;
		double shiftingFactor;
		unsigned int shiftingFrequency;
		bool strongDirichletBC;

		virtual bool InitSph();
		virtual bool PostInitSph();
		virtual bool RunSph();

		bool SolvePressureWithCG();
		bool SolvePressureWithBiCGSTAB();

		double dot(CLGlobalBuffer* a, CLGlobalBuffer* b);
	};

}

#endif
