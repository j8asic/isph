#ifndef ISPH_XMLLOADER_H
#define ISPH_XMLLOADER_H

#include "loader.h"
#include "extern/pugixml/pugixml.hpp"

namespace isph {

	/*!
	 *	\class	XmlLoader
	 *	\brief	Importer of scene and simulation setup from XML.
	 */
	class XmlLoader : public Loader
	{
	public:

		XmlLoader();

		virtual ~XmlLoader();

		virtual Simulation* Read();

	protected:

		inline std::string ParseString(const pugi::xml_node& node, const std::string& def = std::string());
		inline bool ParseBoolean(const pugi::xml_node& node, bool def = false);
		inline int ParseInt(const pugi::xml_node& node, int def = 0);
		inline double ParseScalar(const pugi::xml_node& node, double def = 0.0);
		inline Vec<3,double> ParseVector(const pugi::xml_node& node);
		inline std::vector<std::string> ParseStringArray(const pugi::xml_node& node);

	};

}

#endif
