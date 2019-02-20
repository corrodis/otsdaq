#ifndef _ots_ARTDAQBuilderConfiguration_h_
#define _ots_ARTDAQBuilderConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/FEInterfaceTableBase.h"

namespace ots
{
class ARTDAQBuilderConfiguration : public TableBase
{
  public:
	ARTDAQBuilderConfiguration(void);
	virtual ~ARTDAQBuilderConfiguration(void);

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
		BuilderID,
		Status,
		ConfigurationString
	};
};
}  // namespace ots
#endif
