#include "utils.h"
#include "log.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
using namespace std;
using namespace isph;


unsigned int Utils::NearestPowerOf2( unsigned int x )
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
}

unsigned int Utils::NearestMultiple( unsigned int x, unsigned int dividend, bool snapUp )
{
	if((x % dividend) == 0)
		return x;
	else if(snapUp)
		return x - (x % dividend) + dividend;
	else
		return x - (x % dividend);
}

Vec<2,unsigned int>* Utils::CreateRandomSequence(unsigned int sequenceLength)
{
	Vec<2,unsigned int>* hostHashes = new Vec<2,unsigned int>[sequenceLength];
    srand((unsigned int)time(NULL));
	for ( unsigned int n=0; n < sequenceLength; n++ ) 
	{
        Vec<2,unsigned int> element(n,rand());
        hostHashes[n] = element;
	}
    return hostHashes; 
}

bool Utils::IsPowerOf2( unsigned int x )
{
	return ((x - 1) & x) == 0;
}

std::string Utils::IntegerString( int number )
{
	std::stringstream str;
	str << number;
	return str.str();
}

std::string Utils::DoubleString( double number )
{
	std::stringstream str;
	str << number;
	return str.str();
}

Endianness Utils::MachineEndianness()
{           
	int x = 1;
	return *(char*)&x == 1 ? LittleEndian : BigEndian;
}

bool Utils::IsNaN( double number )
{
	return (number != number);
}

bool Utils::LinesIntersect(Vec<2,double>& startA, Vec<2,double>& endA, Vec<2,double>& startB, Vec<2,double>& endB, Vec<2,double>* intersection)
{
	double distAB, theCos, theSin, newX, ABpos;

	//  (1) Translate the system so that start point A is on the origin.
	endA -= startA;
	startB -= startA;
	endB -= startA;

	//  Discover the length of segment A-B.
	distAB = endA.Length();

	//  (2) Rotate the system so that point B is on the positive X axis.
	theCos = endA.x / distAB;
	theSin = endA.y / distAB;

	newX     = startB.x * theCos + startB.y * theSin;
	startB.y = startB.y * theCos - startB.x * theSin;
	startB.x = newX;

	newX   = endB.x * theCos + endB.y * theSin;
	endB.y = endB.y * theCos - endB.x * theSin;
	endB.x = newX;

	//  Fail if segment B doesn't cross line A.
   if ((startB.y < 0. && endB.y < 0.) || (startB.y >= 0. && endB.y >= 0.))
		return false;

	//  (3) Discover the position of the intersection point along line A-B.
	ABpos = endB.x + (startB.x - endB.x) * endB.y / (endB.y - startB.y);

	//  Fail if segment C-D crosses line A-B outside of segment A-B.
	if (ABpos < 0. || ABpos > distAB)
		return false;

	//  (4) Apply the discovered position to line A-B in the original coordinate system.
	if(intersection)
	{
		intersection->x = startA.x + ABpos * theCos;
		intersection->y = startA.y + ABpos * theSin;
	}

	//  Success.
	return true;
}

bool Utils::CircleLineIntersect(const Vec<3,double>& startLine, const Vec<3,double>& endLine, const Vec<3,double>& sphereCenter, double radius)
{
	Vec<2,double> v = endLine - startLine;
	Vec<2,double> w = sphereCenter - startLine;
	double t = w.Dot(v) / v.Dot(v);
	t = (std::max)((std::min)(t, 1.0), 0.0);
	Vec<2,double> closestLine = startLine + (v*t) - sphereCenter;
	return closestLine.LengthSq() < radius*radius;
}

bool Utils::CirclesIntersect(const Vec<3,double>& center1, double radius1, const Vec<3,double>& center2, double radius2 )
{
	double distSq = (center2 - center1).LengthSq();
	double A = radius1*radius1 + radius2*radius2;
	double B = 2.0 * radius1 * radius2;
	if(distSq < A - B || distSq > A + B) // to avoid sqrt
		return false;
	return true;
}

int Utils::PackIntegerPair( int z1, int z2 )
{
	// convert: Z -> N
	z1 = (z1 >= 0) ? z1*2 : -z1*2-1;
	z2 = (z2 >= 0) ? z2*2 : -z2*2-1;
	// hash
	return ((z1 + z2) * (z1 + z2 + 1)) / 2 + z2;
}
