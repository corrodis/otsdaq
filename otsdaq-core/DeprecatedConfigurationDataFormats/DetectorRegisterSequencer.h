#ifndef ots_DetectorRegisterSequencer_h
#define ots_DetectorRegisterSequencer_h

#include "otsdaq-core/ConfigurationDataFormats/RegisterTable.h"

namespace ots
{
class DetectorRegisterSequencer : public ots::RegisterConfiguration
{
  public:
	DetectorRegisterSequencer(void);
	virtual ~DetectorRegisterSequencer(void);
};
}  // namespace ots
#endif
