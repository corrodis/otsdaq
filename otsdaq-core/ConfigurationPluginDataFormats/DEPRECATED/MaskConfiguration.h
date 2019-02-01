#ifndef _ots_MaskConfiguration_h_
#define _ots_MaskConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <map>
#include <string>

namespace ots
{

class MaskConfiguration : public ConfigurationBase
{

public:

	MaskConfiguration(void);
	virtual ~MaskConfiguration(void);

	//Methods
	virtual void init(ConfigurationManager *configManager);

	//Getters
	const std::string& getROCMask(std::string rocName) const;


protected:
	std::map<std::string, unsigned int> nameToRow_;

private:
	enum{
		DetectorID,
		KillMask
	};

};
}
#endif
