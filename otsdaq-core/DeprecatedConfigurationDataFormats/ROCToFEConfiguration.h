#ifndef ots_ROCToFEConfiguration_h
#define ots_ROCToFEConfiguration_h

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <string>
#include <vector>

namespace ots
{
class ROCToFEConfiguration : public ConfigurationBase
{
  public:
	ROCToFEConfiguration (void);
	virtual ~ROCToFEConfiguration (void);

	//Methods
	void init (ConfigurationManager* configManager);

	//Getters
	std::vector<std::string> getFEWROCsList (std::string fECNumber) const;
	std::vector<std::string> getFERROCsList (std::string fEDNumber) const;
	std::vector<std::string> getFEWCards (unsigned int supervisorInstance) const;
	unsigned int             getFEWChannel (const std::string& rOCName) const;
	unsigned int             getFEWROCAddress (const std::string& rOCName) const;
	unsigned int             getFERChannel (const std::string& rOCName) const;

  private:
	enum
	{
		DetectorID,
		FEWName,
		FEWChannel,
		FEWROCAddress,
		FERName,
		FERChannel,
		FERROCAddress
	};
	struct ROCInfo
	{
		std::string  theFEWName_;
		unsigned int theFEWChannel_;
		unsigned int theFEWROCAddress_;
		std::string  theFERName_;
		unsigned int theFERChannel_;
		unsigned int theFERROCAddress_;
	};
	std::map<std::string, ROCInfo> nameToInfoMap_;
};
}  // namespace ots
#endif
