#ifndef _ots_ARTDAQAggregatorConfiguration_h_
#define _ots_ARTDAQAggregatorConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/FEInterfaceTableBase.h"

namespace ots
{
class ARTDAQAggregatorConfiguration : public TableBase
{
  public:
	ARTDAQAggregatorConfiguration(void);
	virtual ~ARTDAQAggregatorConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getters
	std::string       getAggregatorID(unsigned int supervisorInstance) const;
	bool              getStatus(unsigned int supervisorInstance) const;
	const std::string getConfigurationString(unsigned int supervisorInstance) const;

  private:
	enum
	{
		SupervisorInstance,
		AggregatorID,
		Status,
		ConfigurationString
	};
};
}  // namespace ots
#endif
