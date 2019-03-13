#ifndef _ots_UDPDataListenerProducerConfiguration_h_
#define _ots_UDPDataListenerProducerConfiguration_h_

#include <string>

#include "otsdaq-coreTableCore/TableBase.h"

namespace ots
{
class UDPDataListenerProducerConfiguration : public TableBase
{
  public:
	UDPDataListenerProducerConfiguration(void);
	virtual ~UDPDataListenerProducerConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);

	// Getter
	std::vector<std::string> getProcessorIDList(void) const;
	unsigned int             getBufferSize(std::string processorUID) const;
	std::string              getIPAddress(std::string processorUID) const;
	unsigned int             getPort(std::string processorUID) const;

  private:
	void check(std::string processorUID) const;
	enum
	{
		ProcessorID,
		BufferSize,
		IPAddress,
		Port
	};

	std::map<std::string, unsigned int> processorIDToRowMap_;
};
}  // namespace ots
#endif
