#ifndef _ots_ARTDAQBuilderTable_h_
#define _ots_ARTDAQBuilderTable_h_

#include <string>

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQBuilderTable : public TableBase
{
  public:
	ARTDAQBuilderTable(void);
	virtual ~ARTDAQBuilderTable(void);

	// Methods
	void        init(ConfigurationManager* configManager);
	void        outputFHICL(ConfigurationManager*    configManager,
	                        const ConfigurationTree& builderNode,
	                        unsigned int             selfRank,
	                        std::string              selfHost,
	                        unsigned int             selfPort,
	                        const XDAQContextTable*  contextConfig);
	std::string getFHICLFilename(const ConfigurationTree& builderNode);
	std::string getFlatFHICLFilename(const ConfigurationTree& builderNode);
	void        flattenFHICL(const ConfigurationTree& builderNode);
};
}  // namespace ots
#endif
