#ifndef _ots_DesktopIconConfiguration_h_
#define _ots_DesktopIconConfiguration_h_

#include <string>

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class MessageFacilityConfiguration : public TableBase
{
  public:
	MessageFacilityConfiguration(void);
	virtual ~MessageFacilityConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);

  private:
};
}  // namespace ots
#endif
