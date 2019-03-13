#ifndef _ots_FEInterfaceTableBase_h_
#define _ots_FEInterfaceTableBase_h_

#include "otsdaq-coreTableCore/TableBase.h"

namespace ots
{
class FEInterfaceTableBase : public TableBase
{
  public:
	FEInterfaceTableBase(std::string configurationName) : TableBase(configurationName)
	{
		;
	}
	virtual ~FEInterfaceTableBase(void) { ; }

	virtual std::string getStreamingIPAddress(std::string interface) const
	{
		return "127.0.0.1";
	}
	virtual unsigned int getStreamingPort(std::string interface) const { return 3000; }

  private:
};
}  // namespace ots
#endif
