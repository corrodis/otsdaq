#include "artdaq-ots/Generators/UDPReceiver.hh"

#include "canvas/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq-ots/Overlays/UDPFragmentWriter.hh"
#include "artdaq-ots/Overlays/FragmentType.hh"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"


#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <sys/poll.h>

ots::UDPReceiver::UDPReceiver(fhicl::ParameterSet const & ps)
  : CommandableFragmentGenerator(ps)
  , rawOutput_(ps.get<bool>("raw_output_enabled",false))
  , rawPath_(ps.get<std::string>("raw_output_path", "/tmp"))
  , dataport_(ps.get<int>("port",6343))
  , ip_(ps.get<std::string>("ip","127.0.0.1"))
  , expectedPacketNumber_(0)
  , sendCommands_(ps.get<bool>("send_OtsUDP_commands",false))
  , fragmentWindow_(ps.get<double>("fragment_time_window_ms", 1000))
  , lastFrag_(std::chrono::high_resolution_clock::now())
{
  datasocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(!datasocket_) 
	{
	  throw art::Exception(art::errors::Configuration) << "UDPReceiver: Error creating socket!" << std::endl;
	  exit(1);
	}

  struct sockaddr_in si_me_data;
  si_me_data.sin_family = AF_INET;
  si_me_data.sin_port = htons(dataport_);
  si_me_data.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(datasocket_, (struct sockaddr *)&si_me_data, sizeof(si_me_data)) == -1)
	{
      throw art::Exception(art::errors::Configuration) << 
        "UDPReceiver: Cannot bind data socket to port " << dataport_ << std::endl;
      exit(1);
	}

  si_data_.sin_family = AF_INET;
  si_data_.sin_port = htons(dataport_);
  if(inet_aton(ip_.c_str(), &si_data_.sin_addr) == 0) 
	{
	  throw art::Exception(art::errors::Configuration) << 
		"UDPReceiver: Could not translate provided IP Address: " << ip_ << "\n";
	  exit(1);
	}
  mf::LogInfo("UDPReceiver") << "UDP Receiver Construction Complete!" << std::endl;
}

ots::UDPReceiver::~UDPReceiver()
{
  receiverThread_.join();
}

void ots::UDPReceiver::start() {


	std::cout << __COUT_HDR_FL__ << "Start." << std::endl;
	mf::LogInfo("UDPReceiver") << "Starting..." << std::endl;

	receiverThread_ = std::thread(&UDPReceiver::receiveLoop_,this);
}

void ots::UDPReceiver::receiveLoop_() {

  uint8_t droppedPackets = 0;
  while(!should_stop()) {
    struct pollfd ufds[1];
    ufds[0].fd = datasocket_;
    ufds[0].events = POLLIN | POLLPRI;

    int rv = poll(ufds, 1, 1000);
    if(rv > 0) 
	  {
		std::cout << __COUT_HDR_FL__ << "revents: " << ufds[0].revents << ", " << std::endl;// ufds[1].revents << std::endl;
		if(ufds[0].revents == POLLIN || ufds[0].revents == POLLPRI) 
		  {

			//FIXME -> IN THE STIB GENERATOR WE DON'T HAVE A HEADER
			//FIXME -> IN THE STIB GENERATOR WE DON'T HAVE A HEADER
			//FIXME -> IN THE STIB GENERATOR WE DON'T HAVE A HEADER
			uint32_t peekBuffer[6];
			recvfrom(datasocket_, peekBuffer, sizeof(peekBuffer), MSG_PEEK,
					 (struct sockaddr *) &si_data_, (socklen_t*)sizeof(si_data_));

			mf::LogInfo("UDPReceiver") << "Received UDP Packet with sequence number " << std::hex << "0x" << (int)peekBuffer[1] << "!" << std::dec;
			std::cout << __COUT_HDR_FL__ << "peekBuffer[1] == expectedPacketNumber_: " << std::hex << (int)peekBuffer[1] << " =?= " << (int)expectedPacketNumber_ << std::endl;

			uint32_t seqNum = peekBuffer[5];
			//ReturnCode dataCode = getReturnCode(peekBuffer[0]);
			if(seqNum >= expectedPacketNumber_ || (seqNum < 10 && expectedPacketNumber_ > 240) || droppedPackets > 0 || expectedPacketNumber_ - seqNum > 20)
			  {
				if(seqNum != expectedPacketNumber_ && (seqNum >= expectedPacketNumber_ || (seqNum < 10 && expectedPacketNumber_ > 200)))
				  {
					int deltaHi = seqNum - expectedPacketNumber_;
					int deltaLo = 4294967295 + seqNum - expectedPacketNumber_;
					droppedPackets += deltaLo;// < 255 ? deltaLo : deltaHi;
					mf::LogWarning("UDPReceiver") << "Dropped/Delayed packets detected: " << std::dec << std::to_string(droppedPackets);
					expectedPacketNumber_ = seqNum;
				  } else if (seqNum != expectedPacketNumber_) {
				  int delta = expectedPacketNumber_ - seqNum;
				  mf::LogWarning("UDPReceiver") << std::dec << "Sequence Number significantly different than expected! (delta: " << delta << ")";
				}
				
				packetBuffer_t* buffer = new packetBuffer_t();
				memset(&((*buffer).at(0)),0,sizeof(packetBuffer_t));
				int DJN= recvfrom(datasocket_, &(*buffer).at(0), sizeof(packetBuffer_t), 0,(struct sockaddr *) &si_data_, (socklen_t*)sizeof(si_data_));
				mf::LogInfo("DJN UDPReceiver") << "UDP packet recvfrom got Nbytes="  << (int)DJN << " into buffer.";
				
				if(droppedPackets == 0) {
					std::lock_guard<std::mutex> lock(receiveBufferLock_);
				  receiveBuffers_.emplace_back(std::unique_ptr<packetBuffer_t>(buffer));
				}
				else {
				  bool found = false;
				  for(packetBuffer_list_t::reverse_iterator it = packetBuffers_.rbegin(); it != packetBuffers_.rend(); ++it) {
					if(seqNum < (*it)->at(1)) {
					std::lock_guard<std::mutex> lock(receiveBufferLock_);
					receiveBuffers_.emplace(it.base(), std::unique_ptr<packetBuffer_t>(buffer));
					  droppedPackets--;
					  expectedPacketNumber_--;
					  found = true;
					}
				  }
				  if(!found) {
					std::lock_guard<std::mutex> lock(receiveBufferLock_);
					receiveBuffers_.emplace_back(std::unique_ptr<packetBuffer_t>(buffer));
				  }
				}
				mf::LogInfo("UDPReceiver") << "Now placing UDP packet with sequence number " << std::hex << (int)seqNum << " into buffer." << std::dec;
			  }
  
			++expectedPacketNumber_;
		  }
	  }
    std::cout << __COUT_HDR_FL__ << "waiting..." << std::endl;

  }
  mf::LogInfo("UDPReceiver") << "receive Loop exiting..." << std::endl;
}

bool ots::UDPReceiver::getNext_(artdaq::FragmentPtrs & output)
{
  if (should_stop()) {
    return false;
  }

  while(receiveBuffers_.size() == 0 && isTimerExpired_()) { usleep(1000); }

  {
	std::lock_guard<std::mutex> lock(receiveBufferLock_);
	  std::move(receiveBuffers_.begin(), receiveBuffers_.end(), std::inserter(packetBuffers_,packetBuffers_.end()));
      receiveBuffers_.clear();
  }
  mf::LogInfo("UDPReceiver") << "Calling ProcessData";
  ProcessData_(output);

  mf::LogInfo("UDPReceiver") << "Returning output of size " << output.size() << " to TriggeredFragmentGenerator";
  return true;
}

  void ots::UDPReceiver::ProcessData_(artdaq::FragmentPtrs & output) {

  ots::UDPFragment::Metadata metadata;
  metadata.port = dataport_;
  metadata.address = si_data_.sin_addr.s_addr;
 
  std::size_t initial_payload_size = 0;

  output.emplace_back( artdaq::Fragment::FragmentBytes(initial_payload_size,  
						      ev_counter(), fragment_id(),
						      ots::detail::FragmentType::UDP, metadata) );
  // We now have a fragment to contain this event:
  ots::UDPFragmentWriter thisFrag(*output.back());

  std::cout << __COUT_HDR_FL__ << "Received data, now placing data with UDP sequence number "
            << std::hex << static_cast<int>((*packetBuffers_.front()).at(1)) 
            << " into UDPFragment" << std::endl;
  thisFrag.resize(64050 * packetBuffers_.size() + 1);
  std::ofstream rawOutput;
  if(rawOutput_) {
    std::string outputPath = rawPath_ + "/UDPReceiver-"+ ip_ + ":" + std::to_string(dataport_) + ".bin";
    rawOutput.open(outputPath, std::ios::out | std::ios::app | std::ios::binary );
  }

  DataType dataType = getDataType((*packetBuffers_.front()).at(0));
  thisFrag.set_hdr_type((int)dataType);
  int pos = 0;
  for(auto jj = packetBuffers_.begin(); jj != packetBuffers_.end(); ++jj) {
      for(int ii = 0; ii < 64050; ++ii) {
        // Null-terminate string types
        if((*jj)->at(ii) == 0 && (dataType == DataType::JSON || dataType == DataType::String)) { break; }

        if(rawOutput_) rawOutput.write((char*)&((*jj)->at(ii)), sizeof(uint8_t));
        *(thisFrag.dataBegin() + pos) = (*jj)->at(ii);
        ++pos;
      }
    }
  packetBuffers_.clear();

  if(dataType == DataType::JSON || dataType == DataType::String) {
    *(thisFrag.dataBegin() + pos) = 0;
    char zero =0;
    if(rawOutput_) rawOutput.write(&zero, sizeof(char));
  }
    if(rawOutput_) rawOutput.close();
}

void ots::UDPReceiver::send(CommandType command)
{
	if(sendCommands_) {
		CommandPacket packet;
		packet.type = command;
		packet.dataSize = 0;
		sendto(datasocket_,&packet,sizeof(packet),0, (struct sockaddr *) &si_data_, sizeof(si_data_));
	}
}

bool ots::UDPReceiver::isTimerExpired_()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration<double, std::milli>(now - lastFrag_).count();
  return diff > fragmentWindow_;
}

void ots::UDPReceiver::stop()
{
#pragma message "Using default implementation of UDPReceiver::stop()"
}

void ots::UDPReceiver::stopNoMutex()
{
#pragma message "Using default implementation of UDPReceiver::stopNoMutex()"
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::UDPReceiver)
