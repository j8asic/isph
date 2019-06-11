#include "isph.h"
#include <string>
#include <float.h>
#include <cstring>

using namespace isph;


CLVariable::CLVariable(CLProgram* program, const std::string& semantic, VariableType type)
	: parentProgram(program)
	, data(NULL)
	, varType(type)
	, varDataType(IntType)
	, elementCount(0)
	, memorySize(0)
	, needsUpdate(false)
{
	if(program)
	{
		program->ConnectSemantic(semantic, this);
		program->variablesList.push_back(this);
	}
	else
		Log::Send(Log::Error, "Cannot create variable for NULL OpenCL program.");
}


CLVariable::~CLVariable()
{
	LogDebug("Destroying variable: " + semantics.front());

	/// \todo properly delete semantic connections
	semantics.clear();
	//parentProgram->variablesList.erase myself
}


void CLVariable::SetSpace( VariableDataType dataType, unsigned int elements )
{
	elementCount = elements;
	varDataType = dataType;
	memorySize = elements * DataTypeSize();

	if(Type() == KernelArgument || Type() == ProgramConstant)
		Allocate();
	else
		needsUpdate = true;
}


size_t CLVariable::DataTypeSize()
{
	return CLSystem::Instance()->DataTypeSize(varDataType);
}

double CLVariable::GetScalar( unsigned int id )
{
	if(!data)
		return DBL_MAX;

	switch(DataType())
	{
	case DoubleType:	return *(cl_double*)Get(id);
	case FloatType:		return *(cl_float*)Get(id);
	case IntType:		return *(cl_int*)Get(id);
	case UintType:		return *(cl_uint*)Get(id);
	case CharType:		return *(cl_char*)Get(id);
	case UCharType:		return *(cl_uchar*)Get(id);
	default:
		Log::Send(Log::Error, "Trying to read scalar value from some different data type");
		return DBL_MAX;
	}
}

Vec<3,double> CLVariable::GetVector(unsigned int id)
{
	if(!data)
		return Vec<3,double>(DBL_MAX);

	switch(DataType())
	{
	case Float2Type:	return *(Vec<2,float>*)Get(id);
	case Float4Type:	return *(Vec<3,float>*)Get(id);
	case Double2Type:	return *(Vec<2,double>*)Get(id);
	case Double4Type:	return *(Vec<3,double>*)Get(id);
	case Int2Type:		return *(Vec<2,int>*)Get(id);
	case Int4Type:		return *(Vec<3,int>*)Get(id);
	case Uint2Type:		return *(Vec<2,unsigned int>*)Get(id);
	case Uint4Type:		return *(Vec<3,unsigned int>*)Get(id);
	default:
		Log::Send(Log::Error, "Trying to read vector value from some different data type");
		return Vec<3,double>(DBL_MAX);
	}
}

bool CLVariable::SetScalar( unsigned int id, double var )
{
	if(!data)
		return false;

	void* i = Get(id);

	switch(DataType())
	{
	case DoubleType:	*(cl_double*)i = var; break;
	case FloatType:		*(cl_float*)i = (cl_float)var; break;
	case IntType:		*(cl_int*)i = (cl_int)var; break;
	case UintType:		*(cl_uint*)i = (cl_uint)var; break;
	case CharType:		*(cl_char*)i = (cl_char)var; break;
	case UCharType:		*(cl_uchar*)i = (cl_uchar)var; break;
	default:
		Log::Send(Log::Error, "Trying to write scalar value to some different data type");
		return false;
	}

	return true;
}

bool CLVariable::SetVector( unsigned int id, Vec<3,double> var )
{
	if(!data)
		return false;

	void* i = Get(id);

	switch(DataType())
	{
	case Float2Type:	*(Vec<2,float>*)i = var; break;
	case Float4Type:	*(Vec<3,float>*)i = var; *((float*)i+3) = 0.0f; break;
	case Double2Type:	*(Vec<2,double>*)i = var; break;
	case Double4Type:	*(Vec<3,double>*)i = var; *((double*)i+3) = 0.0; break;
	case Int2Type:		*(Vec<2,int>*)i = var; break;
	case Int4Type:		*(Vec<3,int>*)i = var; *((int*)i+3) = 0; break;
	case Uint2Type:		*(Vec<2,unsigned int>*)i = var; break;
	case Uint4Type:		*(Vec<3,unsigned int>*)i = var; *((unsigned int*)i+3) = 0; break;
	default:
		Log::Send(Log::Error, "Trying to write vector value to some different data type");
		return false;
	}

	return true;
}

void* CLVariable::Get( unsigned int id )
{
	return (void*)(data + DataTypeSize() * id);
}

bool isph::CLVariable::IsScalar()
{
	switch (DataType())
	{
	case FloatType:
	case DoubleType:
	case IntType:
	case UintType:
	case CharType:
	case UCharType:
		return true;
	default:
		return false;
	}
}
