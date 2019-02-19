#ifndef _ots_FEWRegisterSequencer_h_
#define _ots_FEWRegisterSequencer_h_

#include "otsdaq-core/ConfigurationDataFormats/RegisterConfiguration.h"

namespace ots
{
class FEWRegisterSequencer : public ots::RegisterConfiguration
{
  public:
	FEWRegisterSequencer(void);
	virtual ~FEWRegisterSequencer(void);
};
}  // namespace ots
#endif
