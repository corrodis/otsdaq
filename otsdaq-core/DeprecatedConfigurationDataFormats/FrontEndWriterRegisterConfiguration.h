#ifndef _ots_FEWRegisterConfiguration_h_
#define _ots_FEWRegisterConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/RegisterTable.h"

namespace ots
{
class FEWRegisterConfiguration : public ots::RegisterConfiguration
{
  public:
	FEWRegisterConfiguration(void);
	virtual ~FEWRegisterConfiguration(void);
};
}  // namespace ots
#endif
