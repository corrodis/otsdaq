#ifndef _ots_DesktopIconConfiguration_h_
#define _ots_DesktopIconConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include <string>

namespace ots
{

class DesktopIconConfiguration : public ConfigurationBase
{

public:

	DesktopIconConfiguration(void);
	virtual ~DesktopIconConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

private:
	std::string removeCommas(const std::string &str, bool andHexReplace = false, bool andHTMLReplace = false);

};
}
#endif
