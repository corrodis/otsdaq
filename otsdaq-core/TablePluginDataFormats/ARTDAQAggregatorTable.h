#ifndef _ots_ARTDAQAggregatorTable_h_
#define _ots_ARTDAQAggregatorTable_h_

#include <string>

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQAggregatorTable : public TableBase
{
  public:
	ARTDAQAggregatorTable(void);
	virtual ~ARTDAQAggregatorTable(void);

	// Methods
	void        init(ConfigurationManager* configManager);
	void        outputFHICL(ConfigurationManager*           configManager,
	                        const ConfigurationTree&        builderNode,
	                        unsigned int                    selfRank,
	                        std::string                     selfHost,
	                        unsigned int                    selfPort,
	                        const XDAQContextTable* contextConfig);
	std::string getFHICLFilename(const ConfigurationTree& builderNode);
};
}  // namespace ots
#endif
