#include "geometry.h"
#include "isph.h"
#include <cmath>
#include <cfloat>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace isph;

unsigned int Geometry::objectCount = 0;

Geometry::Geometry( Simulation* parentSimulation, ParticleType particleType )
   : startId(0)
	, particleCount(0)
   , sim(parentSimulation)
   , type(particleType)
	, startMovementTime(0)
	, endMovementTime(0)
{
	objectId = objectCount;
	name = "object_" + Utils::IntegerString(objectId);
	sim->models.insert(std::pair<std::string,Geometry*>(name, this));
	objectCount++;
}


Geometry::Geometry( Simulation* parentSimulation, ParticleType particleType, std::string name )
   : startId(0)
   , particleCount(0)
   , sim(parentSimulation)
   , type(particleType)
{
	objectId = objectCount;

	if(name.empty())
		this->name = "object_" + Utils::IntegerString(objectId);
	else
		this->name = name;

	sim->models.insert(std::pair<std::string,Geometry*>(this->name, this));

	objectCount++;
}


Geometry::~Geometry()
{

}


unsigned int Geometry::ParticleCount()
{
	if(!particleCount)
		Build(true);
	return particleCount;
}


void Geometry::InitParticle(const Vec<3,double>& pos, const Vec<3,double>& normal, bool onlyCountParticles, bool isCorner)
{
	if(isCorner)
		if(sim->IsCornerOverlapping(pos))
			return;

	if(!onlyCountParticles)
	{
		Particle p = sim->GetParticle(startId + particleCount);
		p.SetType(BoundaryParticle);
		p.SetPosition(pos);
		p.SetDensity(sim->Density());
		p.SetMass(sim->particleMass[FluidParticle]);
		p.SetPressure(0.);
		p.SetVelocity(Vec<3,double>());
		p.SetNormal(normal);
	}

	particleCount += 1 + sim->InitDummies(startId + particleCount, pos, normal, onlyCountParticles);
}

void Geometry::SetVelocity(Vec<3,double> velocity)
{
	/*if(particleCount)
	{
		sim->program->Argument("VECTOR_VALUE")->Write(velocity);
		sim->program->Argument("OBJECT_START")->Write(startId);
		sim->program->Argument("OBJECT_PARTICLE_COUNT")->Write(particleCount);
		sim->EnqueueSubprogram("upload attribute", Utils::NearestMultiple(particleCount, 512));
		}*/
}

void Geometry::SetInitExpression(std::string attribute, std::vector<std::string> expression)
{
	initializationExp.insert(std::pair<std::string,std::vector<std::string> >(attribute, expression));
}

void Geometry::SetMovementExpression(std::vector<std::string> position, std::vector<std::string> velocity, double startTime, double endTime)
{
	positionExp = position;
	velocityExp = velocity;
	startMovementTime = startTime;
	endMovementTime = endTime;
}

void ReplaceAll(std::string& result, const std::string& replaceWhat, const std::string& replaceWithWhat)
{
	while(1)
	{
		int pos = (int)result.find(replaceWhat);
		if(pos > 0)
			if(isalnum(result[pos-1]))
				pos = -1;
		if(pos < (int)result.size() - (int)replaceWhat.size())
			if(isalnum(result[pos+replaceWhat.size()]))
				pos = -1;
		if (pos == -1) return;
		result.replace(pos, replaceWhat.size(), replaceWithWhat);
	}
}

std::string Geometry::GetInitCode()
{
	if(initializationExp.empty())
		return "";

	std::map<std::string,std::vector<std::string> >::iterator it;
	unsigned int startParticle = startId; // todo fluid

	// dynamically build OpenCL code
	std::stringstream code;

	code << "__kernel void EvaluateInitExpression_" << objectId;
	code << "(__global const " << CLSystem::Instance()->DataTypeString(sim->VectorDataType()) << " *posBuf : POSITIONS,";
	code << "__global const char *typ : CLASS,";
	code << "uint particleCount : PARTICLE_COUNT";

	for(it = initializationExp.begin(); it != initializationExp.end(); it++)
	{
		CLGlobalBuffer* buffer = sim->Program()->Buffer(it->first);
		if(!buffer)
		{
			Log::Send(Log::Error, "Cannot find particle attribute: " + it->first + ", to initialize it with math expression.");
			continue;
		}
		code << ", __global " << CLSystem::Instance()->DataTypeString(buffer->DataType()) << " *buf" << it->first << " : " << it->first;
	}

	code << ") {" << std::endl;
	code << "size_t i = get_global_id(0);" << std::endl;
	if(this->Type() == FluidParticle)
	{
		code << "if(i >= particleCount) return;"  << std::endl;
		code << "if(!IsParticleFluid(typ[i])) return;"  << std::endl;
	}
	else
	{
		code << "if(i >= " << ParticleCount() << ") return;"  << std::endl;
		code << "if(IsParticleDummy(typ[i])) return;"  << std::endl;
	}

	code << CLSystem::Instance()->DataTypeString(sim->VectorDataType()) << " r = posBuf[" << startParticle << " + i];" << std::endl;

	for(it = initializationExp.begin(); it != initializationExp.end(); it++)
	{
		CLGlobalBuffer* buffer = sim->Program()->Buffer(it->first);
		code << CLSystem::Instance()->DataTypeString(buffer->DataType()) << " doit" << it->first << " = (" << CLSystem::Instance()->DataTypeString(buffer->DataType()) << ")(";
		
		unsigned int components = 0;
		switch(buffer->DataType())
		{
		case IntType:
		case UintType:
		case FloatType:
		case DoubleType:
		case CharType:
		case  UCharType:
			components = 1; break;
		case Int2Type:
		case Uint2Type:
		case Float2Type:
		case Double2Type:
			components = 2; break;
		case Int4Type:
		case Uint4Type:
		case Float4Type:
		case Double4Type:
			components = 4; break;
		default:
			break;
		}

		for (unsigned int i=0; i<components; i++)
		{
			std::string expression = "0";
			if(i < 3)
			{
				expression = it->second[i];
				ReplaceAll(expression, "x", "(r.s0)"); /// \todo not working sometimes
				ReplaceAll(expression, "y", "(r.s1)");
				ReplaceAll(expression, "z", "(r.s2)");
			}

			if(i > 0) code << ", ";
			code << expression;
		}

		code << ");" << std::endl;
		code << "buf" << it->first << "[" << startParticle << " + i] = doit" << it->first << ";" << std::endl;
	}

	code << "}" << std::endl;

	return code.str();
}

std::string Geometry::GetMovementCode()
{
	if(/*positionExp.empty() ||*/ velocityExp.empty())
	{
		Log::Send(Log::Error, "For analytic movement, need to define both position and velocity expressions (for now)");
		return "";
	}

	// dynamically build OpenCL code
	std::stringstream code;

	unsigned int startParticle = startId;

	code << "__kernel void EvaluateMovementExpression_" << objectId;
	code << "(__global " << CLSystem::Instance()->DataTypeString(sim->VectorDataType()) << " *pos : POSITIONS";
	code << ",__global const " << CLSystem::Instance()->DataTypeString(sim->VectorDataType()) << " *initPos : INITIAL_POSITIONS";
	code << ",__global " << CLSystem::Instance()->DataTypeString(sim->VectorDataType()) << " *vel : VELOCITIES";
	code << ", " << CLSystem::Instance()->DataTypeString(sim->ScalarDataType()) << " dt : TIME_STEP";
	code << ", " << CLSystem::Instance()->DataTypeString(sim->ScalarDataType()) << " t : TIME) {" << std::endl;
	code << "__local vector calcExp[2];" << std::endl;
	code << "uint i = get_global_id(0);" << std::endl;
	code << "if(get_local_id(0) == 0) {" << std::endl;
	if(sim->Dimensions() == 2)
	{
		if(!positionExp.empty())
			code << "calcExp[0] = (vector)(" << positionExp[0] << ", " << positionExp[1] << ");" << std::endl;
		code << "calcExp[1] = (vector)(" << velocityExp[0] << ", " << velocityExp[1] << ");" << std::endl;
	}
	else
	{
		if(!positionExp.empty())
			code << "calcExp[0] = (vector)(" << positionExp[0] << ", " << positionExp[1] << ", " << positionExp[2] << ", 0);" << std::endl;
		code << "calcExp[1] = (vector)(" << velocityExp[0] << ", " << velocityExp[1] << ", " << velocityExp[2] << ", 0);" << std::endl;
	}
	code << "}" << std::endl;
	code << "barrier(CLK_LOCAL_MEM_FENCE);" << std::endl;
	code << "if(i >= " << ParticleCount() << ") return;" << std::endl;
	code << "uint gi = i + " << Utils::IntegerString(startParticle) << ";" << std::endl;
	
	if(positionExp.empty())
		code << "pos[gi] += vel[gi] * dt;" << std::endl;
	else
		code << "pos[gi] = initPos[gi] + calcExp[0];" << std::endl;
	code << "vel[gi] = calcExp[1];" << std::endl;
	code << "}" << std::endl;

	return code.str();
}


// Line

void geo::Line::Define( Vec<2,double> start, Vec<2,double> end, bool normalStartEndLeft)
{
	startPoint = start;
	endPoint = end;
	normal = (end - start).Rotate((((int)normalStartEndLeft)*2-1)*M_PI_2).Normalize();
}

void geo::Line::Build(bool onlyCountParticles)
{
	particleCount = 0;

	Vec<2,double> diff = endPoint - startPoint;
	int lineCnt = (int)(diff.Length() / sim->ParticleSpacing() + 0.5);
	double spacing = diff.Length() / lineCnt;
	Vec<2,double> unitDiff = diff.Normalize();

	InitParticle(startPoint, normal, onlyCountParticles, true);

	for (int i=1; i<lineCnt; i++)
	{
		Vec<2,double> displacement = startPoint + unitDiff * (i*spacing);
		InitParticle(displacement, normal, onlyCountParticles, false);
	}

	InitParticle(endPoint, normal, onlyCountParticles, true);
}

bool geo::Line::ParticleCollision( const Vec<3,double>& position, double radius )
{
	return Utils::CircleLineIntersect(startPoint, endPoint, position, radius);
}

std::vector<Geometry::Corner> geo::Line::Corners()
{
	Geometry::Corner c1 = {startPoint, normal, (startPoint-endPoint).Normalize()};
	Geometry::Corner c2 = {  endPoint, normal, (endPoint-startPoint).Normalize()};
	std::vector<Geometry::Corner> list(2);
	list[0] = c1;
	list[1] = c2;
	return list;
}


// Box


unsigned int geo::Box::CreateEdge(Vec<3,double> origin,Vec<3,double> edge,bool initParticles, bool interiorOnly,unsigned int startParticleIndex)
{
 	/*Vec<3,double> diff;
    unsigned int cnt=0;
	// Define interior edges particles - not counting edges extremes
	diff = edge;
	Vec<3,double> unitDiff = diff.Normalize();    
	cnt = (unsigned int)floor( (diff.Length() - 0.0001)/ sim->ParticleSpacing());
    
	// Increment number of particles to include border ones
	cnt+=2; 

	if (!initParticles) return (cnt - (2*interiorOnly));

	// Calculate real spacing - take into account inclusion / exclusion of boundaries
	double spacing = diff.Length() / (cnt - 1);

	unsigned int p=startParticleIndex;
	for (unsigned int i=0+interiorOnly;i<cnt-interiorOnly; i++)
		{
		    Vec<3,double> displacement  = i*unitDiff * spacing;
			InitParticle(p, origin + displacement);
			p++;
		}
	return (p-startParticleIndex);*/
	return 0;
}

unsigned int geo::Box::CreateFace(Vec<3,double> origin,Vec<3,double> edge1,Vec<3,double> edge2, bool initParticles, bool interiorOnly, unsigned int startParticleIndex)
{
 	/*Vec<3,double> diff1,diff2;
    Vec<2,unsigned int> faceParticleCounts;
	// Define interior edges particles - not counting edges extremes
	diff1 = edge1;
	Vec<3,double> unitDiff1 = diff1.Normalize();    
	faceParticleCounts.x = (unsigned int)floor( (diff1.Length()-0.0001) / sim->ParticleSpacing());
	diff2 = edge2;
    Vec<3,double> unitDiff2 = diff2.Normalize();
	faceParticleCounts.y = (unsigned int)floor( (diff2.Length()-0.0001) / sim->ParticleSpacing());

	// Increment number of particles to include border ones
	faceParticleCounts.x+=2; 
	faceParticleCounts.y+=2; 

	if (!initParticles) return (faceParticleCounts.x - (2*interiorOnly))*
		                       (faceParticleCounts.y - (2*interiorOnly));

	// Calculate real spacing - take into account inclusion / exclusion of boundaries
	double spacing1 = diff1.Length() / (faceParticleCounts.x - 1);
	double spacing2 = diff2.Length() / (faceParticleCounts.y - 1);

	unsigned int p=startParticleIndex;
	for (unsigned int i=0+interiorOnly;i<faceParticleCounts.x-interiorOnly; i++)
    	for (unsigned int j=0+interiorOnly;j<faceParticleCounts.y-interiorOnly; j++)
		{
		    Vec<3,double> displacement  = unitDiff1 * spacing1*i + unitDiff2 * spacing2*j;
			InitParticle(p, origin + displacement);
			p++;
		}
	return (p-startParticleIndex);*/
	return 0;
}

unsigned int geo::Box::CreateSimple(Vec<3,double> originPoint,Vec<3,double>  eastVector,Vec<3,double>  northVector,Vec<3,double>  upVector,unsigned int startParticleIndex,bool initParticles)
{
	/*unsigned int count = startParticleIndex;

	// Count particles on faces
    if ((facesFlag & W)==W) 
		count += CreateFace(originPoint,northVector,upVector,initParticles,true,count);
	if ((facesFlag & E)==E) 
		count += CreateFace(originPoint + eastVector,northVector,upVector,initParticles,true,count);
    if ((facesFlag & S)==S) 
		count += CreateFace(originPoint,eastVector,upVector,initParticles,true,count);
	if ((facesFlag & N)==N) 
		count += CreateFace(originPoint + northVector,eastVector,upVector,initParticles,true,count);
    if ((facesFlag & D)==D) 
		count += CreateFace(originPoint,eastVector,northVector,initParticles,true,count);
	if ((facesFlag & U)==U) 
		count += CreateFace(originPoint + upVector,eastVector,northVector,initParticles,true,count);

	if (((facesFlag & W) | (facesFlag & D))>0) 
		count += CreateEdge(originPoint,northVector,initParticles,true,count);

	if (((facesFlag & E) | (facesFlag & D))>0) 
		count += CreateEdge(originPoint + eastVector,northVector,initParticles,true,count);

	if (((facesFlag & W) | (facesFlag & U))>0) 
		count += CreateEdge(originPoint+ upVector,northVector,initParticles,true,count);

	if (((facesFlag & E) | (facesFlag & U))>0) 
		count += CreateEdge(originPoint+ upVector + eastVector,northVector,initParticles,true,count);

	if (((facesFlag & W) | (facesFlag & S))>0) 
		count += CreateEdge(originPoint,upVector,initParticles,true,count);

	if (((facesFlag & E) | (facesFlag & S))>0) 
		count += CreateEdge(originPoint + eastVector,upVector,initParticles,true,count);

    if (((facesFlag & W) | (facesFlag & N))>0) 
		count += CreateEdge(originPoint + northVector,upVector,initParticles,true,count);

	if (((facesFlag & E) | (facesFlag & N))>0) 
		count += CreateEdge(originPoint + northVector+ eastVector,upVector,initParticles,true,count);

	if (((facesFlag & S) | (facesFlag & D))>0) 
		count += CreateEdge(originPoint,eastVector,initParticles,true,count);

	if (((facesFlag & S) | (facesFlag & U))>0) 
		count += CreateEdge( originPoint + upVector,eastVector,initParticles,true,count);

    if (((facesFlag & N) | (facesFlag & D))>0) 
		count += CreateEdge(originPoint + northVector,eastVector,initParticles,true,count);

	if (((facesFlag & N) | (facesFlag & U))>0) 
		count += CreateEdge(originPoint + northVector + upVector,eastVector,initParticles,true,count);

	// Count particles on corners
    if (((facesFlag & E) | (facesFlag& U) | (facesFlag& N))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint+ eastVector + northVector + upVector);
	  }
	  count++;
	}

	if (((facesFlag & E) | (facesFlag & U) | (facesFlag & S))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint+ eastVector + upVector);
	  }
	  count++;
	}

	if (((facesFlag & E) | (facesFlag & D) | (facesFlag & N))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint+ eastVector + northVector);
	  }
	  count++;
	}

	if (((facesFlag & E) | (facesFlag & D) | (facesFlag & S))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint + eastVector);
	  }
	  count++;
	}
    
	// Count particles on corners
    if (((facesFlag & W) | (facesFlag & U) | (facesFlag & N))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint+ northVector + upVector);
	  }
	  count++;
	}

	if (((facesFlag & W) | (facesFlag & U) | (facesFlag & S))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint +  upVector);
	  }
	  count++;
	}

	if (((facesFlag & W) | (facesFlag & D) | (facesFlag & N))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint + northVector);
	  }
	  count++;
	}

	if (((facesFlag & W) | (facesFlag & D) | (facesFlag & S))>0) 
	{  
	  if (initParticles)
	  {
	     InitParticle(count, originPoint);
	  }
	  count++;
	}
    

    return (count-startParticleIndex);*/
return 0;
}

unsigned int geo::Box::Create(bool initParticles)
{
	/*unsigned int count = 0;
	Vec<3,double> externalDir = -1*(eastVector.Normalize() + northVector.Normalize() + upVector.Normalize());
	externalDir = externalDir.Normalize();
    double spacing = sim->ParticleSpacing();
	for (unsigned int n=1;n<=2;n++)
	{
		Vec<3,double> originDisplacement  = n* externalDir * 0.5*sqrt(1.0*sim->Dimensions())*spacing;
		 
		count = count + CreateSimple(originPoint + originDisplacement,
			eastVector.Normalize()*(eastVector.Length() + n*spacing),
			northVector.Normalize()*(northVector.Length() + n*spacing),
			upVector.Normalize()*(upVector.Length() + n*spacing),
count,initParticles);
	}
    return count;*/
	return 0;
}

void geo::Box::Build(bool onlyCountParticles)
{
	/*Create(true);*/
}

// Circle

void geo::Circle::Build(bool onlyCountParticles)
{
	particleCount = 0;

	double length = 2.0 * M_PI * radius;
	int lineCnt = (int)(length / sim->ParticleSpacing() + 0.5);
	
	for (int i=0; i<lineCnt; i++)
	{
		double angle = 2.0 * M_PI * (double)i / lineCnt;
		Vec<2,double> r = radius * Vec<2,double>(cos(angle), sin(angle));
		Vec<2,double> n = normalOut ? r.Normalize() : -r.Normalize();
		InitParticle(center + r, n, onlyCountParticles, false);
	}
}

bool geo::Circle::ParticleCollision( const Vec<3,double>& position, double radius )
{
	return Utils::CirclesIntersect(position, radius, this->center, this->radius);
}


// Sphere

void geo::Sphere::Build(bool onlyCountParticles)
{
	/*unsigned int p = 0;
	if(filled)
	{
		double radiusSq = radius * radius;
		Vec<3,double> localStartPoint = -Vec<3,double>(radius-realSpacing/2, radius-realSpacing/2, this->sim->Dimensions()==2 ? 0 : radius-realSpacing/2);
		unsigned int zslices = this->sim->Dimensions()==2 ? 1 : slices;
		for(unsigned int k=0; k<zslices; k++)
			for(unsigned int i=0; i<slices; i++)
				for(unsigned int j=0; j<slices; j++) 
				{
					Vec<3,double> dist = localStartPoint + Vec<3,double>(realSpacing*i, realSpacing*j, realSpacing*k);
					if(dist.LengthSq() <= radiusSq)
					{
						Vec<3,double> pos = center + dist;
						InitParticle(p, Vec<3,double>(&pos.x));
						p++;
					}
				}
	}
	else
	{
		if(this->sim->Dimensions() == 2)
		{
			for (unsigned int i=0; i<particleCount; i++)
			{
				double angle = i * realSpacing;
				InitParticle(i, Vec<3,double>(cos(angle)*radius, sin(angle)*radius));
			}
		}
		else
		{
			// TODO
		}	
	}*/
}

unsigned int geo::Patch::Create(bool initParticles)
{
	/*unsigned int count = 0;
    Vec<3,double> diff1 = (endPoint1 - startPoint);
    Vec<3,double> diff2 = (endPoint2 - startPoint);
    unsigned int lineCnt1 = 1 + (unsigned int)(diff1.Length() / sim->ParticleSpacing());
    unsigned int lineCnt2 = 1 + (unsigned int)(diff2.Length() / sim->ParticleSpacing());
	//count = lineCnt1*lineCnt2;
	
	for (unsigned int i=0; i<width ;i++) 
		count += (lineCnt1 + (i % 2))*(lineCnt2 + (i % 2));

	if (!initParticles)
		return count;

	Vec<3,double> unitDiff1 = diff1.Normalize();
	Vec<3,double> unitDiff2 = diff2.Normalize();

	double spacing1 = diff1.Length() / (lineCnt1 - 1);
	double spacing2 = diff2.Length() / (lineCnt2 - 1);
    double spacingNorm = sim->ParticleSpacing()/2;
	Vec<3,double> unitNormal  = unitDiff1.Cross(unitDiff2).Normalize();

    unsigned int p = 0; 
	for (unsigned int k=0; k<width; k++)
      for (unsigned int i=0; i<lineCnt1 + (k % 2);i++)
        for (unsigned int j=0; j<lineCnt2+ (k % 2);j++)
		{
            Vec<3,double> displacement  = unitDiff1 * (i - 0.5*(k%2)) * spacing1 + unitDiff2 * (j- 0.5*(k%2)) * spacing2- unitNormal * (k * spacingNorm);
			InitParticle(p, startPoint + displacement);
			p++;
		}

	return p;*/
	return 0;
}

void geo::Patch::Build(bool onlyCountParticles)
{
	/*Create(true);*/
}
