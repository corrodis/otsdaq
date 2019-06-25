#ifndef _ots_DesktopIconTable_h_
#define _ots_DesktopIconTable_h_

#include <string>

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class DesktopIconTable : public TableBase
{
  public:
	DesktopIconTable(void);
	virtual ~DesktopIconTable(void);

	// Methods
	void init(ConfigurationManager* configManager);

	struct DesktopIcon
	{
		bool        enforceOneWindowInstance_;
		std::string caption_, alternateText_, imageURL_, windowContentURL_, folderPath_;
		std::string permissionThresholdString_;  // <groupName>:<permissionsThreshold>
		                                         // pairs separated by ',' '&' or '|'
	};

	const std::vector<DesktopIconTable::DesktopIcon>& getAllDesktopIcons() const
	{
		return activeDesktopIcons_;
	}  // activeDesktopIcons_ is setup in init

  private:
	std::string removeCommas(const std::string& str,
	                         bool               andHexReplace  = false,
	                         bool               andHTMLReplace = false);

	std::vector<DesktopIconTable::DesktopIcon>
	    activeDesktopIcons_;  // only icons with status=true
};
}  // namespace ots
#endif
