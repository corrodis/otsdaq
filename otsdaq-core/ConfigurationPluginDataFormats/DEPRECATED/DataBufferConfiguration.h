#ifndef ots_DataBufferConfiguration_h
#define ots_DataBufferConfiguration_h

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <map>
#include <string>
#include <vector>

namespace ots {

class DataBufferConfiguration : public ConfigurationBase {
       public:
	DataBufferConfiguration(void);
	virtual ~DataBufferConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getter
	std::vector<std::string> getProcessorIDList(std::string dataBufferID) const;

	std::vector<std::string> getProducerIDList(std::string dataBufferID) const;
	bool			 getProducerStatus(std::string dataBufferID, std::string producerID) const;
	std::string		 getProducerClass(std::string dataBufferID, std::string producerID) const;

	std::vector<std::string> getConsumerIDList(std::string dataBufferID) const;
	bool			 getConsumerStatus(std::string dataBufferID, std::string consumerID) const;
	std::string		 getConsumerClass(std::string dataBufferID, std::string consumerID) const;

       private:
	enum {
		UniqueID,
		DataBufferID,
		ProcessorID,
		ProcessorType,
		ProcessorClass,
		ProcessorStatus
	};

	struct Info
	{
		std::string class_;
		bool	status_;
	};

	struct BufferProcessors
	{
		std::map<std::string, Info> producers_;
		std::map<std::string, Info> consumers_;
		std::map<std::string, Info> processors_;
	};
	//DataBufferID,
	std::map<std::string, BufferProcessors> processorInfos_;
};
}  // namespace ots
#endif
