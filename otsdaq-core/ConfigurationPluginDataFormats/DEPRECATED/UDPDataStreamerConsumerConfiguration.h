#ifndef _ots_UDPDataStreamerConsumerConfiguration_h_
#define _ots_UDPDataStreamerConsumerConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <string>

namespace ots {

class UDPDataStreamerConsumerConfiguration : public ConfigurationBase {
 public:
  UDPDataStreamerConsumerConfiguration(void);
  virtual ~UDPDataStreamerConsumerConfiguration(void);

  // Methods
  void init(ConfigurationManager *configManager);

  // Getter
  std::vector<std::string> getProcessorIDList(void) const;
  std::string getIPAddress(std::string processorUID) const;
  unsigned int getPort(std::string processorUID) const;
  std::string getStreamToIPAddress(std::string processorUID) const;
  unsigned int getStreamToPort(std::string processorUID) const;

 private:
  void check(std::string processorUID) const;
  enum { ProcessorID, IPAddress, Port, StreamToIPAddress, StreamToPort };

  std::map<std::string, unsigned int> processorIDToRowMap_;
};
}  // namespace ots
#endif
