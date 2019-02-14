#ifndef _ots_FESlowControlsConfiguration_h_
#define _ots_FESlowControlsConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include <string>

namespace ots
{
class FESlowControlsConfiguration : public ConfigurationBase
{
  public:
	FESlowControlsConfiguration(void);
	virtual ~FESlowControlsConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getters

  private:
	//Column names
	struct ColSlowControls
	{
		std::string const colDataType_ = "ChannelDataType";
	} colNames_;
};
}  // namespace ots
#endif
