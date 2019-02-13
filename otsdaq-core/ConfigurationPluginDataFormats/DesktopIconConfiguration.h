#ifndef _ots_DesktopIconConfiguration_h_
#define _ots_DesktopIconConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

namespace ots {

class DesktopIconConfiguration : public ConfigurationBase {
 public:
  DesktopIconConfiguration(void);
  virtual ~DesktopIconConfiguration(void);

  // Methods
  void init(ConfigurationManager *configManager);

  struct DesktopIcon {
    bool enforceOneWindowInstance_;
    std::string caption_, alternateText_, imageURL_, windowContentURL_, folderPath_;
    std::string permissionThresholdString_;  // <groupName>:<permissionsThreshold> pairs separated by ',' '&' or '|'
  };

  const std::vector<DesktopIconConfiguration::DesktopIcon> &getAllDesktopIcons() const {
    return activeDesktopIcons_;
  }  // activeDesktopIcons_ is setup in init

 private:
  std::string removeCommas(const std::string &str, bool andHexReplace = false, bool andHTMLReplace = false);

  std::vector<DesktopIconConfiguration::DesktopIcon> activeDesktopIcons_;  // only icons with status=true
};
}  // namespace ots
#endif
