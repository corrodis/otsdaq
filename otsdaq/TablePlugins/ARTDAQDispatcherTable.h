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
	void outputFHICL(ConfigurationManager*    configManager,
	                 const ConfigurationTree& builderNode,
	                 unsigned int             selfRank,
					 const std::string&       selfHost,
	                 unsigned int             selfPort,
	                 const XDAQContextTable*  contextConfig,
	                 size_t                   maxFragmentSizeBytes);
};
}  // namespace ots
#endif
