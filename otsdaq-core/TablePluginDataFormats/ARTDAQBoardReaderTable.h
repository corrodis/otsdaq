#ifndef _ots_ARTDAQBoardReaderTable_h_
#define _ots_ARTDAQBoardReaderTable_h_

#include <string>

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQBoardReaderTable : public TableBase
{
  public:
	ARTDAQBoardReaderTable(void);
	virtual ~ARTDAQBoardReaderTable(void);

	// Methods
	void        init(ConfigurationManager* configManager);
	void        outputFHICL(ConfigurationManager*           configManager,
	                        const ConfigurationTree&        readerNode,
	                        unsigned int                    selfRank,
	                        std::string                     selfHost,
	                        unsigned int                    selfPort,
	                        const XDAQContextTable* contextConfig);
	std::string getFHICLFilename(const ConfigurationTree& readerNode);

	// std::string	getBoardReaderApplication	(const ConfigurationTree &readerNode,
	// const XDAQContextTable *contextConfig, const ConfigurationTree
	// &contextNode, std::string &applicationUID, std::string &bufferUID, std::string
	// &consumerUID);
};
}  // namespace ots
#endif
