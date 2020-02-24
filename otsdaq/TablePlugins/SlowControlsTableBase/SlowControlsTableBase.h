#ifndef _ots_SlowControlsTableBase_h_
#define _ots_SlowControlsTableBase_h_

#include "otsdaq/TableCore/TableBase.h"


namespace ots
{
// clang-format off
class SlowControlsTableBase : public TableBase
{
  public:
	SlowControlsTableBase(void);
	SlowControlsTableBase(std::string tableName, std::string* accumulatedExceptions = 0);

	virtual ~SlowControlsTableBase(void);

	virtual bool	slowControlsChannelListHasChanged 	(void) const = 0;
	virtual void	getSlowControlsChannelList			(std::vector<std::string /*channelName*/>& channelList) const = 0;
	
};
// clang-format on
}  // namespace ots

#endif
