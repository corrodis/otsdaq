#ifndef _ots_ARTDAQBuilderTable_h_
#define _ots_ARTDAQBuilderTable_h_

#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQBuilderTable : public ARTDAQTableBase
{
  public:
	ARTDAQBuilderTable(void);
	virtual ~ARTDAQBuilderTable(void);

	// Methods
	void init(ConfigurationManager* configManager);
};
}  // namespace ots
#endif
