#ifndef _ots_ARTDAQDataLoggerTable_h_
#define _ots_ARTDAQDataLoggerTable_h_

#include <string>

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQDataLoggerTable : public ARTDAQTableBase
{
  public:
	ARTDAQDataLoggerTable(void);
	virtual ~ARTDAQDataLoggerTable(void);

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
