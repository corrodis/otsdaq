#ifndef _ots_MaskConfiguration_h_
#define _ots_MaskConfiguration_h_

#include <map>
#include <string>

#include "otsdaq-coreTableCore/TableBase.h"

namespace ots
{
class MaskConfiguration : public TableBase
{
  public:
	MaskConfiguration(void);
	virtual ~MaskConfiguration(void);

	// Methods
	virtual void init(ConfigurationManager* configManager);

	// Getters
	const std::string& getROCMask(std::string rocName) const;

  protected:
	std::map<std::string, unsigned int> nameToRow_;

  private:
	enum
	{
		DetectorID,
		KillMask
	};
};
}  // namespace ots
#endif
