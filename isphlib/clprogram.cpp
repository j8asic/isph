#include "isph.h"
using namespace isph;

#include <sstream>

CLProgram::CLProgram()
	: link(NULL)
   , madMath(true)
   , unsafeMath(true)
   , finiteMath(true)
   , normalMath(true)
	, isBuilt(false)
	, program(NULL)
{
	CLLocalBuffer *var;
	var = new CLLocalBuffer(this, "LOCAL_SIZE_UINT");  var->SetSpace(UintType, 0);
	var = new CLLocalBuffer(this, "LOCAL_SIZE_UINT2"); var->SetSpace(Uint2Type, 0);
	var = new CLLocalBuffer(this, "LOCAL_SIZE_UINT4"); var->SetSpace(Uint4Type, 0);
	var = new CLLocalBuffer(this, "LOCAL_SIZE_INT");  var->SetSpace(IntType, 0);
	var = new CLLocalBuffer(this, "LOCAL_SIZE_INT2"); var->SetSpace(Int2Type, 0);
	var = new CLLocalBuffer(this, "LOCAL_SIZE_INT4"); var->SetSpace(Int4Type, 0);
	/// \todo add more default local buffers
}

CLProgram::~CLProgram()
{
	// delete subprograms' allocated data
	ClearSubprograms();

	// delete program variables
	for (std::list<CLVariable*>::iterator it=variablesList.begin() ; it != variablesList.end();)
	{
		CLVariable *var = (*it);
		it++;
		if(var)
		{
			delete var;
			var = NULL;
		}
	}
	variables.clear();
	
	// delete program itself
	if(program)
		clReleaseProgram(program);

	LogDebug("Program destroyed");
}

bool CLProgram::AddSubprogram(CLSubProgram* subprogram)
{
	if(!subprogram)
	{
		Log::Send(Log::Error, "Cannot add NULL subprogram to program");
		return false;
	}

	subprogram->ReleaseKernel();
	subprogram->program = this;
	
	subprograms.push_back(subprogram);
	return true;
}

void CLProgram::SetOptimizations(bool multiplyAdd, bool unsafeMathOpts, bool finiteMathOnly, bool disableDenormals)
{
   madMath = multiplyAdd;
   unsafeMath = unsafeMathOpts;
   finiteMath = finiteMathOnly;
   normalMath = disableDenormals;
	isBuilt = false;
}

void CLProgram::AddBuildOption(const std::string& option)
{
	LogDebug("Adding preprocessor option: " + option);
	buildOptions.push_back(option);
}

void CLProgram::ClearBuildOptions()
{
	LogDebug("Clearing program preprocessor options");
	buildOptions.clear();
}

bool CLProgram::Build()
{
	LogDebug("Building program.");
	isBuilt = false;
	
	if(!link)
	{
		Log::Send(Log::Error, "No OpenCL devices set to build program on.");
		return false;
	}

	// make the source from subprograms
	source.clear();

	for (std::map<std::string,CLProgramConstant*>::iterator it=constants.begin() ; it != constants.end(); it++)
	{
		source.append("#define " + it->first + " (" + it->second->CLSource() + ")\n");
	}

	for(size_t i=0; i<subprograms.size(); i++)
	{
		source.append(subprograms[i]->source);
	}

	//Log::Send(Log::Info, source);

	cl_int status;

	// create CL program
	const char* source_cstring = source.c_str();
	size_t source_size = source.size();
	program = clCreateProgramWithSource(link->context, 1, &source_cstring, &source_size, &status); 

	if(status)
	{
		Log::Send(Log::Error, CLSystem::Instance()->ErrorDesc(status));
		return false;
	}

	// set build options
	std::string buildOptionsStr;
   buildOptionsStr = "-cl-no-signed-zeros";
   if (normalMath)
      buildOptionsStr.append(" -cl-denorms-are-zero");
   if (finiteMath)
      buildOptionsStr.append(" -cl-finite-math-only");
   if (madMath)
      buildOptionsStr.append(" -cl-mad-enable");
   if (unsafeMath)
      buildOptionsStr.append(" -cl-unsafe-math-optimizations");

	for(size_t i=0; i<buildOptions.size(); i++)
		buildOptionsStr.append(' ' + buildOptions[i]);

	// build on our link (devices)
	if(!link->BuildProgram(program, buildOptionsStr))
		return false;

	// init the variables for devices
	for (std::map<std::string,CLVariable*>::iterator it=variables.begin() ; it != variables.end(); it++)
	{
		if(it->second->needsUpdate)
			if(!it->second->Allocate())
			{
				Log::Send(Log::Error, "Error while allocating variable: " + it->second->Semantic());
				return false;
			}
	}

	// create kernels
	for(size_t i=0; i<subprograms.size(); i++)
	{
		if(subprograms[i]->IsKernel())
			if(!subprograms[i]->CreateKernel())
			{
				Log::Send(Log::Error, "Error while creating kernel: " + subprograms[i]->KernelName());
				return false;
			}
	}

	isBuilt = true;
	return isBuilt;
}

bool CLProgram::Finish()
{
	if(!isBuilt)
		return false;

	if(!link)
		return false;

	return link->Finish();
}

bool CLProgram::SetLink(CLLink* linkToDevices)
{
	LogDebug("Setting new link to devices for program");

	if(!linkToDevices)
	{
		Log::Send(Log::Error, "Cannot set NULL link to devices");
		return false;
	}

	if(!linkToDevices->DeviceCount())
	{
		Log::Send(Log::Error, "Link isn't connected to any device");
		return false;
	}

	link = linkToDevices;
	isBuilt = false;

	return true;
}

void CLProgram::ClearSubprograms()
{
	LogDebug("Clearing program source");

	for(unsigned int i=0; i<subprograms.size(); i++)
		if(subprograms[i])
		{
			subprograms[i]->ReleaseKernel();
			subprograms[i]->program = NULL;
		}

	subprograms.clear();
	isBuilt = false;
}

CLVariable* CLProgram::Variable(const std::string& semantic)
{
	std::map<std::string,CLVariable*>::iterator found = variables.find(semantic);
	if(found != variables.end())
		return found->second;
	return NULL;
}

CLGlobalBuffer* CLProgram::Buffer( const std::string& semantic )
{
	std::map<std::string,CLGlobalBuffer*>::iterator found = globalBuffers.find(semantic);
	if(found != globalBuffers.end())
		return found->second;
	return NULL;
}

CLKernelArgument* CLProgram::Argument( const std::string& semantic )
{
	std::map<std::string,CLKernelArgument*>::iterator found = arguments.find(semantic);
	if(found != arguments.end())
		return found->second;
	return NULL;
}

CLProgramConstant* CLProgram::Constant( const std::string& semantic )
{
	std::map<std::string,CLProgramConstant*>::iterator found = constants.find(semantic);
	if(found != constants.end())
		return found->second;
	return NULL;
}

bool CLProgram::ConnectSemantic(const std::string& semantic, CLVariable* var, bool autoUpdateKernelsIfNeeded)
{
	if(!var) /// \todo allow NULL var arg that disconnects semantic with current var
	{
		Log::Send(Log::Error, "Cannot connect a semantic with NULL OpenCL variable.");
		return false;
	}

	if(!semantic.size())
	{
		Log::Send(Log::Error, "Cannot connect an empty semantic with OpenCL variable.");
		return false;
	}

	std::map<std::string,CLVariable*>::iterator found = variables.find(semantic);
	
	if(found != variables.end())
	{
		if(found->second == var)
			return true; // variable-sematic connection already made
		else
			found->second->semantics.remove(semantic);
	}

	variables[semantic] = var;

	switch(var->Type())
	{
	case GlobalBuffer:		globalBuffers[semantic] = (CLGlobalBuffer*)var; break;
	case LocalBuffer:		localBuffers[semantic] = (CLLocalBuffer*)var; break;
	case KernelArgument:	arguments[semantic] = (CLKernelArgument*)var; break;
	case ProgramConstant:	arguments[semantic] = (CLKernelArgument*)var; constants[semantic] = (CLProgramConstant*)var; break;
	}

	var->semantics.push_back(semantic);

	// if called at run-time of program, update kernels that depend on this semantic
	if(autoUpdateKernelsIfNeeded && var->Type() == GlobalBuffer && isBuilt)
	{
		for(unsigned int i=0; i<subprograms.size(); i++)
		{
			int semi = subprograms[i]->SemanticIndex(semantic);
			if(semi >= 0)
				//Variable(semantic)->SetAsArgument(subprograms[i], i);
				subprograms[i]->SetPersistentArguments(); /// \todo
		}
	}

	return true;
}

size_t CLProgram::UsedMemorySize()
{
	size_t bytes = 0;
	for (std::list<CLVariable*>::iterator it=variablesList.begin() ; it != variablesList.end(); it++)
		bytes += (*it)->MemorySize();
	return bytes;
}

std::string CLProgram::CompiledBinary()
{
	/// \todo retrieve binary for each device, when multi-device implemented
	if(!isBuilt)
		return "";
	size_t binarySize;
	cl_int status = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binarySize, NULL);
	char *binary = new char[binarySize + 1];
	status = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(char*), &binary, NULL);
	return binary;
}
