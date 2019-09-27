#ifndef _ots_ARTDAQBoardReaderTable_h_
#define _ots_ARTDAQBoardReaderTable_h_

#include <string>

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
class XDAQContextTable;

class ARTDAQBoardReaderTable : public ARTDAQTableBase
{
  public:
	ARTDAQBoardReaderTable(void);
	virtual ~ARTDAQBoardReaderTable(void);

	// Methods
	void        init(ConfigurationManager* configManager);
	void        outputFHICL(const ConfigurationManager*    configManager,
	                        const ConfigurationTree& readerNode,
	                        unsigned int             selfRank,
							const std::string&       selfHost,
	                        unsigned int             selfPort,
	                        const XDAQContextTable*  contextConfig,
		size_t maxFragmentSizeBytes);
	//std::string getFHICLFilename(const ConfigurationTree& readerNode);

	// std::string	getBoardReaderApplication	(const ConfigurationTree &readerNode,
	// const XDAQContextTable *contextConfig, const ConfigurationTree
	// &contextNode, std::string &applicationUID, std::string &bufferUID, std::string
	// &consumerUID);
};
}  // namespace ots
#endif
