#ifndef _ots_ARTDAQBoardReaderConfiguration_h_
#define _ots_ARTDAQBoardReaderConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

namespace ots {

class XDAQContextConfiguration;

class ARTDAQBoardReaderConfiguration : public ConfigurationBase {
 public:
  ARTDAQBoardReaderConfiguration(void);
  virtual ~ARTDAQBoardReaderConfiguration(void);

  // Methods
  void init(ConfigurationManager *configManager);
  void outputFHICL(ConfigurationManager *configManager, const ConfigurationTree &readerNode, unsigned int selfRank,
                   std::string selfHost, unsigned int selfPort, const XDAQContextConfiguration *contextConfig);
  std::string getFHICLFilename(const ConfigurationTree &readerNode);

  // std::string	getBoardReaderApplication	(const ConfigurationTree &readerNode, const XDAQContextConfiguration
  // *contextConfig, const ConfigurationTree &contextNode, std::string &applicationUID, std::string &bufferUID,
  // std::string &consumerUID);
};
}  // namespace ots
#endif
