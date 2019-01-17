#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/CircularBuffer.h"
#include "otsdaq-core/DataManager/DataProducer.h"
#include "otsdaq-core/DataManager/DataConsumer.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/PluginMakers/MakeDataProcessor.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

/*
#include "otsdaq-core/DataProcessorPlugins/RawDataSaverConsumer.h"
#include "otsdaq-core/DataProcessorPlugins/ARTDAQConsumer.h"
#include "otsdaq-core/EventBuilder/EventDataSaver.h"
#include "otsdaq-core/DataProcessorPlugins/DataListenerProducer.h"
#include "otsdaq-core/DataProcessorPlugins/DataStreamer.h"
#include "otsdaq-core/DataProcessorPlugins/DQMHistosConsumer.h"
#include "otsdaq-core/EventBuilder/AssociativeMemoryEventBuilder.h"
#include "otsdaq-core/EventBuilder/Event.h"
#include "otsdaq-core/DataProcessorPlugins/DataListenerProducer.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/UDPDataListenerProducerConfiguration.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/ARTDAQConsumerConfiguration.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/DataManagerConfiguration.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/DataBufferConfiguration.h"
*/

#include <iostream>
#include <vector>
#include <unistd.h> //usleep

using namespace ots;

//========================================================================================================================
DataManager::DataManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath)
	: Configurable(theXDAQContextConfigTree, supervisorConfigurationPath)
{
	__CFG_COUT__ << "Constructed" << __E__;
}

//========================================================================================================================
DataManager::~DataManager(void)
{
	__CFG_COUT__ << "Destructor" << __E__;
	eraseAllBuffers();
}

//========================================================================================================================
void DataManager::configure(void)
{
	eraseAllBuffers(); //Deletes all pointers created and given to the DataManager!

	for (const auto& buffer :
			theXDAQContextConfigTree_.getNode(theConfigurationPath_ +
					"/LinkToDataManagerConfiguration").getChildren())
	{
		__CFG_COUT__ << "Data Buffer Name: " << buffer.first << std::endl;
		if (buffer.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
		{
			std::vector<unsigned int> producersVectorLocation;
			std::vector<unsigned int> consumersVectorLocation;
			auto bufferConfigurationList = buffer.second.getNode("LinkToDataBufferConfiguration").getChildren();
			unsigned int location = 0;
			for (const auto& bufferConfiguration : bufferConfigurationList)
			{
				__CFG_COUT__ << "Processor id: " << bufferConfiguration.first << std::endl;
				if (bufferConfiguration.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
				{
					if (bufferConfiguration.second.getNode("ProcessorType").getValue<std::string>() == "Producer")
					{
						producersVectorLocation.push_back(location);
					}
					else if (bufferConfiguration.second.getNode("ProcessorType").getValue<std::string>() == "Consumer")
					{
						consumersVectorLocation.push_back(location);
					}
					else
					{
						__CFG_SS__ << "Node ProcessorType in "
							<< bufferConfiguration.first
							<< " of type "
							<< bufferConfiguration.second.getNode("ProcessorType").getValue<std::string>()
							<< " is invalid. The only accepted types are Producer and Consumer" << std::endl;
						__CFG_MOUT_ERR__ << ss.str();
						__CFG_SS_THROW__;
					}
				}
				++location;

			}

			if (producersVectorLocation.size() == 0)// || consumersVectorLocation.size() == 0)
			{
				__CFG_SS__ << "Node Data Buffer "
					<< buffer.first
					<< " has " << producersVectorLocation.size() << " Producers"
					<< " and " << consumersVectorLocation.size() << " Consumers"
					<< " there must be at least 1 Producer " << //	of both configured
					"for the buffer!" << std::endl;
				__CFG_MOUT_ERR__ << ss.str();
				__CFG_SS_THROW__;
			}

			configureBuffer<std::string, std::map<std::string, std::string>>(buffer.first);
			for (auto& producerLocation : producersVectorLocation)
			{
				//				__CFG_COUT__ << theConfigurationPath_ << std::endl;
				//				__CFG_COUT__ << buffer.first << std::endl;
				__CFG_COUT__ << "Creating producer... " <<
						bufferConfigurationList[producerLocation].first << std::endl;
				//				__CFG_COUT__ << bufferConfigurationMap[producer].getNode("ProcessorPluginName").getValue<std::string>() << std::endl;
				//				__CFG_COUT__ << bufferConfigurationMap[producer].getNode("LinkToProcessorConfiguration") << std::endl;
				//				__CFG_COUT__ << "THIS DATA MANAGER POINTER: " << this << std::endl;
				//				__CFG_COUT__ << "PASSED" << std::endl;

				try
				{
					buffers_[buffer.first].producers_.push_back(std::shared_ptr<DataProducer>(
							dynamic_cast<DataProducer*>(
						makeDataProcessor
						(
								bufferConfigurationList[producerLocation].second.getNode(
										"ProcessorPluginName").getValue<std::string>()
										, theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode(
												"ApplicationUID").getValue<std::string>()
												, buffer.first
												, bufferConfigurationList[producerLocation].first
												, theXDAQContextConfigTree_
												, theConfigurationPath_ + "/LinkToDataManagerConfiguration/" + buffer.first +
												"/LinkToDataBufferConfiguration/" + bufferConfigurationList[producerLocation].first +
												"/LinkToProcessorConfiguration"
						))));
				}
				catch(const cet::exception& e)
				{
					__CFG_SS__ << "Failed to instantiate plugin named '" <<
							bufferConfigurationList[producerLocation].first << "' of type '" <<
							bufferConfigurationList[producerLocation].second.getNode(
															"ProcessorPluginName").getValue<std::string>()
							<< "' due to the following error: \n" << e.what() << __E__;
					__CFG_MOUT_ERR__ << ss.str();
					__CFG_SS_THROW__;
				}
				__CFG_COUT__ << bufferConfigurationList[producerLocation].first << " has been created!" << std::endl;
			}
			for (auto& consumerLocation : consumersVectorLocation)
			{
				//				__CFG_COUT__ << theConfigurationPath_ << std::endl;
				//				__CFG_COUT__ << buffer.first << std::endl;
				__CFG_COUT__ << "Creating consumer... " <<
						bufferConfigurationList[consumerLocation].first << std::endl;
				//				__CFG_COUT__ << bufferConfigurationMap[consumer].getNode("ProcessorPluginName").getValue<std::string>() << std::endl;
				//				__CFG_COUT__ << bufferConfigurationMap[consumer].getNode("LinkToProcessorConfiguration") << std::endl;
				//				__CFG_COUT__ << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_) << std::endl;
				//				__CFG_COUT__ << "THIS DATA MANAGER POINTER: " << this << std::endl;
				//				__CFG_COUT__ << "PASSED" << std::endl;
				try
				{
					buffers_[buffer.first].consumers_.push_back(std::shared_ptr<DataConsumer>(dynamic_cast<DataConsumer*>(
							makeDataProcessor
							(
									bufferConfigurationList[consumerLocation].second.getNode(
											"ProcessorPluginName").getValue<std::string>()
									, theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode(
											"ApplicationUID").getValue<std::string>()
									, buffer.first
									, bufferConfigurationList[consumerLocation].first
									, theXDAQContextConfigTree_
									, theConfigurationPath_ + "/LinkToDataManagerConfiguration/" + buffer.first +
									"/LinkToDataBufferConfiguration/" +
									bufferConfigurationList[consumerLocation].first +
									"/LinkToProcessorConfiguration"
							))));
				}
				catch(const cet::exception& e)
				{
					__CFG_SS__ << "Failed to instantiate plugin named '" <<
							bufferConfigurationList[consumerLocation].first << "' of type '" <<
							bufferConfigurationList[consumerLocation].second.getNode(
															"ProcessorPluginName").getValue<std::string>()
							<< "' due to the following error: \n" << e.what() << __E__;
					__CFG_MOUT_ERR__ << ss.str();
					__CFG_SS_THROW__;
				}
				__CFG_COUT__ << bufferConfigurationList[consumerLocation].first << " has been created!" << std::endl;
			}
		}
	}
}

//========================================================================================================================
void DataManager::halt(void)
{
	stop();
	DataManager::eraseAllBuffers(); //Stop all Buffers and deletes all pointers created and given to the DataManager!
}

//========================================================================================================================
void DataManager::pause(void)
{
	__CFG_COUT__ << "Pausing..." << std::endl;
	DataManager::pauseAllBuffers();
}

//========================================================================================================================
void DataManager::resume(void)
{
	DataManager::resumeAllBuffers();
}
//========================================================================================================================
void DataManager::start(std::string runNumber)
{
	DataManager::startAllBuffers(runNumber);
}

//========================================================================================================================
void DataManager::stop()
{
	DataManager::stopAllBuffers();
}

//========================================================================================================================
void DataManager::eraseAllBuffers(void)
{
	for (auto& it : buffers_)
		deleteBuffer(it.first);

	buffers_.clear();
}

//========================================================================================================================
void DataManager::eraseBuffer(std::string bufferUID)
{
	if (deleteBuffer(bufferUID))
		buffers_.erase(bufferUID);
}

//========================================================================================================================
bool DataManager::unregisterConsumer(std::string consumerID)
{
	for (auto it = buffers_.begin(); it != buffers_.end(); it++)
		for (auto& itc : it->second.consumers_)
		{
			if (itc->getProcessorID() == consumerID)
			{
				it->second.buffer_->unregisterConsumer(itc.get());
				return true;
			}
		}

	return false;
}

//========================================================================================================================
bool DataManager::deleteBuffer(std::string bufferUID)
{
	auto it = buffers_.find(bufferUID);
	if (it != buffers_.end())
	{
		auto aBuffer = it->second;
		if (aBuffer.status_ == Running)
			stopBuffer(bufferUID);

		for (auto& itc : aBuffer.consumers_)
		{
			aBuffer.buffer_->unregisterConsumer(itc.get());
			//			delete itc;
		}
		aBuffer.consumers_.clear();
		//		for(auto& itp: aBuffer.producers_)
		//			delete itp;
		aBuffer.producers_.clear();

		delete aBuffer.buffer_;
		return true;
	}
	return false;
}

//========================================================================================================================
void DataManager::registerProducer(std::string bufferUID, DataProducerBase* producer)
{
	buffers_[bufferUID].buffer_->registerProducer(producer, producer->getBufferSize());
}

//========================================================================================================================
void DataManager::registerConsumer(std::string bufferUID, DataConsumer* consumer, bool registerToBuffer)
{
	if (registerToBuffer)
	{
		if (buffers_.find(bufferUID) == buffers_.end())
		{
			__CFG_SS__ << ("Can't find buffer UID: " + bufferUID + ". Make sure that your configuration is correct!") << std::endl;
			__CFG_SS_THROW__;
		}
		buffers_[bufferUID].buffer_->registerConsumer(consumer);
	}
}

//========================================================================================================================
//void DataManager::addConsumers(std::string bufferUID, std::vector<DataConsumer&> consumers)
//{
//
//}

//========================================================================================================================
void DataManager::startAllBuffers(std::string runNumber)
{
	for (auto it = buffers_.begin(); it != buffers_.end(); it++)
		startBuffer(it->first, runNumber);
}

//========================================================================================================================
void DataManager::stopAllBuffers(void)
{
	for (auto it = buffers_.begin(); it != buffers_.end(); it++)
		stopBuffer(it->first);
}

//========================================================================================================================
void DataManager::resumeAllBuffers(void)
{
	for (auto it = buffers_.begin(); it != buffers_.end(); it++)
		resumeBuffer(it->first);
}

//========================================================================================================================
void DataManager::pauseAllBuffers(void)
{
	for (auto it = buffers_.begin(); it != buffers_.end(); it++)
		pauseBuffer(it->first);
}

//========================================================================================================================
void DataManager::startBuffer(std::string bufferUID, std::string runNumber)
{
	buffers_[bufferUID].buffer_->reset();
	for (auto& it : buffers_[bufferUID].consumers_)
		it->startProcessingData(runNumber);
	for (auto& it : buffers_[bufferUID].producers_)
		it->startProcessingData(runNumber);
	buffers_[bufferUID].status_ = Running;
}

//========================================================================================================================
void DataManager::stopBuffer(std::string bufferUID)
{
	for (auto& it : buffers_[bufferUID].producers_)
		it->stopProcessingData();

	//Wait until all buffers are flushed
	unsigned int timeOut = 0;
	const unsigned int ratio = 100;
	const unsigned int sleepTime = 1000 * ratio;
	unsigned int totalSleepTime = sleepTime / ratio * buffers_[bufferUID].buffer_->getNumberOfBuffers();//1 milliseconds for each buffer!!!!
	if (totalSleepTime < 5000000)
		totalSleepTime = 5000000;//At least 5 seconds
	while (!buffers_[bufferUID].buffer_->isEmpty())
	{
		usleep(sleepTime);
		timeOut += sleepTime;
		if (timeOut > totalSleepTime)
		{
			std::cout << "Couldn't flush all buffers! Timing out after " << totalSleepTime / 1000000. << " seconds!" << std::endl;
			buffers_[bufferUID].buffer_->isEmpty();
			break;
		}
	}
	std::cout << "Stopping consumers, buffer MUST BE EMPTY. Is buffer empty? " << buffers_[bufferUID].buffer_->isEmpty() << std::endl;
	for (auto& it : buffers_[bufferUID].consumers_)
		it->stopProcessingData();
	buffers_[bufferUID].buffer_->reset();
	buffers_[bufferUID].status_ = Initialized;
}

//========================================================================================================================
void DataManager::resumeBuffer(std::string bufferUID)
{
	for (auto& it : buffers_[bufferUID].consumers_)
		it->resumeProcessingData();
	for (auto& it : buffers_[bufferUID].producers_)
		it->resumeProcessingData();
	buffers_[bufferUID].status_ = Running;
}

//========================================================================================================================
void DataManager::pauseBuffer(std::string bufferUID)
{
	__CFG_COUT__ << "Pausing..." << std::endl;

	for (auto& it : buffers_[bufferUID].producers_)
		it->pauseProcessingData();
	//Wait until all buffers are flushed
	unsigned int timeOut = 0;
	const unsigned int sleepTime = 1000;
	while (!buffers_[bufferUID].buffer_->isEmpty())
	{
		usleep(sleepTime);
		timeOut += sleepTime;
		if (timeOut > sleepTime*buffers_[bufferUID].buffer_->getNumberOfBuffers())//1 milliseconds for each buffer!!!!
		{
			std::cout << "Couldn't flush all buffers! Timing out after " << buffers_[bufferUID].buffer_->getNumberOfBuffers()*sleepTime / 1000000. << " seconds!" << std::endl;
			break;
		}
	}
	for (auto& it : buffers_[bufferUID].consumers_)
		it->pauseProcessingData();
	buffers_[bufferUID].status_ = Initialized;
}
