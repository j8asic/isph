#ifndef ISPH_VTKWRITER_H
#define ISPH_VTKWRITER_H

#include "writer.h"
#include <fstream>

namespace isph {

	/*!
	 *	\class	VtkWriter
	 *	\brief	Exporting simulated data to a VTK legacy file-format.
	 */
	class VtkWriter : public Writer
	{
	public:

		VtkWriter(Simulation* simulation);

		virtual ~VtkWriter();

		virtual bool Prepare();

		virtual void PrepareData();

		virtual void WriteData();

		virtual void SetBinaryOutput(bool binary);

	protected:

		virtual void WriteHeader();

		virtual void WritePositions();

		virtual void WriteScalarField(CLGlobalBuffer* att);

		virtual void WriteVectorField(CLGlobalBuffer* att);

		bool binary;
		bool endianSwap;
		std::ofstream stream;
    
	};

} // namespace isph

#endif
