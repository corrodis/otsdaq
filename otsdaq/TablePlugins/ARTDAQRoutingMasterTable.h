#ifndef _ots_ARTDAQRoutingMasterTable_h_
#define _ots_ARTDAQRoutingMasterTable_h_

#include <string>

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQRoutingMasterTable : public ARTDAQTableBase
{
  public:
	ARTDAQRoutingMasterTable(void);
	virtual ~ARTDAQRoutingMasterTable(void);

	// Methods
	void init(ConfigurationManager* configManager);
};
}  // namespace ots
#endif
