#ifndef _ots_DesktopIconConfiguration_h_
#define _ots_DesktopIconConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

namespace ots
{
class MessageFacilityConfiguration : public ConfigurationBase
{
  public:
	MessageFacilityConfiguration (void);
	virtual ~MessageFacilityConfiguration (void);

	//Methods
	void init (ConfigurationManager *configManager);

  private:
};
}  // namespace ots
#endif
