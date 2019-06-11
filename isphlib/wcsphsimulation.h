#ifndef ISPH_WCSPHSIMULATION_H
#define ISPH_WCSPHSIMULATION_H

#include "simulation.h"

namespace isph {

	/*!
	 *	\enum	ViscosityFormulationType
	 *	\brief	Different viscosity formulations types.
	 *	\todo move to wcsph
	 */
	enum ViscosityFormulationType
	{
		ArtificialViscosity = 1,		//!< Artificial Viscosity.
		LaminarViscosity = 2,			//!< Laminar Viscosity.
		SubParticleScaleViscosity = 3	//!< Sub-particle-scale (SPS) viscosity.
	};

	/*!
	 *	\enum IntegratorType
	 *	\brief	Different time advancing schemes.
	 *	\todo move to wcsph
	 */
	enum IntegratorType
	{
		Euler,				//!< Euler 1st order.
		PredictorCorrector,	//!< Predictor Corrector (Modified Euler) 2nd order.
		RungeKutta4			//!< Runge-Kutta 4th order.
	};

    /*!
	 *	\enum	DensityReinitMethods
	 *	\brief	Different density reinitialization methods.
	 */
	enum DensityReinitMethods
	{
		None,						//!< No density reinitialization.
		ShepardFilter,				//!< Shepard filter (0th order).
		MovingLeastSquares			//!< Moving Least Square (1st order, Dilts IJNME 99).
	};

	/*!
	 *	\class	WcsphSimulation
	 *	\brief	Weakly compressible SPH simulation class.
	 *
	 *	In the smoothed particle hydrodynamics (SPH) discretization method for the Navier–Stokes
	 *	equations the most widespread method to solve for pressure and mass conservation
	 *	is the weakly compressible assumption (WCSPH). The pressure is obtained from an algebraic thermodynamic equation
	 *	and diffusion terms are treated as explicit. However, some drawbacks appear: WCSPH requires a very small time
	 *	step associated with a numerical speed of sound (which is roughly at least 10 times higher than the maximum of velocity),
	 *	small density errors always remain causing significant non-physical pressure fluctuations which can yield numerical instability.
	 */
	class WcsphSimulation : public Simulation
	{
	public:
		using Simulation::program;
		
		/*!
		 *	\brief	Constructor.
		 *	\param	simDimensions	Number of space dimensions: 2 or 3.
		 *	\param	scalarDataType	Type representing real numbers (affects accuracy and performance): FloatType or DoubleType.
		 */
		WcsphSimulation(unsigned int simDimensions, VariableDataType scalarDataType);

		/*!
		 *	\brief	Destructor.
		 */
		virtual ~WcsphSimulation();

		/*!
		 *	\brief	Set the integrator type.
		 */
		void SetIntegratorType(IntegratorType type);

		/*!
		 *	\brief	Get the integrator type.
		 */
		inline IntegratorType Integrator() { return integratorType; }

		/*!
		 *	\brief	Set should the grid be refreshed on every integration step.
		 *
		 *	How large the order of integration is - that many grid refreshing. Default is true.
		 *
		 *	\remarks	Affects simulation accuracy and performance.
		 */
		void SetIntegratorGridRefreshing(bool enable);

		/*!
		 *	\brief	Set the formulation for viscosity.
		 */
		void SetViscosityFormulationType(ViscosityFormulationType type);

		/*!
		 *	\brief	Get the viscosity formulation type.
		 */
		inline ViscosityFormulationType  ViscosityFormulation() { return viscosityFormulation; }

		/*!
		 *	\brief	Set WCSPH parameters.
		 *	\param	speedOfSound	Speed of sound inside fluid, in meters per second [m/s].
		 *	\param	gamma			Gamma parameter, default value is 7.
		 */
		void SetWcsphParameters(double speedOfSound, double gamma);

		/*!
		 *	\brief	Get WCSPH gamma parameter.
		 */
		inline double WcsphGamma() { return wcGamma; }

		/*!
		 *	\brief	Get WCSPH speed-of-sound parameter, in meters per second [m/s].
		 */
		inline double WcsphSpeedOfSound() { return wcSpeedOfSound; }

		/*!
		 *	\brief	Set the fluid alpha parameter for artificial viscosity.
		 *	\param	alpha	Value of the alpha parameter for artificial viscosity.
		 *	\param	beta	Value of the beta parameter for artificial viscosity.
		 */
		void SetArtificialViscosity(double alpha, double beta);

		/*!
		 *	\brief	Set XSPH velocity correction parameters.
		 *	\param	factor	The factor of XSPH, default is 0.5
		 */
		inline void SetXsphFactor(double factor) { xsphFactor = factor; }

		/*!
		 *	\brief	Get the XSPH factor.
		 */
		inline double XsphFactor() { return xsphFactor; }

		/*!
		 *	\brief	Set Tensile Correction factor for compression (negative pressure).
		 *	\param	The factor for compression (negative pressure) in tensile correction, default is 0.1
		 */
		inline void SetTCEpsilon1(double factor) { tcEpsilon1= factor; }

		/*!
		 *	\brief	Get the Tensile Correction factor for compression (negative pressure).
		 */
		inline double TCEpsilon1() { return tcEpsilon1; }

			/*!
		 *	\brief	Set Tensile Correction factor for (positive pressure).
		 *	\param	The factor for expansion (positive pressure) in tensile correction , default is 0.01
		 */
		inline void SetTCEpsilon2(double factor) { tcEpsilon2= factor; }

		/*!
		 *	\brief	Get the Tensile Correction factor for expansion (positive pressure).
		 */
		inline double TCEpsilon2() { return tcEpsilon2; }

		/*!
		 *	\brief	Set density reinitialization method.
		 *	\param	method Selected method for density reinitialization.
		 */
		void SetDensityReinitMethod(DensityReinitMethods method);

		/*!
		 *	\brief	Get selected method for density reinitialization.
		 */
		inline DensityReinitMethods DensityReinitMethod() { return densityReinitMethod ; }

		/*!
		 *	\brief	Set frequency of density reinitialization.
		 *	\param	reinitFreq	Number of timesteps between reinitializations.
		 */
		inline void SetDensityReinitFrequency(int reinitFreq) { densityReinitFrequency = reinitFreq; }

		/*!
		 *	\brief	Get frequency of density reinitialization in terms of timesteps.
		 */
		inline int DensityReinitFrequency() { return densityReinitFrequency; }

		/*!
		 *	\brief	Enable density initialization from pressure for fluid particles.
		 *	\param	true enables density correction
		 */
		inline void SetInitDensityFromPressure(bool enable) { initDensityFromPressure = enable; }

		/*!
		 *	\brief	Enable mass initialization from density for fluid particles.
		 *	\param	true enables mass correction
		 */
		inline void SetInitMassFromDensity(bool enable) { initMassFromDensity = enable; }

		/*!
		 *	\brief	Suggest next time step based on CFL condition, Monaghan & Kos (1999).
		 *	\return	Time in seconds.
		 */
		virtual double SuggestTimeStep();

	protected:

		/*!
		 *	\brief	Add an integration step subprogram.
		 */
		bool AddIntegrationStep(const std::string& subprogram);

		/*!
		 *	\brief	Integration step count.
		 */
		inline size_t IntegrationSteps() { return integrationStepSubprograms.size(); }

		/*!
		 *	\brief	Enqueue integration step subprogram.
		 */
		bool EnqueueIntegrationStep(unsigned int i);

		virtual bool UploadModifiedBuffers();

		virtual bool InitSph();
		virtual bool PostInitSph();
		virtual bool RunSph();
		void CalculateDerivatives();

		double wcGamma;
		double wcSpeedOfSound;
		double xsphFactor;
		double tcEpsilon1;
        double tcEpsilon2;

        bool initDensityFromPressure;
		bool initMassFromDensity;
		bool initedWcsphStuff;

		// viscosity
		ViscosityFormulationType viscosityFormulation;
		double alphaViscosity;
		double betaViscosity;

		// integrator
		IntegratorType integratorType;
		bool integratorGridRefresh;
		std::vector<std::string> integrationStepSubprograms;

        // Density reinitialization
		DensityReinitMethods densityReinitMethod;
		int densityReinitFrequency;
	};

}

#endif
