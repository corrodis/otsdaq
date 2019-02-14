#ifndef ots_DetectorRegisterConfiguration_h
#define ots_DetectorRegisterConfiguration_h

#include "otsdaq-core/ConfigurationDataFormats/RegisterConfiguration.h"

namespace ots
{
class DetectorRegisterConfiguration : public ots::RegisterConfiguration
{
  public:
	DetectorRegisterConfiguration (void);
	virtual ~DetectorRegisterConfiguration (void);
};
}  // namespace ots
#endif
