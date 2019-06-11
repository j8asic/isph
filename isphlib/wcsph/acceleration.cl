/*!
 *	\brief	Compute accelerations for each particle
 */
#if VISCOSITY_FORMULATION == 3
	//__global const tensor *tau		: SPS_TENSORS,
#endif
__kernel void Accelerations
(
	__global vector *xsphVel 		: XSPH_VELOCITIES,
	__global vector *xsphVelS 		: XSPH_SORTED,
	__global vector *acc			: ACCELERATIONS,
	__global const vector *vel		: SORTED_VELOCITIES,
	__global const vector *pos		: SORTED_POSITIONS,
	__global const scalar *density	: SORTED_DENSITIES,
	__global const scalar *mass		: SORTED_MASSES,
	__global const scalar *pods		: PODS,
	__global const uint *cellsStart	: CELLS_START,
	__global const int2 *hashes		: HASHES,
	uint fluidParticleCount			: FLUID_PARTICLE_COUNT,
	scalar deltaPKernelInv			: DELTA_P_INV
)
{
	size_t i = get_global_id(0);
	uint unsorted = hashes[i].y;
	if(unsorted >= fluidParticleCount)
	{
		xsphVelS[i] = vel[i];
		//xsphVel[unsorted] = velI;
		return;
	}
	vector posI = pos[i];
	vector velI = vel[i];
	scalar densityI = density[i];
	scalar podsI = pods[i];
	scalar tensileI = podsI * (podsI<0 ? TC_EPSILON1 : TC_EPSILON2);
	
#if VISCOSITY_FORMULATION == 1 // ARTIFICIAL
	scalar csI = densityI * DENSITY_INV;
	csI *= WC_SOUND_SPEED * csI * csI;
#elif VISCOSITY_FORMULATION == 3 // SPS
	tensor tauI = tau[i];
#endif
	vector a = (vector)0;
	vector corr = (vector)0;
	
	ForEachSetup(posI)
	ForEachNeighbor(hashes,cellsStart,pos,posI)

		scalar massJ = mass[_j];
		scalar podsJ = pods[_j];
		vector velDif = velI - vel[_j];
		scalar W = SphKernel(QSq);
		vector gradW = SphKernelGrad(QSq, posDif);

	 	// tensile correction, Monaghan JCP 2000
		scalar f = W * deltaPKernelInv;
		f *= f; f *= f; // (Wij/Wdp)^4
		scalar tensileJ = podsJ * (podsJ<0 ? TC_EPSILON1 : TC_EPSILON2);
		
		// Additional condition positive pressure tensile control only if both pressures are positive
	    scalar tensile  = podsI<0 ? tensileI : 0;
		tensile += podsJ<0 ? tensileJ : 0;
	    tensile += (podsI>0 && podsJ>0) ? tensileI + tensileJ : 0;

	    // pressure and viscosity acceleration
		#if VISCOSITY_FORMULATION == 1 // ARTIFICIAL (Monaghan 1994)
			scalar densityJ = density[_j];
			scalar densityAdd = 1 / (densityI + densityJ);
    	    scalar csJ = densityJ * DENSITY_INV;
	        csJ *= WC_SOUND_SPEED * csJ * csJ;
			scalar phi = SMOOTHING_LENGTH * min(dot(posDif,velDif), (scalar)0) / (dot(posDif,posDif) + DIST_EPSILON);
            a -= (podsI + podsJ + f * tensile + (2*BETA_VISCOSITY*phi*phi - ALPHA_VISCOSITY*phi*(csI + csJ))*densityAdd) * massJ * gradW;
        #else //#if VISCOSITY_FORMULATION >= 2 // LAMINAR (Lo & Shao 2002)
			scalar densityAdd = 1 / (densityI + density[_j]);
            a -= (podsI + podsJ + f * tensile) * massJ * gradW;
        	a += 4 * DYNAMIC_VISCOSITY * massJ * velDif * dot(gradW, posDif) * densityAdd / (dot(posDif,posDif) + DIST_EPSILON);
		#endif
		#if VISCOSITY_FORMULATION == 3 && DIM == 2 // SPS 2D
			tensor t = tauI + tau[_j];
			a += massJ * (vector)(dot(gradW,t.xy), dot(gradW,t.yz));
		#elif VISCOSITY_FORMULATION == 3 && DIM == 3 // SPS 3D
			tensor t = tauI + tau[_j];
			a += massJ * (vector)(
				dot(gradW,(vector)(t.s012,0),
				dot(gradW,(vector)(t.s134,0)),
				dot(gradW,(vector)(t.s245,0)),
				0);
		#endif
		
		// XSPH
		if(particleJ.y < fluidParticleCount)
			corr += velDif * (massJ * W * densityAdd);
		
	ForEachEnd

	acc[unsorted] = a + GRAVITY;
		
	vector xsph = velI - 2 * XSPH_FACTOR * corr;
	xsphVelS[i] = xsph;
	xsphVel[unsorted] = xsph;
}