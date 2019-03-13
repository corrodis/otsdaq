#include "otsdaq-core/DataProcessorPlugins/OtsDataSaverConsumer.h"
#include "otsdaq-core/Macros/ProcessorPluginMacros.h"

using namespace ots;

//========================================================================================================================
OtsDataSaverConsumer::OtsDataSaverConsumer(
    std::string              supervisorApplicationUID,
    std::string              bufferUID,
    std::string              processorUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       configurationPath)
    : WorkLoop(processorUID)
    , RawDataSaverConsumerBase(supervisorApplicationUID,
                               bufferUID,
                               processorUID,
                               theXDAQContextConfigTree,
                               configurationPath)
{
}

//========================================================================================================================
OtsDataSaverConsumer::~OtsDataSaverConsumer(void) {}

//========================================================================================================================
void OtsDataSaverConsumer::writeHeader(void) {}

//========================================================================================================================
// add one byte quad-word count before each packet
void OtsDataSaverConsumer::writePacketHeader(const std::string& data)
{
	unsigned char quadWordsCount = (data.length() - 2) / 8;
	outFile_.write((char*)&quadWordsCount, 1);

	// packetTypes is data[0]
	// seqId is in data[1] position

	if(quadWordsCount)
	{
		unsigned char seqId = data[1];
		if(!(lastSeqId_ + 1 == seqId || (lastSeqId_ == 255 && seqId == 0)))
		{
			__COUT__ << "?????? NOOOO Missing Packets: " << (unsigned int)lastSeqId_
			         << " v " << (unsigned int)seqId << __E__;
		}
		lastSeqId_ = seqId;
	}
}

DEFINE_OTS_PROCESSOR(OtsDataSaverConsumer)
