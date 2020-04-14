#ifndef _ots_ARTDAQEventBuilderTable_h_
#define _ots_ARTDAQEventBuilderTable_h_

#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"
#include "otsdaq/TablePlugins/SlowControlsTableBase/SlowControlsTableBase.h"

namespace ots
{
class XDAQContextTable;
// clang-format off
class ARTDAQEventBuilderTable : public ARTDAQTableBase, public SlowControlsTableBase
{
  public:
	ARTDAQEventBuilderTable(void);
	virtual ~ARTDAQEventBuilderTable(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getters
	virtual bool    slowControlsChannelListHasChanged	(void) const override;
	virtual void    getSlowControlsChannelList			(std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>& channelList) const override;

	unsigned int	slowControlsHandler					(
															  std::stringstream& out
															, std::string& tabStr
															, std::string& commentStr
															, std::string& subsystem
															, std::string& location
															, ConfigurationTree slowControlsLink
															, std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList /*= 0*/
														);

  private:
	bool 			outputEpicsPVFile					(ConfigurationManager* configManager,
	                       								std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList = 0) const;

	bool            			isFirstAppInContext_, channelListHasChanged_;  // for managing if PV list has changed
	ConfigurationManager* 		lastConfigManager_;
};
// clang-format on
}  // namespace ots
#endif
