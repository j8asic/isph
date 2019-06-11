#include "isph.h"
#include <sstream>

using namespace isph;

CLProgramConstant::CLProgramConstant(CLProgram* program, const std::string& semantic)
	: CLKernelArgument(program, semantic, true)
{

}


CLProgramConstant::~CLProgramConstant()
{

}


std::string CLProgramConstant::CLSource()
{
	if(!data)
		return "";

	std::ostringstream stream;

	switch(varDataType)
	{
	case FloatType:
		stream << *(float*)data; break;
	case DoubleType:
		stream << *(double*)data; break;
	case IntType:
		stream << *(int*)data; break;
	case UintType:
		stream << *(unsigned int*)data; break;
	case Float2Type:
		stream << "(float2)(" << ((Vec<2,float>*)data)->x << ", " << ((Vec<2,float>*)data)->y << ")"; break;
	case Double2Type:
		stream << "(double2)(" << ((Vec<2,double>*)data)->x << ", " << ((Vec<2,double>*)data)->y << ')'; break;
	case Int2Type:
		stream << "(int2)(" << ((Vec<2,int>*)data)->x << ", " << ((Vec<2,int>*)data)->y << ')'; break;
	case Uint2Type:
		stream << "(uint2)(" << ((Vec<2,unsigned int>*)data)->x << "u, " << ((Vec<2,unsigned int>*)data)->y << "u)"; break;
	case Float4Type:
		stream << "(float4)(" << ((Vec<3,float>*)data)->x << ", " << ((Vec<3,float>*)data)->y << ", " << ((Vec<3,float>*)data)->z << ", 0.0f)"; break;
	case Double4Type:
		stream << "(double4)(" << ((Vec<3,double>*)data)->x << ", " << ((Vec<3,double>*)data)->y << ", " << ((Vec<3,double>*)data)->z << ", 0.0)"; break;		
	case Int4Type:
		stream << "(int4)(" << ((Vec<3,int>*)data)->x << ", " << ((Vec<3,int>*)data)->y << ", " << ((Vec<3,int>*)data)->z << ", 0)"; break;
	case Uint4Type:
		stream << "(uint4)(" << ((Vec<3,unsigned int>*)data)->x << "u, " << ((Vec<3,unsigned int>*)data)->y << "u, " << ((Vec<3,unsigned int>*)data)->z << "u, 0u)"; break;
	case CharType:
		stream << "(char)(" << *(char*)data; break;
	case UCharType:
		stream << "(uchar)(" << *(unsigned char*)data; break;
	default:
		return "";
	}

	return stream.str();
}
