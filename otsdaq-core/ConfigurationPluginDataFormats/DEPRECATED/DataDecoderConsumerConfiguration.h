#ifndef _ots_DataDecoderConsumerConfiguration_h_
#define _ots_DataDecoderConsumerConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <string>

namespace ots {

class DataDecoderConsumerConfiguration : public ConfigurationBase {
 public:
  DataDecoderConsumerConfiguration(void);
  virtual ~DataDecoderConsumerConfiguration(void);

  // Methods
  void init(ConfigurationManager *configManager);

  // Getter
  std::vector<std::string> getProcessorIDList(void) const;

 private:
  void check(std::string processorUID) const;
  enum { ProcessorID };

  std::map<std::string, unsigned int> processorIDToRowMap_;
};
}  // namespace ots
#endif
