#ifndef _ots_ARTDAQDispatcherTable_h_
#define _ots_ARTDAQDispatcherTable_h_

#include <string>

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQDispatcherTable : public ARTDAQTableBase
{
  public:
	ARTDAQDispatcherTable(void);
	virtual ~ARTDAQDispatcherTable(void);

	// Methods
	void        init(ConfigurationManager* configManager);
	void        outputFHICL(ConfigurationManager*    configManager,
	                        const ConfigurationTree& builderNode,
	                        unsigned int             selfRank,
	                        std::string              selfHost,
	                        unsigned int             selfPort,
	                        const XDAQContextTable*  contextConfig);
};
}  // namespace ots
#endif
