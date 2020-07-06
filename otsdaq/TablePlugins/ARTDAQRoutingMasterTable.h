#ifndef _ots_ARTDAQRoutingMasterTable_h_
#define _ots_ARTDAQRoutingMasterTable_h_

#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"
#include "otsdaq/TablePlugins/SlowControlsTableBase/SlowControlsTableBase.h"

namespace ots
{
class XDAQContextTable;
// clang-format off
class ARTDAQRoutingMasterTable : public ARTDAQTableBase, public SlowControlsTableBase
{
  public:
	ARTDAQRoutingMasterTable(void);
	virtual ~ARTDAQRoutingMasterTable(void);

	// Methods
	void 					init						(ConfigurationManager* configManager) override;

	virtual unsigned int	slowControlsHandlerConfig	(
															  std::stringstream& out
															, ConfigurationManager* configManager
															, std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList /*= 0*/
														) const override;

	virtual std::string		setFilePath					() const override;
};
// clang-format on
}  // namespace ots
#endif
