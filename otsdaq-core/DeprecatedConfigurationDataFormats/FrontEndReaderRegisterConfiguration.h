#ifndef _ots_FERRegisterConfiguration_h_
#define _ots_FERRegisterConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/RegisterTable.h"

namespace ots
{
class FERRegisterConfiguration : public ots::RegisterConfiguration
{
  public:
	FERRegisterConfiguration(void);
	virtual ~FERRegisterConfiguration(void);
};
}  // namespace ots
#endif
