#ifndef _ots_FERRegisterSequencer_h_
#define _ots_FERRegisterSequencer_h_

#include "otsdaq-core/ConfigurationDataFormats/RegisterTable.h"

namespace ots
{
class FERRegisterSequencer : public RegisterConfiguration
{
  public:
	FERRegisterSequencer(void);
	virtual ~FERRegisterSequencer(void);
};
}  // namespace ots
#endif
