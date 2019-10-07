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
	void init(ConfigurationManager* configManager);
};
}  // namespace ots
#endif
