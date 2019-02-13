#ifndef _ots_VersionAliases_h_
#define _ots_VersionAliases_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <string>

namespace ots {

class VersionAliases : public ConfigurationBase {
 public:
  VersionAliases(void);
  ~VersionAliases(void);

  // Methods
  void init(ConfigurationManager *configManager);

  // Getters
  unsigned int getAliasedKey(std::string alias) const;

 private:
  enum { VersionAlias, Version, KOC };
};
}  // namespace ots
#endif
