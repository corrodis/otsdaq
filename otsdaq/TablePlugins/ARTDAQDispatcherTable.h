#ifndef _ots_ARTDAQDispatcherTable_h_
#define _ots_ARTDAQDispatcherTable_h_

#include <string>

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQDispatcherTable : public ARTDAQTableBase
{
  public:
	ARTDAQDispatcherTable(void);
	virtual ~ARTDAQDispatcherTable(void);

	// Methods
	void init(ConfigurationManager* configManager);
};
}  // namespace ots
#endif
