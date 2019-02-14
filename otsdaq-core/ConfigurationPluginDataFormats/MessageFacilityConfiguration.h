#ifndef _ots_DesktopIconConfiguration_h_
#define _ots_DesktopIconConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include <string>

namespace ots
{

class MessageFacilityConfiguration : public ConfigurationBase
{

public:

	MessageFacilityConfiguration(void);
	virtual ~MessageFacilityConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

private:

};
}
#endif
