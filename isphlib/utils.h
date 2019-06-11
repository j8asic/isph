#ifndef ISPH_UTILS_H
#define ISPH_UTILS_H

#include <cmath>
#include <string>
#include "vec.h"

namespace isph
{
	/*!
	 *	\enum  Number representation Endianness 
	 *	\brief Byte ordering used in the binary file.
	 */
	enum Endianness
	{
		BigEndian,	        //!< Most significan byte first.
		LittleEndian		//!< Least significant byte first.
	};


	/*!
	 *	\class	Utils
	 *	\brief	Static class with some useful functions.
	 */
   namespace Utils
	{
		/*!
		 *	\brief	Test if an number (x) is power of 2 number (x=2^y).
		 */
      bool IsPowerOf2(unsigned int x);

		/*!
		 *	\brief	Get a number closest to specified one, that is power of 2 number (x=2^y).
		 */
      unsigned int NearestPowerOf2(unsigned int x);

		/*!
		 *	\brief	Get the nearest multiple.
		 *	\param	x	Number to be near to.
		 *	\param	dividend	Dividend, returned number will be multiple of.
		 *	\param	snapUp	If true, find multiple larger than num, otherwise smaller.
		 */
      unsigned int NearestMultiple(unsigned int x, unsigned int dividend, bool snapUp = true);

		/*!
		 *	\brief	Create a random sequence of 2D integer vectors.
		 */
      Vec<2,unsigned int>* CreateRandomSequence(unsigned int sequenceLength);

		/*!
		 *	\brief	Convert a number to std::string.
		 */
      std::string IntegerString(int number);

		/*!
		 *	\brief	Convert a number to std::string.
		 */
      std::string DoubleString(double number);

		/*!
		 *	\brief	Returns machine native endianness.
		 */
      Endianness MachineEndianness();

		/*!
		 *	\brief	Check if a double precision variable broke its number context.
		 */
      bool IsNaN(double number);

		/*!
		 *	\brief	Check if two line segments intersect.
		 */
      bool LinesIntersect(Vec<2,double>& startA, Vec<2,double>& endA, Vec<2,double>& startB, Vec<2,double>& endB, Vec<2,double>* intersection = NULL);

		/*!
		 *	\brief	Check if line intersects a circle.
		 */
      bool CircleLineIntersect(const Vec<3,double>& startLine, const Vec<3,double>& endLine, const Vec<3,double>& sphereCenter, double radius);

		/*!
		 *	\brief	Check if two circles collide.
		 */
      bool CirclesIntersect(const Vec<3,double>& center1, double radius1, const Vec<3,double>& center2, double radius2);

		/*!
		 *	\brief	Hash two integers into one, with Cantor's pairing function.
		 */
      int PackIntegerPair(int z1, int z2);

   }

} // namespace isph

#endif
