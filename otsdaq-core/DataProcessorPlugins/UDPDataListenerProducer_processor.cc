#include "otsdaq-core/DataProcessorPlugins/UDPDataListenerProducer.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/NetworkUtilities/NetworkConverters.h"
#include "otsdaq-core/Macros/ProcessorPluginMacros.h"

#include <iostream>
#include <cassert>
#include <string.h>
#include <unistd.h>

using namespace ots;


//========================================================================================================================
UDPDataListenerProducer::UDPDataListenerProducer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath)
: WorkLoop     (processorUID)
, Socket
(
		theXDAQContextConfigTree.getNode(configurationPath).getNode("HostIPAddress").getValue<std::string>(),
		theXDAQContextConfigTree.getNode(configurationPath).getNode("HostPort").getValue<unsigned int>()
)
//, Socket       ("192.168.133.100", 40000)
, DataProducer
(
		supervisorApplicationUID,
		bufferUID,
		processorUID,
		theXDAQContextConfigTree.getNode(configurationPath).getNode("BufferSize").getValue<unsigned int>()
)
//, DataProducer (supervisorApplicationUID, bufferUID, processorUID, 100)
, Configurable (theXDAQContextConfigTree, configurationPath)
, dataP_       (nullptr)
, headerP_     (nullptr)
{
	Socket::initialize();
}

//========================================================================================================================
UDPDataListenerProducer::~UDPDataListenerProducer(void)
{}

//========================================================================================================================
bool UDPDataListenerProducer::workLoopThread(toolbox::task::WorkLoop* workLoop)
//bool UDPDataListenerProducer::getNextFragment(void)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << DataProcessor::processorUID_ << " running, because workloop: " << WorkLoop::continueWorkLoop_ << std::endl;
	fastWrite();
	return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void UDPDataListenerProducer::slowWrite(void)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << name_ << " running!" << std::endl;

	if(ReceiverSocket::receive(data_, ipAddress_, port_) != -1)
	{
		//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << name_ << " Buffer: " << message << std::endl;
		//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " -> Got some data. From: " << std::hex << fromIPAddress << " port: " << fromPort << std::dec << std::endl;
		header_["IPAddress"] = NetworkConverters::networkToStringIP  (ipAddress_);
		header_["Port"]      = NetworkConverters::networkToStringPort(port_);
		//        unsigned long long value;
		//        memcpy((void *)&value, (void *) data_.substr(2).data(),8); //make data counter
		//        std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::hex << value << std::dec << std::endl;

		while(DataProducer::write(data_, header_) < 0)
		{
			__MOUT__ << "There are no available buffers! Retrying...after waiting 10 milliseconds!" << std::endl;
			usleep(10000);
			return;
		}
	}
}

//========================================================================================================================
void UDPDataListenerProducer::fastWrite(void)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << name_ << " running!" << std::endl;

	if(DataProducer::attachToEmptySubBuffer(dataP_, headerP_) < 0)
	{
		__MOUT__ << "There are no available buffers! Retrying...after waiting 10 milliseconds!" << std::endl;
		usleep(10000);
		return;
	}

	if(ReceiverSocket::receive(*dataP_, ipAddress_, port_) != -1)
	{
		(*headerP_)["IPAddress"] = NetworkConverters::networkToStringIP  (ipAddress_);
		(*headerP_)["Port"]      = NetworkConverters::networkToStringPort(port_);
		//unsigned long long value;
		//memcpy((void *)&value, (void *) dataP_->substr(2).data(),8); //make data counter
		//__MOUT__ << "Got data: " << dataP_->length()
		//		<< std::hex << value << std::dec
		//		<< " from port: " << NetworkConverters::networkToUnsignedPort(port_)
		//										 << std::endl;
		//if( NetworkConverters::networkToUnsignedPort(port_) == 2001)
		//{
			//			std::cout << __COUT_HDR_FL__ << dataP_->length() << std::endl;
		//	*dataP_ = dataP_->substr(2);
			//			std::cout << __COUT_HDR_FL__ << dataP_->length() << std::endl;
			//			unsigned int oldValue32;
			//			memcpy((void *)&oldValue32, (void *) dataP_->data(),4);
			//			unsigned int value32;
			//			for(unsigned int i=8; i<dataP_->size(); i+=8)
			//			{
			//				memcpy((void *)&value32, (void *) dataP_->substr(i).data(),4); //make data counter
			//				if(value32 == oldValue32)
			//					std::cout << __COUT_HDR_FL__ << "Trimming! i=" << i
			//					<< std::hex << " v: " << value32 << std::dec
			//					<< " from port: " << NetworkConverters::networkToUnsignedPort(port_) << std::endl;
			//
			//				oldValue32 = value32;
			//			}
		//}
		DataProducer::setWrittenSubBuffer<std::string,std::map<std::string,std::string>>();
	}
}

DEFINE_OTS_PROCESSOR(UDPDataListenerProducer)
