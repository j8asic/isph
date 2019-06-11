#include "vtkwriter.h"
#include "simulation.h"
#include "log.h"
#include "utils.h"
#include "clprogram.h"

using namespace isph;

void fix_endian(double& d)
{
	unsigned char *dst = (unsigned char *)&d;
	unsigned char src[8];
	std::memcpy(src, dst, 8);

	dst[0] = src[7];
	dst[1] = src[6];
	dst[2] = src[5];
	dst[3] = src[4];
	dst[4] = src[3];
	dst[5] = src[2];
	dst[6] = src[1];
	dst[7] = src[0];
}

VtkWriter::VtkWriter(Simulation* simulation)
	: Writer(simulation)
	, binary(false)
	, endianSwap(false)
{
	SetFileExtension("vtk");
}


VtkWriter::~VtkWriter()
{

}


bool VtkWriter::Prepare()
{
	if(!Writer::Prepare())
		return false;

	if (binary && Utils::MachineEndianness() != BigEndian)
		endianSwap = true;

	return true;
}


void VtkWriter::PrepareData()
{
	/// \todo make opencl endianess fix and do it here 

	this->sim->ParticlePositions()->Download();

	Writer::PrepareData();
}


void VtkWriter::WriteData()
{
	// since VTK file format is for only one time step, ignore standard header/footer procedure
	// on every step create new file, and write everything at once
	std::string curPath = this->path + '_' + Utils::IntegerString(this->ExportsCount()) + '.' + this->extension;

	this->UpdateStats();

	Log::Send(Log::Info, "Exporting data to: " + curPath + ", for simulation time: " + Utils::DoubleString(this->LastExportedTime()));

	if(binary)
		stream.open(curPath.c_str(), std::ios_base::binary);
	else
		stream.open(curPath.c_str());

	if(!stream.is_open())
	{
		Log::Send(Log::Error, "Couldn't open new VTK export file: " + curPath);
		return;
	}
    
	// write header
    WriteHeader();

	stream.setf(std::ios::scientific);

	// write positions
    WritePositions();

	// write data
	for (std::list<CLGlobalBuffer*>::iterator iter = attributeList.begin(); iter != attributeList.end(); ++iter)
	{
		if (iter == attributeList.begin())
        	stream << "POINT_DATA " << this->sim->ParticleCount()  << std::endl;

		if((*iter)->IsScalar())
			WriteScalarField(*iter);
		else
			WriteVectorField(*iter);
	}

	// close file
	if(stream.is_open())
		stream.close();
}


void VtkWriter::WriteHeader()
{
	// write header
	stream << "# vtk DataFile Version 2.0" << std::endl;
	stream << "t = " << this->LastExportedTime() << " s" << std::endl;
	if(binary)
		stream << "BINARY" << std::endl;
	else
		stream << "ASCII" << std::endl;
	stream << "DATASET UNSTRUCTURED_GRID" << std::endl;
}


void VtkWriter::WritePositions()
{
	CLGlobalBuffer* att = this->sim->ParticlePositions();

	if(!att)
	{
		Log::Send(Log::Error, "Particles positions buffer is incorrectly initialized");
		return;
	}
	
	// write positions

	stream << "POINTS " << this->sim->ParticleCount() << " double" << std::endl;

	if(binary)
	{
		if(endianSwap)
		{
			for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
			{
				Vec<3,double> pos = att->GetVector(i);
				fix_endian(pos.x);
				fix_endian(pos.y);
				fix_endian(pos.z);
				stream.write(reinterpret_cast<char*>(&pos), sizeof(Vec<3,double>));
			}
		}
		else
		{
			for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
			{
				Vec<3,double> pos = att->GetVector(i);
				stream.write(reinterpret_cast<char*>(&pos), sizeof(Vec<3,double>));
			}
		}
		stream << std::endl;
	}
	else
	{
		for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
		{
			Vec<3,double> pos = att->GetVector(i);
			stream << pos.x << ' ' << pos.y << ' ' << pos.z << std::endl;
		}
	}
}


void VtkWriter::WriteScalarField(CLGlobalBuffer* att)
{
	if(!att)
		return;

	if(binary)
	{
		if(endianSwap)
		{
			stream << "SCALARS " << att->Semantic() << " double" << std::endl;
			stream << "LOOKUP_TABLE default" << std::endl;
			for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
			{
				double x = att->GetScalar(i);
				fix_endian(x);
				stream.write(reinterpret_cast<char*>(&x), sizeof(double));
			}
		}
		else
		{
			if(att->DataType() == FloatType)
				stream << "SCALARS " << att->Semantic() << " float" << std::endl;
			else
				stream << "SCALARS " << att->Semantic() << " double" << std::endl;
			stream << "LOOKUP_TABLE default" << std::endl;
			stream.write((char*)att->HostData(), att->DataTypeSize() * this->sim->ParticleCount());
		}
		stream << std::endl;
	}
	else
	{
		stream << "SCALARS " << att->Semantic() << " double" << std::endl;
		stream << "LOOKUP_TABLE default" << std::endl;
		for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
			stream << att->GetScalar(i) << std::endl;
	}
}


void VtkWriter::WriteVectorField(CLGlobalBuffer* att)
{
	if(!att)
		return;

	stream << "VECTORS " << att->Semantic() << " double" << std::endl;

	if(binary)
	{
		if(endianSwap)
		{
			for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
			{
				Vec<3,double> vec = att->GetVector(i);
				fix_endian(vec.x);
				fix_endian(vec.y);
				fix_endian(vec.z);
				stream.write(reinterpret_cast<char*>(&vec), sizeof(Vec<3,double>));
			}
		}
		else
		{
			for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
			{
				Vec<3,double> vec = att->GetVector(i);
				stream.write(reinterpret_cast<char*>(&vec), sizeof(Vec<3,double>));
			}
		}
		stream << std::endl;
	}
	else
	{
		for (unsigned int i=0; i < this->sim->ParticleCount(); i++)
		{
			Vec<3,double> v = att->GetVector(i);
			stream << v.x << ' ' << v.y << ' ' << v.z << std::endl;
		}
	}
}

void VtkWriter::SetBinaryOutput( bool binary )
{
	this->binary = binary;
}
