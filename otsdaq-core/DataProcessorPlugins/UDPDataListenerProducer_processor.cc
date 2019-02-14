#include "otsdaq-core/DataProcessorPlugins/UDPDataListenerProducer.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
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
	unsigned int socketReceiveBufferSize;
	try //if socketReceiveBufferSize is defined in configuration, use it
	{
		socketReceiveBufferSize = theXDAQContextConfigTree.getNode(configurationPath).getNode("SocketReceiveBufferSize").getValue<unsigned int>();
	}
	catch(...)
	{
		//for backwards compatibility, ignore
		socketReceiveBufferSize = 0x10000000; //default to "large"
	}

	Socket::initialize(socketReceiveBufferSize);
}

//========================================================================================================================
UDPDataListenerProducer::~UDPDataListenerProducer(void)
{}

//========================================================================================================================
bool UDPDataListenerProducer::workLoopThread(toolbox::task::WorkLoop* workLoop)
//bool UDPDataListenerProducer::getNextFragment(void)
{
	//__CFG_COUT__DataProcessor::processorUID_ << " running, because workloop: " << WorkLoop::continueWorkLoop_ << std::endl;
	fastWrite();
	return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void UDPDataListenerProducer::slowWrite(void)
{
	//__CFG_COUT__name_ << " running!" << std::endl;

	if(ReceiverSocket::receive(data_, ipAddress_, port_) != -1)
	{
		//__CFG_COUT__name_ << " Buffer: " << message << std::endl;
		//__CFG_COUT__processorUID_ << " -> Got some data. From: " << std::hex << fromIPAddress << " port: " << fromPort << std::dec << std::endl;
		header_["IPAddress"] = NetworkConverters::networkToStringIP  (ipAddress_);
		header_["Port"]      = NetworkConverters::networkToStringPort(port_);
		//        unsigned long long value;
		//        memcpy((void *)&value, (void *) data_.substr(2).data(),8); //make data counter
		//        __CFG_COUT__std::hex << value << std::dec << std::endl;

		while(DataProducer::write(data_, header_) < 0)
		{
			__CFG_COUT__ << "There are no available buffers! Retrying...after waiting 10 milliseconds!" << std::endl;
			usleep(10000);
			return;
		}
	}
}

//========================================================================================================================
void UDPDataListenerProducer::fastWrite(void)
{
	//__CFG_COUT__ << " running!" << std::endl;

	if(DataProducer::attachToEmptySubBuffer(
			dataP_, headerP_) < 0)
	{
		__CFG_COUT__ << "There are no available buffers! Retrying...after waiting 10 milliseconds!" << std::endl;
		usleep(10000);
		return;
	}

	if(0) // test data buffers
	{
		sleep(1);
		unsigned long long value = 0xA54321; //this is 8-bytes
		std::string& buffer = *dataP_;
		buffer.resize(8); //NOTE: this is inexpensive according to Lorenzo/documentation in C++11 (only increases size once and doesn't decrease size)
		memcpy((void *)&buffer[0] /*dest*/,(void *)&value /*src*/, 8 /*numOfBytes*/);

		//size() and length() are equivalent
		__CFG_COUT__ << "Writing to buffer " << buffer.size() << " bytes!" << __E__;
		__CFG_COUT__ << "Writing to buffer length " << buffer.length() << " bytes!" << __E__;

		__CFG_COUT__ << "Buffer Data: " << BinaryStringMacros::binaryTo8ByteHexString(buffer) << __E__;

		__CFG_COUTV__(DataProcessor::theCircularBuffer_);

		DataProducer::setWrittenSubBuffer<std::string,std::map<std::string,std::string> >();
		return;
	}

	if(ReceiverSocket::receive(*dataP_, ipAddress_, port_,1,0, true) != -1)
	{
		(*headerP_)["IPAddress"] = NetworkConverters::networkToStringIP  (ipAddress_);
		(*headerP_)["Port"]      = NetworkConverters::networkToStringPort(port_);
	    //__CFG_COUT__ << "Data for ip: " << IPAddress_ << " listening on port: " << requestedPort_ << std::endl;

		if(
				(requestedPort_ == 47000 ||  requestedPort_ == 47001 || requestedPort_ == 47002) &&
				 dataP_->length() > 2)
		{
			unsigned char seqId = (*dataP_)[1];
			if(!(lastSeqId_ + 1 == seqId ||
					(lastSeqId_ == 255 && seqId == 0)))
			{
				__CFG_COUT__ << requestedPort_ <<
						"?????? NOOOO Missing Packets: " <<
						(unsigned int)lastSeqId_ << " v " << (unsigned int)seqId << __E__;
			}

			//if(seqId == 0)
			//	__CFG_COUT__ << requestedPort_ << " test" << __E__;

			lastSeqId_ = seqId;
		}

//		char str[5];
//		for(unsigned int j=0;j<dataP_->size();++j)
//		{
//			sprintf(str,"%2.2x",((unsigned int)(*dataP_)[j]) & ((unsigned int)(0x0FF)));
//
//			if(j%64 == 0) std::cout << "RECV 0x\t";
//			std::cout << str;
//			if(j%8 == 7) std::cout << " ";
//			if(j%64 == 63) std::cout << std::endl;
//		}

		//unsigned long long value;
		//memcpy((void *)&value, (void *) dataP_->substr(2).data(),8); //make data counter
		//__CFG_COUT__ << "Got data: " << dataP_->size()
		//		<< std::hex << value << std::dec
		//		<< " from port: " << NetworkConverters::networkToUnsignedPort(port_)
		//										 << std::endl;
		//if( NetworkConverters::networkToUnsignedPort(port_) == 2001)
		//{
			//			std::cout << __CFG_COUT_HDR_FL__ << dataP_->size() << std::endl;
		//	*dataP_ = dataP_->substr(2);
			//			std::cout << __CFG_COUT_HDR_FL__ << dataP_->size() << std::endl;
			//			unsigned int oldValue32;
			//			memcpy((void *)&oldValue32, (void *) dataP_->data(),4);
			//			unsigned int value32;
			//			for(unsigned int i=8; i<dataP_->size(); i+=8)
			//			{
			//				memcpy((void *)&value32, (void *) dataP_->substr(i).data(),4); //make data counter
			//				if(value32 == oldValue32)
			//					std::cout << __CFG_COUT_HDR_FL__ << "Trimming! i=" << i
			//					<< std::hex << " v: " << value32 << std::dec
			//					<< " from port: " << NetworkConverters::networkToUnsignedPort(port_) << std::endl;
			//
			//				oldValue32 = value32;
			//			}
		//}
		DataProducer::setWrittenSubBuffer<std::string,std::map<std::string,std::string> >();
	}
}

DEFINE_OTS_PROCESSOR(UDPDataListenerProducer)
