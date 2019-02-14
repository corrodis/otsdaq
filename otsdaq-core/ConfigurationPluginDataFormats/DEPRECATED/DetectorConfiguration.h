#ifndef _ots_DetectorConfiguration_h_
#define _ots_DetectorConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <map>
#include <string>
#include <vector>

namespace ots
{
class DetectorConfiguration : public ConfigurationBase
{
  public:
	DetectorConfiguration (void);
	virtual ~DetectorConfiguration (void);

	//Methods
	void init (ConfigurationManager* configManager);

	//Getters
	const std::vector<std::string>& getDetectorIDs (void) const;
	const std::vector<std::string>& getDetectorTypes (void) const;  //TODO add a class or configuration.info where there is a list of supported DetectorTypes
	const std::string&              getDetectorType (const std::string& detectorID) const;
	const std::string&              getDetectorStatus (const std::string& detectorID) const;

	const std::map<std::string, unsigned int>& getNameToRowMap () const { return nameToRow_; }

  private:
	enum
	{
		DetectorID,
		DetectorType,
		DetectorStatus
	};

	std::map<std::string, unsigned int> nameToRow_;
	std::vector<std::string>            detectorIDs_;
	std::vector<std::string>            detectorTypes_;
};
}  // namespace ots
#endif
