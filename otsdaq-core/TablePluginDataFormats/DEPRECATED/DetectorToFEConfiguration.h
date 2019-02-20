#ifndef _ots_DetectorToFEConfiguration_h_
#define _ots_DetectorToFEConfiguration_h_

#include <string>
#include <vector>

#include "../../TableCore/TableBase.h"

namespace ots
{
class DetectorToFEConfiguration : public TableBase
{
  public:
	DetectorToFEConfiguration(void);
	virtual ~DetectorToFEConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getters
	std::vector<std::string> getFEWriterDetectorList(std::string interfaceID) const;
	std::vector<std::string> getFEReaderDetectorList(std::string interfaceID) const;
	// std::vector<std::string>  getFEWCards     (unsigned int supervisorInstance) const;
	unsigned int getFEWriterChannel(const std::string& detectorID) const;
	unsigned int getFEWriterDetectorAddress(const std::string& detectorID) const;
	unsigned int getFEReaderChannel(const std::string& detectorID) const;

  private:
	enum
	{
		DetectorID,
		FEWriterID,
		FEWriterChannel,
		FEWriterDetectorAddress,
		FEReaderID,
		FEReaderChannel,
		FEReaderDetectorAddress
	};
	struct DetectorInfo
	{
		std::string  theFEWriterID_;
		unsigned int theFEWriterChannel_;
		unsigned int theFEWriterDetectorAddress_;
		std::string  theFEReaderID_;
		unsigned int theFEReaderChannel_;
		unsigned int theFEReaderDetectorAddress_;
	};
	std::map<std::string, DetectorInfo> nameToInfoMap_;
};
}  // namespace ots
#endif
