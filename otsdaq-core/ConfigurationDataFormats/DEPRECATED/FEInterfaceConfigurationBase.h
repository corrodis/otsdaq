#ifndef _ots_FEInterfaceConfigurationBase_h_
#define _ots_FEInterfaceConfigurationBase_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

namespace ots {

class FEInterfaceConfigurationBase : public ConfigurationBase {
 public:
  FEInterfaceConfigurationBase(std::string configurationName) : ConfigurationBase(configurationName) { ; }
  virtual ~FEInterfaceConfigurationBase(void) { ; }

  virtual std::string getStreamingIPAddress(std::string interface) const { return "127.0.0.1"; }
  virtual unsigned int getStreamingPort(std::string interface) const { return 3000; }

 private:
};
}  // namespace ots
#endif
