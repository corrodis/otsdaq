#ifndef _ots_ARTDAQConsumerConfiguration_h_
#define _ots_ARTDAQConsumerConfiguration_h_

#include <string>

#include "otsdaq-coreTableCore/TableBase.h"

namespace ots
{
class ARTDAQConsumerConfiguration : public TableBase
{
  public:
	ARTDAQConsumerConfiguration(void);
	virtual ~ARTDAQConsumerConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getters
	const std::string getConfigurationString(std::string processorUID) const;

  private:
	enum
	{
		ProcessorID,
		ConfigurationString
	};
};
}  // namespace ots
#endif
