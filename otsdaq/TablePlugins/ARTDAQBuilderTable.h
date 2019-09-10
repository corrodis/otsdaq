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
	void outputFHICL(ConfigurationManager*    configManager,
	                 const ConfigurationTree& builderNode,
	                 unsigned int             selfRank,
	                 std::string              selfHost,
	                 unsigned int             selfPort,
	                 const XDAQContextTable*  contextConfig);
};
}  // namespace ots
#endif
