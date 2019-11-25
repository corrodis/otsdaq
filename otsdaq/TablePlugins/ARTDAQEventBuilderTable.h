#ifndef _ots_ARTDAQEventBuilderTable_h_
#define _ots_ARTDAQEventBuilderTable_h_

#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQEventBuilderTable : public ARTDAQTableBase
{
  public:
	ARTDAQEventBuilderTable(void);
	virtual ~ARTDAQEventBuilderTable(void);

	// Methods
	void init(ConfigurationManager* configManager);
};
}  // namespace ots
#endif
