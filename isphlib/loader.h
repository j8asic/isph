#ifndef ISPH_LOADER_H
#define ISPH_LOADER_H

#include <string>


namespace isph {

	class Simulation;

	/*!
	 *	\class	Loader
	 *	\brief	Abstract class for reading input data.
	 *	\todo	create cpp file and on constructor setlocale(LC_ALL, "C");
	 */
	class Loader
	{
	public:

		Loader() {}
		virtual ~Loader() {}

		/*!
		 *	\brief	Set the file to read.
		 */
		void SetInput(const std::string& inputPath)	{ path = inputPath; }

		/*!
		 *	\brief	Parse the file and return built simulation object.
		 *	\return	New Simulation object.
		 */
		virtual Simulation* Read() = 0;

	protected:

		std::string path;

	};


}

#endif
