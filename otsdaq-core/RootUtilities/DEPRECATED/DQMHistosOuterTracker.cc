#include "otsdaq-core/RootUtilities/DQMHistosOuterTracker.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/DataDecoders/FSSRData.h"
#include "otsdaq-core/DataDecoders/VIPICData.h"
#include "otsdaq-core/NetworkUtilities/NetworkConverters.h"

#include "otsdaq-core/ConfigurationDataFormats/FEInterfaceTableBase.h"
#include "otsdaq-core/TablePluginDataFormats/DataBufferTable.h"
#include "otsdaq-core/TablePluginDataFormats/DataManagerTable.h"
#include "otsdaq-core/TablePluginDataFormats/DetectorTable.h"
#include "otsdaq-core/TablePluginDataFormats/DetectorToFETable.h"
#include "otsdaq-core/TablePluginDataFormats/FETable.h"
#include "otsdaq-core/TablePluginDataFormats/UDPDataListenerProducerTable.h"
#include "otsdaq-core/TablePluginDataFormats/UDPDataStreamerConsumerTable.h"
//#include
//"otsdaq-demo/UserConfigurationDataFormats/FEWROtsUDPFSSRInterfaceTable.h"
//#include
//"otsdaq-demo/UserConfigurationDataFormats/FEWRPurdueFSSRInterfaceTable.h"

#include <iostream>
#include <sstream>
#include <string>

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TFrame.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TROOT.h>
#include <TRandom.h>
#include <TStyle.h>
#include <TThread.h>

#include <stdint.h>
//#include <arpa/inet.h>

// ROOT documentation
// http://root.cern.ch/root/html/index.html

using namespace ots;

//========================================================================================================================
DQMHistosOuterTracker::DQMHistosOuterTracker(std::string supervisorApplicationUID,
                                             std::string bufferUID,
                                             std::string processorUID)
    : theFile_(0)
    , theDataDecoder_(supervisorApplicationUID, bufferUID, processorUID)
    , supervisorApplicationUID_(supervisorApplicationUID)
    , bufferUID_(bufferUID)
    , processorUID_(processorUID)
{
	gStyle->SetPalette(1);
}

//========================================================================================================================
DQMHistosOuterTracker::~DQMHistosOuterTracker(void) { closeFile(); }

//========================================================================================================================
void DQMHistosOuterTracker::openFile(std::string fileName)
{
	closeFile();
	currentDirectory_ = 0;
	theFile_          = TFile::Open(fileName.c_str(), "RECREATE");
	theFile_->cd();
}

//========================================================================================================================
void DQMHistosOuterTracker::book()
{
	std::cout << __COUT_HDR_FL__ << "Booking start!" << std::endl;

	currentDirectory_ = theFile_->mkdir("General", "General");
	currentDirectory_->cd();
	numberOfTriggers_ = new TH1I("NumberOfTriggers", "Number of triggers", 1, -0.5, 0.5);
	currentDirectory_ = theFile_->mkdir("Planes", "Planes");
	currentDirectory_->cd();
	std::stringstream name;
	std::stringstream title;
	// FIXME
	const FEConfiguration* feConfiguration =
	    theConfigurationManager_->__GET_CONFIG__(FEConfiguration);
	const DetectorToFEConfiguration* detectorToFEConfiguration =
	    theConfigurationManager_->__GET_CONFIG__(DetectorToFEConfiguration);
	const DataManagerConfiguration* dataManagerConfiguration =
	    theConfigurationManager_->__GET_CONFIG__(DataManagerConfiguration);
	const DataBufferConfiguration* dataBufferConfiguration =
	    theConfigurationManager_->__GET_CONFIG__(DataBufferConfiguration);
	const UDPDataStreamerConsumerConfiguration* dataStreamerConsumerConfiguration =
	    theConfigurationManager_->__GET_CONFIG__(UDPDataStreamerConsumerConfiguration);
	const DetectorConfiguration* detectorConfiguration =
	    theConfigurationManager_->__GET_CONFIG__(DetectorConfiguration);

	//    const std::string   supervisorType_;
	//    const unsigned int  supervisorInstance_;
	//    const unsigned int  bufferUID_;
	//    const std::string   processorUID_;

	std::stringstream        processName;
	std::vector<std::string> bufferList =
	    dataManagerConfiguration->getListOfDataBuffers();
	std::vector<std::string> interfaceList = feConfiguration->getListOfFEIDs();
	for(const auto& itInterfaces : interfaceList)
	{
		std::string streamToIP;
		std::string streamToPort;

		// RAR -- on June 28
		//	Eric changed this to Base.. but this functionality does not belong in
		// ConfiguraitonBase 	If LORE wants this functionality then move to otsdaq_demo
		// repository since it depends on 		interfaces in demo (Or think about how to
		// extract this dependency?.. should there be an 		intermediate class that
		// interfaces have to inherit from to have DQM functionality?)
		if(feConfiguration->getFEInterfaceType(itInterfaces).find("FSSRInterface") !=
		   std::string::npos)
		{
			const TableBase* interfaceConfiguration =
			    theConfigurationManager_->getTableByName(
			        feConfiguration->getFEInterfaceType(itInterfaces) + "Configuration");
			auto feinterfaceConfiguration =
			    dynamic_cast<const FEInterfaceTableBase*>(interfaceConfiguration);
			if(feinterfaceConfiguration)
			{
				streamToIP =
				    feinterfaceConfiguration->getStreamingIPAddress(itInterfaces);
				streamToPort = feinterfaceConfiguration->getStreamingPort(itInterfaces);
			}
		}

		for(const auto& itBuffers : bufferList)
		{
			std::vector<std::string> producerList =
			    dataBufferConfiguration->getProducerIDList(itBuffers);
			std::vector<std::string> consumerList =
			    dataBufferConfiguration->getConsumerIDList(itBuffers);
			for(const auto& itProducers : producerList)
			{
				if(dataBufferConfiguration->getProducerClass(itBuffers, itProducers) ==
				   "UDPDataListenerProducer")
				{
					const UDPDataListenerProducerConfiguration* listerConfiguration =
					    static_cast<const UDPDataListenerProducerConfiguration*>(
					        theConfigurationManager_->getTableByName(
					            "UDPDataListenerProducerConfiguration"));
					if(listerConfiguration->getIPAddress(itProducers) == streamToIP &&
					   listerConfiguration->getPort(itProducers) == streamToPort)
					{
						for(const auto& itConsumers : consumerList)
						{
							std::cout << __COUT_HDR_FL__
							          << "CONSUMER LIST: " << itConsumers << std::endl;
							if(dataBufferConfiguration->getConsumerClass(
							       itBuffers, itConsumers) == "UDPDataStreamerConsumer")
							{
								// FIXME This is very bad since I am getting the PC IP
								// Hardcoded!!!!!!!
								std::string ipAddress = NetworkConverters::nameToStringIP(
								    dataStreamerConsumerConfiguration->getIPAddress(
								        itConsumers));
								// FIXME the streamer should have it's own configured port
								// and not fedport +1
								std::string port =
								    NetworkConverters::unsignedToStringPort(
								        dataStreamerConsumerConfiguration->getPort(
								            itConsumers));
								planeOccupancies_[ipAddress][port] =
								    std::map<unsigned int, TH1*>();
								std::cout << __COUT_HDR_FL__ << "IP: "
								          << NetworkConverters::stringToNameIP(ipAddress)
								          << " Port:----"
								          << NetworkConverters::stringToUnsignedPort(port)
								          << "----" << std::endl;
								const std::vector<std::string> rocList =
								    detectorToFEConfiguration->getFEReaderDetectorList(
								        itInterfaces);
								std::cout << __COUT_HDR_FL__
								          << "List of rocs for FER: " << itInterfaces
								          << " has size: " << rocList.size() << std::endl;
								for(const auto& itROC : rocList)
								{
									std::string ROCType =
									    detectorConfiguration->getDetectorType(itROC);
									// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__
									// << "ROC type: " << ROCType << std::endl;
									if(ROCType == "FSSR")
									{
										unsigned int fedChannel =
										    detectorToFEConfiguration->getFEReaderChannel(
										        itROC);
										if(planeOccupancies_[ipAddress][port].find(
										       fedChannel) ==
										   planeOccupancies_[ipAddress][port].end())
										{
											name.str("");
											title.str("");
											name << "Plane_FE" << itInterfaces
											     << "_Channel" << fedChannel
											     << "_Occupancy";
											title << "Plane FE" << itInterfaces
											      << " Channel" << fedChannel
											      << " Occupancy";
											std::cout << __COUT_HDR_FL__
											          << "Adding:" << name.str()
											          << std::endl;
											planeOccupancies_[ipAddress][port]
											                 [fedChannel] = new TH1F(
											                     name.str().c_str(),
											                     title.str().c_str(),
											                     640,
											                     -0.5,
											                     639.5);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	/*
	for(const auto& it: consumerList)
	{
	    std::cout << __COUT_HDR_FL__ << "CONSUMER LIST: " << it << std::endl;
	    if((dataBufferConfiguration->getConsumerClass(itBuffers,it) ==
"UDPDataStreamerConsumer") && (it.find("Purdue") != std::string::npos || it.find("OtsUDP")
!= std::string::npos))
	    {
	        //FIXME This is very bad since I am getting the PC IP Hardcoded!!!!!!!
	        std::string ipAddress =
NetworkConverters::nameToStringIP(dataStreamerConsumerConfiguration->getIPAddress(it));
	        //FIXME the streamer should have it's own configured port and not fedport +1
	        std::string port =
NetworkConverters::unsignedToStringPort(dataStreamerConsumerConfiguration->getPort(it));
	        planeOccupancies_[ipAddress][port] = std::map<unsigned int, TH1*);
	        std::cout << __COUT_HDR_FL__ << "IP: " <<
NetworkConverters::stringToNameIP(ipAddress) << " Port:----" <<
NetworkConverters::stringToUnsignedPort(port) << "----" << std::endl;
	        std::vector<std::string> interfaceList = feConfiguration->getListOfFEIDs();
	        for(const auto& it: interfaceList)
	        {
	            std::cout << __COUT_HDR_FL__ << "Configuring histos for FER: " << it <<
std::endl; if(feConfiguration->getFEInterfaceType(it).find( "FSSRInterface" ) !=
std::string::npos)
	            {
	                //const OtsUDPFERConfiguration* OtsUDPFERConfiguration =
theConfigurationManager_->__GET_CONFIG__(OtsUDPFERConfiguration); const
std::vector<std::string> rocList = DetectorToFEConfiguration->getFERROCsList(it);
	                std::cout << __COUT_HDR_FL__ << "List of rocs for FER: " << it  << "
has size: " << rocList.size() << std::endl; for(const auto& itROC: rocList)
	                {
	                    std::string ROCType = detectorConfiguration->getROCType(itROC);
	                    //std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "ROC
type: " << ROCType << std::endl; if(ROCType == "FSSR")
	                    {
	                        unsigned int fedChannel =
DetectorToFEConfiguration->getFERChannel(itROC);
	                        if(planeOccupancies_[ipAddress][port].find(fedChannel) ==
planeOccupancies_[ipAddress][port].end())
	                        {
	                            name.str("");
	                            title.str("");
	                            name << "Plane_FE" << it << "_Channel"<< fedChannel <<
"_Occupancy"; title << "Plane FE" << it << " Channel"<< fedChannel << " Occupancy";
	                            std::cout << __COUT_HDR_FL__ << "Adding:" << name.str() <<
std::endl; planeOccupancies_[ipAddress][port][fedChannel] = new TH1F(name.str().c_str(),
title.str().c_str(), 640, -0.5, 639.5);
	                        }
	                    }
	                    else if (ROCType == "VIPIC")
	                    {
	                        unsigned int fedChannel =
DetectorToFEConfiguration->getFERChannel(itROC);
	                        if(planeOccupancies_[ipAddress][port].find(fedChannel) ==
planeOccupancies_[ipAddress][port].end())
	                        {
	                            name.str("");
	                            title.str("");
	                            name << "Plane_FER" << it << "_Channel"<< fedChannel <<
"_Occupancy"; title << "Plane FER" << it << " Channel"<< fedChannel << " Occupancy";
	                            std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
"Adding:" << name << std::endl; planeOccupancies_[ipAddress][port][fedChannel] = new
TH2F(name.str().c_str(), title.str().c_str(), 64, -0.5, 63.5, 64, -0.5, 63.5);
	                        }
	                    }
	                }
	            }
	        }
	    }

	}
}
	 */
	std::cout << __COUT_HDR_FL__ << "Booking done!" << std::endl;
	/*
	    for(unsigned int p=0; p<4; p++)
	    {
	        name.str("");
	        title.str("");
	        name << "Plane_" << p << "_Occupancy";
	        title << "Plane " << p << " Occupancy";
	        planeOccupancies_.push_back(new TH1I(name.str().c_str(), title.str().c_str(),
	   640, 0, 640));
	    }
	 */

	// Create a new canvas
	// canvas_ = new TCanvas("MainCanvas", "Main Canvas", 200, 10, 700, 500);
	// canvas_->SetFillColor(42);
	// canvas_->GetFrame()->SetFillColor(21);
	// canvas_->GetFrame()->SetBorderSize(6);
	// canvas_->GetFrame()->SetBorderMode(-1);
	// directory_->Append(canvas_);

	// Create a 1-D, 2-D and a profile histogram
	// histo1D_ = new TH1F("Histo1D", "This is the px distribution", 100, -4, 4);
	// histo2D_ = new TH2F("Histo2D", "py vs px", 40, -4, 4, 40, -4, 4);
	// profile_ = new TProfile("Profile", "Profile of pz versus px", 100, -4, 4, 0, 20);
	//  gDirectory->ls();

	//  Set canvas/frame attributes (save old attributes)
	// histo1D_->SetFillColor(48);

	//	  gROOT->cd();
}

//========================================================================================================================
void DQMHistosOuterTracker::fill(std::string&                       buffer,
                                 std::map<std::string, std::string> header)
{
	// std::cout << __COUT_HDR_FL__ << buffer.length() << std::endl;
	// int triggerNumber = 0;
	// int triggerHigh = 0;
	// int triggerLow = 0;
	// unsigned long  ipAddress = (((header["IPAddress"][0]&0xff)<<24) +
	// ((header["IPAddress"][1]&0xff)<<16) + ((header["IPAddress"][2]&0xff)<<8) +
	// (header["IPAddress"][3]&0xff)) & 0xffffffff;
	std::string ipAddress = header["IPAddress"];
	std::string port      = header["Port"];

	// std::cout << __COUT_HDR_FL__ << "Got data from IP: " <<
	// NetworkConverters::stringToNameIP(ipAddress) << " port: " <<
	// NetworkConverters::stringToUnsignedPort(port) << std::endl;

	if(NetworkConverters::stringToUnsignedPort(port) == 48003)
		theDataDecoder_.convertBuffer(buffer, convertedBuffer_, false);
	else
		theDataDecoder_.convertBuffer(buffer, convertedBuffer_, true);
	// std::cout << __COUT_HDR_FL__ << "New buffer" << std::endl;
	unsigned int bufferCounter = 0;
	uint32_t     oldData       = 0;
	while(!convertedBuffer_.empty())
	{
		// if (!theDataDecoder_.isBCOHigh(convertedBuffer_.front())
		//    && !theDataDecoder_.isBCOLow(convertedBuffer_.front())
		//    && !theDataDecoder_.isTrigger(convertedBuffer_.front()))

		if(NetworkConverters::stringToUnsignedPort(port) == 48003 &&
		   bufferCounter % 2 == 1)
		{
			convertedBuffer_.pop();
			bufferCounter++;
			continue;
		}
		// if (NetworkConverters::stringToUnsignedPort(port) == 48003 && bufferCounter%2
		// == 0) 	std::cout << __COUT_HDR_FL__ << "Data: " << std::hex <<
		// convertedBuffer_.front() << std::dec << std::endl;
		if(theDataDecoder_.isFSSRData(convertedBuffer_.front()))
		{
			//			if(oldData != 0 && oldData==convertedBuffer_.front())
			//				std::cout << __COUT_HDR_FL__ << "There is a copy FSSR: " <<
			// std::hex
			//<< convertedBuffer_.front() << " counter: " << bufferCounter << std::dec <<
			// std::endl;
			oldData                    = convertedBuffer_.front();
			FSSRData* detectorDataFSSR = 0;
			theDataDecoder_.decodeData(convertedBuffer_.front(),
			                           (DetectorDataBase**)&detectorDataFSSR);
			// if(detectorData->getChannelNumber() >= 4)
			//{
			//    std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Wrong channel: "
			//    << detectorData->getChannelNumber() << std::endl; return;
			//}
			// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Receiving data
			// from: " << hex << ipAddress << " port: " << port << std::endl;
			if(planeOccupancies_.find(ipAddress) != planeOccupancies_.end() &&
			   planeOccupancies_[ipAddress].find(port) !=
			       planeOccupancies_[ipAddress].end() &&
			   planeOccupancies_[ipAddress][port].find(
			       detectorDataFSSR->getChannelNumber()) !=
			       planeOccupancies_[ipAddress][port].end())
				planeOccupancies_[ipAddress][port][detectorDataFSSR->getChannelNumber()]
				    ->Fill(detectorDataFSSR->getSensorStrip());
			else
				std::cout << __COUT_HDR_FL__
				          << "ERROR: I haven't book histos for streamer "
				          << NetworkConverters::stringToNameIP(ipAddress)
				          << " port number: "
				          << NetworkConverters::stringToUnsignedPort(port)
				          << " channel: " << detectorDataFSSR->getChannelNumber()
				          << " data: " << std::hex << convertedBuffer_.front() << std::dec
				          << std::endl;
			//    }
			// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << "
			// filling histograms!" << std::endl;
			//  Float_t px, py, pz;
			//  const Int_t kUPDATE = 1000;
			//   std::cout << __COUT_HDR_FL__ << mthn << "Filling..." << std::endl;
			/*
			  for (Int_t i = 1; i <= 1000; i++)
			    {
			      gRandom->Rannor(px, py);
			      pz = px * px + py * py;
			      histo1D_->Fill(px);
			      histo2D_->Fill(px, py);
			      profile_->Fill(px, pz);
			    }
			 */
		}
		//		if (theDataDecoder_.isVIPICData(convertedBuffer_.front()))
		//		{
		//			VIPICData* detectorDataVIPIC = 0;
		//			theDataDecoder_.decodeData(convertedBuffer_.front(),
		//(DetectorDataBase**)&detectorDataVIPIC);
		//			//if(detectorData->getChannelNumber() >= 4)
		//			//{
		//			//    std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Wrong
		// channel: " << detectorData->getChannelNumber() << std::endl;
		//			//    return;
		//			//}
		//			//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Receiving
		// data  from: " << hex << ipAddress << " port: " << port << std::endl;
		// if
		// ( 					planeOccupancies_.find(ipAddress) !=
		// planeOccupancies_.end()
		//					&& planeOccupancies_[ipAddress].find(port) !=
		// planeOccupancies_[ipAddress].end()
		//					&&
		// planeOccupancies_[ipAddress][port].find(detectorDataVIPIC->getChannelNumber())
		//!= planeOccupancies_[ipAddress][port].end()
		//			)
		//				planeOccupancies_[ipAddress][port][detectorDataVIPIC->getChannelNumber()]->Fill(detectorDataVIPIC->getCol(),
		// detectorDataVIPIC->getRow()); 			else 				std::cout <<
		//__COUT_HDR_FL__ << "ERROR: I haven't book histos for streamer " <<
		// NetworkConverters::stringToNameIP(ipAddress)
		//			<< " port number: " << NetworkConverters::stringToUnsignedPort(port)
		//			<< " port std::string:----" << port << "-----"
		//			<< " channel: " << detectorDataVIPIC->getChannelNumber() << std::endl;
		//			//    }
		//			//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_
		//<< " filling histograms!" << std::endl;
		//			//  Float_t px, py, pz;
		//			//  const Int_t kUPDATE = 1000;
		//			//   std::cout << __COUT_HDR_FL__ << mthn << "Filling..." <<
		// std::endl;
		//			/*
		//              for (Int_t i = 1; i <= 1000; i++)
		//                {
		//                  gRandom->Rannor(px, py);
		//                  pz = px * px + py * py;
		//                  histo1D_->Fill(px);
		//                  histo2D_->Fill(px, py);
		//                  profile_->Fill(px, pz);
		//                }
		//			 */
		//		}

		/*
		else if (theDataDecoder_.isTriggerLow(convertedBuffer_.front()))
		  triggerLow = convertedBuffer_.front();
		else if (theDataDecoder_.isTriggerHigh(convertedBuffer_.front()))
		  {
		  triggerHigh = convertedBuffer_.front();
		  triggerNumber = theDataDecoder_.mergeTriggerHighAndLow(triggerHigh,triggerLow);
		  numberOfTriggers_->Fill(0);
		  //std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Number of Triggers: "
		<< triggerNumber << std::endl;
		  //if(triggerNumber != numberOfTriggers_->GetEntries())
		  //  std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
		"---------------------------- ERROR: different trigger number " << triggerNumber
		<< " entries: " << numberOfTriggers_->GetEntries() << std::endl;
		  }
		 */
		convertedBuffer_.pop();
		bufferCounter++;
	}
	/*
	  std::cout << __COUT_HDR_FL__ << "base: " <<  typeid(detectorData).name()
	      << "=" << typeid(FSSRData*).name()
	      << " d cast: " << dynamic_cast<FSSRData*>(detectorData)
	      << " r cast: " << static_cast<FSSRData*>(detectorData)
	      << std::endl;
	  if (dynamic_cast<FSSRData*>(detectorData) != 0)
	    {
	      FSSRData* data = (FSSRData*)detectorData;
	 */
	//      std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Strip: " << std::endl;
	//      std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Strip: " <<
	//      detectorData->getStripNumber() << std::endl;
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Strip: " <<
	// detectorData->getChannelNumber() << std::endl;
}

//========================================================================================================================
void DQMHistosOuterTracker::save(void)
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Saving file!" << std::endl;
	if(theFile_ != 0)
		theFile_->Write();
}

//========================================================================================================================
void DQMHistosOuterTracker::load(std::string fileName)
{
	closeFile();
	theFile_ = TFile::Open(
	    fileName.c_str());  // WebPath/js/visualizers_lib/tmpRootData/Suca.root");
	if(!theFile_->IsOpen())
		return;
	theFile_->cd();
	numberOfTriggers_ = (TH1I*)theFile_->Get("General/NumberOfTriggers");

	std::string       directory = "Planes";
	std::stringstream name;
	for(unsigned int p = 0; p < 4; p++)
	{
		name.str("");
		name << directory << "/Plane_" << p << "_Occupancy";
		// FIXME Must organize better all histograms!!!!!
		// planeOccupancies_.push_back((TH1I*)theFile_->Get(name.str().c_str()));
	}
	// canvas_ = (TCanvas*) theFile_->Get("MainDirectory/MainCanvas");
	// histo1D_ = (TH1F*) theFile_->Get("MainDirectory/Histo1D");
	// histo2D_ = (TH2F*) theFile_->Get("MainDirectory/Histo2D");
	// profile_ = (TProfile*) theFile_->Get("MainDirectory/Profile");
	closeFile();
}

//========================================================================================================================
void DQMHistosOuterTracker::closeFile(void)
{
	if(theFile_ != 0)
	{
		theFile_->Close();
		theFile_ = 0;
	}
}

//========================================================================================================================
TObject* DQMHistosOuterTracker::get(std::string name)
{
	if(theFile_ != 0)
		return theFile_->Get(name.c_str());
	return 0;
}
