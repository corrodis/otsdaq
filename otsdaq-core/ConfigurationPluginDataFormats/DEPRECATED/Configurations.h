#ifndef _ots_Configurations_h_
#define _ots_Configurations_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationVersion.h"

#include <set>
#include <string>

namespace ots {

class Configurations : public ConfigurationBase {
 public:
  Configurations(void);
  virtual ~Configurations(void);

  // Methods
  void init(ConfigurationManager *configManager);
  bool findKOC(ConfigurationGroupKey ConfigurationGroupKey, std::string koc) const;

  // Getters
  ConfigurationVersion getConditionVersion(const ConfigurationGroupKey &ConfigurationGroupKey, std::string koc) const;

  std::set<std::string> getListOfKocs(
      ConfigurationGroupKey ConfigurationGroupKey = ConfigurationGroupKey()) const;  // INVALID to get all Kocs
  void getListOfKocsForView(
      ConfigurationView *cfgView, std::set<std::string> &kocList,
      ConfigurationGroupKey ConfigurationGroupKey = ConfigurationGroupKey()) const;  // INVALID to get all Kocs

  // Setters
  int setConditionVersionForView(ConfigurationView *cfgView, ConfigurationGroupKey ConfigurationGroupKey,
                                 std::string koc, ConfigurationVersion newKOCVersion);

 private:
  enum { ConfigurationGroupKeyAlias, KOC, ConditionVersion };
};
}  // namespace ots
#endif
