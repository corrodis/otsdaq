#include "otsdaq-core/FECore/FESlowControlsChannel.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <iostream>
#include <sstream>
#include <stdexcept> /*runtime_error*/

using namespace ots;

////////////////////////////////////
//Packet Types sent in txBuffer:
//
//			create value packet:
//			  1B type (0: value, 1: loloalarm, 2: loalarm, 3: hioalarm, 4: hihialarm)
//				1B sequence count from channel
//				8B time
//				4B sz of name
//				name
//				1B sz of value in bytes
//				1B sz of value in bits
//				value or alarm threshold value
//
////////////////////////////////////

//========================================================================================================================
FESlowControlsChannel::FESlowControlsChannel(
    const std::string &interfaceUID,
    const std::string &channelName,
    const std::string &dataType,
    unsigned int       universalDataSize,
    unsigned int       universalAddressSize,
    const std::string &universalAddress,
    unsigned int       universalDataBitOffset,

    bool	       readAccess,
    bool	       writeAccess,
    bool	       monitoringEnabled,
    bool	       recordChangesOnly,
    time_t	     delayBetweenSamples,
    bool	       saveEnabled,
    const std::string &savePath,
    const std::string &saveFileRadix,
    bool	       saveBinaryFormat,
    bool	       alarmsEnabled,
    bool	       latchAlarms,
    const std::string &lolo,
    const std::string &lo,
    const std::string &hi,
    const std::string &hihi)
    : interfaceUID_(interfaceUID), channelName_(channelName), fullChannelName_(interfaceUID_ + "/" + channelName_), dataType_(dataType), universalDataBitOffset_(universalDataBitOffset), txPacketSequenceNumber_(0),

    readAccess_(readAccess)
    , writeAccess_(writeAccess)
    , monitoringEnabled_(monitoringEnabled)
    , recordChangesOnly_(recordChangesOnly)
    , delayBetweenSamples_(delayBetweenSamples)
    , saveEnabled_(saveEnabled)
    , savePath_(savePath)
    , saveFileRadix_(saveFileRadix)
    , saveBinaryFormat_(saveBinaryFormat)
    , alarmsEnabled_(alarmsEnabled)
    , latchAlarms_(latchAlarms)
    ,

    lastSampleTime_(0)
    , loloAlarmed_(false)
    , loAlarmed_(false)
    , hiAlarmed_(false)
    , hihiAlarmed_(false)
    , saveFullFileName_(savePath_ + "/" + saveFileRadix_ + "-" + underscoreString(fullChannelName_) + "-" + std::to_string(time(0)) + (saveBinaryFormat_ ? ".dat" : ".txt"))
{
	__COUT__ << "dataType_ = " << dataType_ << std::endl;
	__COUT__ << "universalAddressSize = " << universalAddressSize << std::endl;
	__COUT__ << "universalAddress = " << universalAddress << std::endl;

	//check for valid types:
	//	if(dataType_ != "char" &&
	//				dataType_ != "short" &&
	//				dataType_ != "int" &&
	//				dataType_ != "unsigned int" &&
	//				dataType_ != "long long " &&
	//				dataType_ != "unsigned long long" &&
	//				dataType_ != "float" &&
	//				dataType_ != "double")
	//		{
	if (dataType_[dataType_.size() - 1] == 'b')  //if ends in 'b' then take that many bits
		sscanf(&dataType_[0], "%u", &sizeOfDataTypeBits_);
	else if (dataType_ == "char" || dataType_ == "unsigned char")
		sizeOfDataTypeBits_ = sizeof(char) * 8;
	else if (dataType_ == "short" || dataType_ == "unsigned short")
		sizeOfDataTypeBits_ = sizeof(short) * 8;
	else if (dataType_ == "int" || dataType_ == "unsigned int")
		sizeOfDataTypeBits_ = sizeof(int) * 8;
	else if (dataType_ == "long long" || dataType_ == "unsigned long long")
		sizeOfDataTypeBits_ = sizeof(long long) * 8;
	else if (dataType_ == "float")
		sizeOfDataTypeBits_ = sizeof(float) * 8;
	else if (dataType_ == "double")
		sizeOfDataTypeBits_ = sizeof(double) * 8;
	else
	{
		__SS__ << "Data type '" << dataType_ << "' is invalid. "
		       << "Valid data types (w/size in bytes) are as follows: "
		       << "#b (# bits)"
		       << ", char (" << sizeof(char) << "B), unsigned char (" << sizeof(unsigned char) << "B), short (" << sizeof(short) << "B), unsigned short (" << sizeof(unsigned short) << "B), int (" << sizeof(int) << "B), unsigned int (" << sizeof(unsigned int) << "B), long long (" << sizeof(long long) << "B), unsigned long long (" << sizeof(unsigned long long) << "B), float (" << sizeof(float) << "B), double (" << sizeof(double) << "B)." << std::endl;
		__COUT_ERR__ << "\n"
			     << ss.str();
		__SS_THROW__;
	}

	if (sizeOfDataTypeBits_ > 64)
	{
		__SS__ << "Invalid Data Type '" << dataType_ << "' (" << sizeOfDataTypeBits_ << "-bits)"
												". Size in bits must be less than or equal to 64-bits."
		       << std::endl;
		__COUT_ERR__ << "\n"
			     << ss.str();
		__SS_THROW__;
	}

	if (universalDataSize * 8 < sizeOfDataTypeBits_)
	{
		__SS__ << "Invalid Data Type '" << dataType_ << "' (" << sizeOfDataTypeBits_ << "-bits) or Universal Data Size of " << universalDataSize * 8 << "-bits. Data Type size must be less than or equal to Universal Data Size." << std::endl;
		__COUT_ERR__ << "\n"
			     << ss.str();
		__SS_THROW__;
	}

	universalAddress_.resize(universalAddressSize);
	convertStringToBuffer(universalAddress, universalAddress_);

	sizeOfDataTypeBytes_ = (sizeOfDataTypeBits_ / 8 +
				((universalDataBitOffset_ % 8) ? 1 : 0));

	lolo_.resize(sizeOfDataTypeBytes_);
	convertStringToBuffer(lolo, lolo_, true);
	lo_.resize(sizeOfDataTypeBytes_);
	convertStringToBuffer(lo, lo_, true);
	hi_.resize(sizeOfDataTypeBytes_);
	convertStringToBuffer(hi, hi_, true);
	hihi_.resize(sizeOfDataTypeBytes_);
	convertStringToBuffer(hihi, hihi_, true);

	//prepare for data to come
	sample_.resize(sizeOfDataTypeBytes_);
	lastSample_.resize(sizeOfDataTypeBytes_);
}

//========================================================================================================================
FESlowControlsChannel::~FESlowControlsChannel(void)
{
}

//========================================================================================================================
//underscoreString
//	replace all non-alphanumeric with underscore
std::string FESlowControlsChannel::underscoreString(const std::string &str)
{
	std::string retStr;
	retStr.reserve(str.size());
	for (unsigned int i = 0; i < str.size(); ++i)
		if (
		    (str[i] >= 'a' && str[i] <= 'z') ||
		    (str[i] >= 'A' && str[i] <= 'Z') ||
		    (str[i] >= '0' && str[i] <= '9'))
			retStr.push_back(str[i]);
		else
			retStr.push_back('_');
	return retStr;
}

//========================================================================================================================
//convertStringToBuffer
//	adds to txBuffer if sample should be sent to monitor server
//	if useDataType == false, then assume unsigned long long
//
// 	Note: buffer is expected to sized properly in advance, e.g. buffer.resize(#)
void FESlowControlsChannel::convertStringToBuffer(const std::string &inString, std::string &buffer, bool useDataType)
{
	__COUT__ << "Input Str Sz= \t" << inString.size() << std::endl;
	__COUT__ << "Input Str Val= \t'" << inString << "'" << std::endl;
	__COUT__ << "Output buffer Sz= \t" << buffer.size() << std::endl;

	if (useDataType &&
	    (dataType_ == "float" || dataType_ == "double"))
	{
		__COUT__ << "Floating point spec'd" << std::endl;
		if (dataType_ == "float" && buffer.size() == sizeof(float))
		{
			sscanf(&inString[0], "%f", (float *)&buffer[0]);
			__COUT__ << "float: " << *((float *)&buffer[0]) << std::endl;
		}
		else if (dataType_ == "double" && buffer.size() == sizeof(double))
		{
			sscanf(&inString[0], "%lf", (double *)&buffer[0]);
			__COUT__ << "double: " << *((double *)&buffer[0]) << std::endl;
		}
		else
		{
			__SS__ << "Invalid floating point spec! "
			       << "dataType_=" << dataType_ << " buffer.size()=" << buffer.size() << std::endl;
			__COUT_ERR__ << "\n"
				     << ss.str();
			__SS_THROW__;
		}

		{  //print
			__SS__ << "0x ";
			for (int i = (int)buffer.size() - 1; i >= 0; --i)
				ss << std::hex << (int)((buffer[i] >> 4) & 0xF) << (int)((buffer[i]) & 0xF) << " " << std::dec;
			ss << std::endl;
			__COUT__ << "\n"
				 << ss.str();
		}
		return;
	}

	//clear buffer
	for (unsigned int i = 0; i < buffer.size(); ++i)
		buffer[i] = 0;

	//	{ //print
	//		__SS__ << "0x ";
	//		for(int i=(int)buffer.size()-1;i>=0;--i)
	//			ss << std::hex << (int)((buffer[i]>>4)&0xF) <<
	//			(int)((buffer[i])&0xF) << " " << std::dec;
	//		ss << std::endl;
	//		__COUT__ << "\n" << ss.str();
	//	}

	if (inString.size())
	{
		if (inString.size() > 2 && inString[0] == '0' && inString[1] == 'x')
		{
			//hex value

			__COUT__ << "Hex." << std::endl;

			unsigned int  j;
			unsigned char val;
			for (unsigned int i = 0; i < inString.size(); ++i)
			{
				j = (inString.size() - 1 - i);
				if (inString[i] >= '0' &&
				    inString[i] <= '9')
					val = inString[i] - 48;
				else if (inString[i] >= 'A' &&
					 inString[i] <= 'F')
					val = inString[i] - 55;
				else if (inString[i] >= 'a' &&
					 inString[i] <= 'f')
					val = inString[i] - 87;
				else
					val = 0;

				buffer[j / 2] |= val << ((j % 2) * 4);
			}
		}
		else  //treat as decimal value
		{
			//assume not bigger than 64 bits if decimal

			__COUT__ << "Decimal." << std::endl;
			unsigned long long val;

			if (!useDataType || dataType_[0] == 'u')  //then use unsigned long long
			{
				sscanf(&inString[0], "%llu", &val);
			}
			else  //use long long
			{
				sscanf(&inString[0], "%lld", (long long *)&val);
			}
			//transfer the long long to the universal address
			for (unsigned int i = 0; i < sizeof(long long) && i < buffer.size(); ++i)
				buffer[i] = ((char *)&val)[i];
			//assume rest of buffer bytes are 0, since decimal value was given
		}
	}

	{  //print
		__SS__ << "0x ";
		for (int i = (int)buffer.size() - 1; i >= 0; --i)
			ss << std::hex << (int)((buffer[i] >> 4) & 0xF) << (int)((buffer[i]) & 0xF) << " " << std::dec;
		ss << std::endl;
		__COUT__ << "\n"
			 << ss.str();
	}
}

//========================================================================================================================
//handleSample
//	adds to txBuffer if sample should be sent to monitor server
void FESlowControlsChannel::handleSample(const std::string &universalReadValue, std::string &txBuffer,
					 FILE *fpAggregate, bool aggregateIsBinaryFormat)
{
	__COUT__ << "txBuffer size=" << txBuffer.size() << std::endl;

	//extract sample from universalReadValue
	//	considering bit size and offset
	extractSample(universalReadValue);  //sample_ = universalReadValue

	//behavior:
	//	if recordChangesOnly
	//		if no change and not first value
	//			return
	//
	//	for interesting value
	//		if monitoringEnabled
	//			add packet to buffer
	//			if alarmsEnabled
	//				for each alarm add packet to buffer
	//		if localSavingEnabled
	//			append to file

	//////////////////////////////////////////////////

	if (recordChangesOnly_)
	{
		if (lastSampleTime_ &&
		    lastSample_ == sample_)
		{
			__COUT__ << "no change." << std::endl;
			return;  //no change
		}
	}

	__COUT__ << "new value!" << std::endl;

	//else we have an interesting value!
	lastSampleTime_ = time(0);
	lastSample_     = sample_;

	char alarmMask = 0;

	/////////////////////////////////////////////
	/////////////////////////////////////////////
	/////////////////////////////////////////////
	if (monitoringEnabled_)
	{
		//create value packet:
		//  1B type (0: value, 1: loloalarm, 2: loalarm, 3: hioalarm, 4: hihialarm)
		//	1B sequence count from channel
		//	8B time
		//	4B sz of name
		//	name
		//	1B sz of value in bytes
		//	1B sz of value in bits
		//	value

		__COUT__ << "before txBuffer sz=" << txBuffer.size() << std::endl;
		txBuffer.push_back(0);				//value type
		txBuffer.push_back(txPacketSequenceNumber_++);  //sequence counter and increment

		txBuffer.resize(txBuffer.size() + sizeof(lastSampleTime_));
		memcpy(&txBuffer[txBuffer.size() - sizeof(lastSampleTime_)], &lastSampleTime_, sizeof(lastSampleTime_));

		unsigned int tmpSz = fullChannelName_.size();

		txBuffer.resize(txBuffer.size() + sizeof(tmpSz));
		memcpy(&txBuffer[txBuffer.size() - sizeof(tmpSz)], &tmpSz, sizeof(tmpSz));

		txBuffer += fullChannelName_;

		txBuffer.push_back((unsigned char)sample_.size());       //size in bytes
		txBuffer.push_back((unsigned char)sizeOfDataTypeBits_);  //size in bits

		txBuffer += sample_;
		__COUT__ << "after txBuffer sz=" << txBuffer.size() << std::endl;

		{  //print
			__SS__ << "txBuffer: \n";
			for (unsigned int i = 0; i < txBuffer.size(); ++i)
			{
				ss << std::hex << (int)((txBuffer[i] >> 4) & 0xF) << (int)((txBuffer[i]) & 0xF) << " " << std::dec;
				if (i % 8 == 7) ss << std::endl;
			}
			ss << std::endl;
			__COUT__ << "\n"
				 << ss.str();
		}
	}

	//check alarms
	if (alarmsEnabled_)
		alarmMask = checkAlarms(txBuffer);

	//create array helper for saving
	std::string *alarmValueArray[] = {
	    &lolo_, &lo_, &hi_, &hihi_};

	/////////////////////////////////////////////
	/////////////////////////////////////////////
	/////////////////////////////////////////////
	if (fpAggregate)  //aggregate file means saving enabled at FE level
	{
		//append to file
		if (aggregateIsBinaryFormat)
		{
			//save binary format:
			//	8B time (save any alarms by marking with time = 1, 2, 3, 4)
			//	4B sz of name
			//	name
			//	1B sz of value in bytes
			//	1B sz of value in bits
			//	value or alarm threshold

			__COUT__ << "Aggregate Binary File Format: " << sizeof(lastSampleTime_) << " " << sample_.size() << std::endl;

			{
				fwrite(&lastSampleTime_, sizeof(lastSampleTime_), 1, fpAggregate);  //	8B time

				unsigned int tmpSz = fullChannelName_.size();
				fwrite(&tmpSz, sizeof(tmpSz), 1, fpAggregate);				//4B sz of name
				fwrite(&fullChannelName_[0], fullChannelName_.size(), 1, fpAggregate);  //name

				unsigned char tmpChar = (unsigned char)sample_.size();
				fwrite(&tmpChar, 1, 1, fpAggregate);  //size in bytes

				tmpChar = (unsigned char)sizeOfDataTypeBits_;
				fwrite(&tmpChar, 1, 1, fpAggregate);		      //size in bits
				fwrite(&sample_[0], sample_.size(), 1, fpAggregate);  //value
			}

			//save any alarms by marking with time 1, 2, 3, 4
			if (alarmMask)  //if any alarms
				for (time_t i = 1; i < 5; ++i, alarmMask >>= 1)
					if (alarmMask & 1)
					{
						{
							fwrite(&i, sizeof(lastSampleTime_), 1, fpAggregate);  //	8B save any alarms by marking with time = 1, 2, 3, 4

							unsigned int tmpSz = fullChannelName_.size();
							fwrite(&tmpSz, sizeof(tmpSz), 1, fpAggregate);				//4B sz of name
							fwrite(&fullChannelName_[0], fullChannelName_.size(), 1, fpAggregate);  //name

							unsigned char tmpChar = (unsigned char)sample_.size();
							fwrite(&tmpChar, 1, 1, fpAggregate);  //size in bytes

							tmpChar = (unsigned char)sizeOfDataTypeBits_;
							fwrite(&tmpChar, 1, 1, fpAggregate);							  //size in bits
							fwrite(&(*alarmValueArray[i - 1])[0], (*alarmValueArray[i - 1]).size(), 1, fpAggregate);  //alarm threshold
						}
					}
		}
		else
		{
			//save text format:
			//	timestamp (save any alarms by marking with time 1, 2, 3, 4)
			//	name
			//	value

			__COUT__ << "Aggregate Text File Format: " << dataType_ << std::endl;

			fprintf(fpAggregate, "%lu\n", lastSampleTime_);
			fprintf(fpAggregate, "%s\n", fullChannelName_.c_str());

			if (dataType_[dataType_.size() - 1] == 'b')  //if ends in 'b' then take that many bits
			{
				std::stringstream ss;
				ss << "0x";
				for (unsigned int i = 0; i < sample_.size(); ++i)
					ss << std::hex << (int)((sample_[i] >> 4) & 0xF) << (int)((sample_[i]) & 0xF) << std::dec;
				fprintf(fpAggregate, "%s\n", ss.str().c_str());
			}
			else if (dataType_ == "char")
				fprintf(fpAggregate, "%d\n", *((char *)(&sample_[0])));
			else if (dataType_ == "unsigned char")
				fprintf(fpAggregate, "%u\n", *((unsigned char *)(&sample_[0])));
			else if (dataType_ == "short")
				fprintf(fpAggregate, "%d\n", *((short *)(&sample_[0])));
			else if (dataType_ == "unsigned short")
				fprintf(fpAggregate, "%u\n", *((unsigned short *)(&sample_[0])));
			else if (dataType_ == "int")
				fprintf(fpAggregate, "%d\n", *((int *)(&sample_[0])));
			else if (dataType_ == "unsigned int")
				fprintf(fpAggregate, "%u\n", *((unsigned int *)(&sample_[0])));
			else if (dataType_ == "long long")
				fprintf(fpAggregate, "%lld\n", *((long long *)(&sample_[0])));
			else if (dataType_ == "unsigned long long")
				fprintf(fpAggregate, "%llu\n", *((unsigned long long *)(&sample_[0])));
			else if (dataType_ == "float")
				fprintf(fpAggregate, "%f\n", *((float *)(&sample_[0])));
			else if (dataType_ == "double")
				fprintf(fpAggregate, "%f\n", *((double *)(&sample_[0])));

			//save any alarms by marking with time 1, 2, 3, 4
			if (alarmMask)  //if any alarms
			{
				char checkMask = 1;  //use mask to maintain alarm mask
				for (time_t i = 1; i < 5; ++i, checkMask <<= 1)
					if (alarmMask & checkMask)
					{
						fprintf(fpAggregate, "%lu\n", i);
						fprintf(fpAggregate, "%s\n", fullChannelName_.c_str());

						if (dataType_[dataType_.size() - 1] == 'b')  //if ends in 'b' then take that many bits
						{
							std::stringstream ss;
							ss << "0x";
							for (unsigned int j = 0; j < (*alarmValueArray[i - 1]).size(); ++i)
								ss << std::hex << (int)(((*alarmValueArray[i - 1])[j] >> 4) & 0xF) << (int)(((*alarmValueArray[i - 1])[j]) & 0xF) << std::dec;
							fprintf(fpAggregate, "%s\n", ss.str().c_str());
						}
						else if (dataType_ == "char")
							fprintf(fpAggregate, "%d\n", *((char *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned char")
							fprintf(fpAggregate, "%u\n", *((unsigned char *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "short")
							fprintf(fpAggregate, "%d\n", *((short *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned short")
							fprintf(fpAggregate, "%u\n", *((unsigned short *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "int")
							fprintf(fpAggregate, "%d\n", *((int *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned int")
							fprintf(fpAggregate, "%u\n", *((unsigned int *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "long long")
							fprintf(fpAggregate, "%lld\n", *((long long *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned long long")
							fprintf(fpAggregate, "%llu\n", *((unsigned long long *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "float")
							fprintf(fpAggregate, "%f\n", *((float *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "double")
							fprintf(fpAggregate, "%f\n", *((double *)(&(*alarmValueArray[i - 1])[0])));
					}
			}
		}
	}

	/////////////////////////////////////////////
	/////////////////////////////////////////////
	/////////////////////////////////////////////
	if (saveEnabled_)
	{
		FILE *fp = fopen(saveFullFileName_.c_str(), saveBinaryFormat_ ? "ab" : "a");
		if (!fp)
		{
			__COUT_ERR__ << "Failed to open slow controls channel file: " << saveFullFileName_ << std::endl;
			return;
		}

		//append to file
		if (saveBinaryFormat_)
		{
			__COUT__ << "Binary File Format: " << sizeof(lastSampleTime_) << " " << sample_.size() << std::endl;
			fwrite(&lastSampleTime_, sizeof(lastSampleTime_), 1, fp);
			fwrite(&sample_[0], sample_.size(), 1, fp);

			//save any alarms by marking with time 1, 2, 3, 4
			if (alarmMask)  //if any alarms
				for (time_t i = 1; i < 5; ++i, alarmMask >>= 1)
					if (alarmMask & 1)
					{
						fwrite(&i, sizeof(lastSampleTime_), 1, fp);
						fwrite(&(*alarmValueArray[i - 1])[0], (*alarmValueArray[i - 1]).size(), 1, fp);
					}
		}
		else
		{
			__COUT__ << "Text File Format: " << dataType_ << std::endl;

			fprintf(fp, "%lu\n", lastSampleTime_);

			if (dataType_[dataType_.size() - 1] == 'b')  //if ends in 'b' then take that many bits
			{
				std::stringstream ss;
				ss << "0x";
				for (unsigned int i = 0; i < sample_.size(); ++i)
					ss << std::hex << (int)((sample_[i] >> 4) & 0xF) << (int)((sample_[i]) & 0xF) << std::dec;
				fprintf(fp, "%s\n", ss.str().c_str());
			}
			else if (dataType_ == "char")
				fprintf(fp, "%d\n", *((char *)(&sample_[0])));
			else if (dataType_ == "unsigned char")
				fprintf(fp, "%u\n", *((unsigned char *)(&sample_[0])));
			else if (dataType_ == "short")
				fprintf(fp, "%d\n", *((short *)(&sample_[0])));
			else if (dataType_ == "unsigned short")
				fprintf(fp, "%u\n", *((unsigned short *)(&sample_[0])));
			else if (dataType_ == "int")
				fprintf(fp, "%d\n", *((int *)(&sample_[0])));
			else if (dataType_ == "unsigned int")
				fprintf(fp, "%u\n", *((unsigned int *)(&sample_[0])));
			else if (dataType_ == "long long")
				fprintf(fp, "%lld\n", *((long long *)(&sample_[0])));
			else if (dataType_ == "unsigned long long")
				fprintf(fp, "%llu\n", *((unsigned long long *)(&sample_[0])));
			else if (dataType_ == "float")
				fprintf(fp, "%f\n", *((float *)(&sample_[0])));
			else if (dataType_ == "double")
				fprintf(fp, "%f\n", *((double *)(&sample_[0])));

			//save any alarms by marking with time 1, 2, 3, 4
			if (alarmMask)  //if any alarms
			{
				char checkMask = 1;  //use mask to maintain alarm mask
				for (time_t i = 1; i < 5; ++i, checkMask <<= 1)
					if (alarmMask & checkMask)
					{
						fprintf(fp, "%lu\n", i);

						if (dataType_[dataType_.size() - 1] == 'b')  //if ends in 'b' then take that many bits
						{
							std::stringstream ss;
							ss << "0x";
							for (unsigned int j = 0; j < (*alarmValueArray[i - 1]).size(); ++i)
								ss << std::hex << (int)(((*alarmValueArray[i - 1])[j] >> 4) & 0xF) << (int)(((*alarmValueArray[i - 1])[j]) & 0xF) << std::dec;
							fprintf(fp, "%s\n", ss.str().c_str());
						}
						else if (dataType_ == "char")
							fprintf(fp, "%d\n", *((char *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned char")
							fprintf(fp, "%u\n", *((unsigned char *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "short")
							fprintf(fp, "%d\n", *((short *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned short")
							fprintf(fp, "%u\n", *((unsigned short *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "int")
							fprintf(fp, "%d\n", *((int *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned int")
							fprintf(fp, "%u\n", *((unsigned int *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "long long")
							fprintf(fp, "%lld\n", *((long long *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "unsigned long long")
							fprintf(fp, "%llu\n", *((unsigned long long *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "float")
							fprintf(fp, "%f\n", *((float *)(&(*alarmValueArray[i - 1])[0])));
						else if (dataType_ == "double")
							fprintf(fp, "%f\n", *((double *)(&(*alarmValueArray[i - 1])[0])));
					}
			}
		}

		fclose(fp);
	}
}

//========================================================================================================================
//extractSample
//	extract sample from universalReadValue
//		considering bit size and offset
void FESlowControlsChannel::extractSample(const std::string &universalReadValue)
{
	//procedure:

	{  //print
		__SS__ << "Univ Read: ";
		for (unsigned int i = 0; i < universalReadValue.size(); ++i)
			ss << std::hex << (int)((universalReadValue[i] >> 4) & 0xF) << (int)((universalReadValue[i]) & 0xF) << " " << std::dec;
		ss << std::endl;
		__COUT__ << "\n"
			 << ss.str();
	}

	unsigned int byteOffset = universalDataBitOffset_ / 8;
	unsigned int bitOffset  = universalDataBitOffset_ % 8;
	unsigned int bitsLeft   = sizeOfDataTypeBits_;

	//copy to tmp (assume sample max size is 64 bits)
	unsigned long long tmp = 0;

	if (bitOffset)  //copy first partial byte
	{
		//		__COUT__ << "bitsLeft=" << bitsLeft <<
		//				" byteOffset=" << byteOffset <<
		//				" bitOffset=" << bitOffset <<
		//				std::endl;

		tmp      = ((unsigned long long)(universalReadValue[byteOffset++])) >> bitOffset;
		bitsLeft = 8 - bitOffset;

		//		{ //print
		//			__SS__ << "Tmp: ";
		//			for(unsigned int i=0; i<sizeof(tmp); ++i)
		//				ss << std::hex << (int)((((char *)&tmp)[i]>>4)&0xF) <<
		//				(int)((((char *)&tmp)[i])&0xF) << " " << std::dec;
		//			ss << std::endl;
		//			__COUT__ << "\n" << ss.str();
		//		}
	}

	while (bitsLeft > 7)  //copy whole bytes
	{
		//		__COUT__ << "bitsLeft=" << bitsLeft <<
		//				" byteOffset=" << byteOffset <<
		//				" bitOffset=" << bitOffset <<
		//				std::endl;

		tmp |= ((unsigned long long)(universalReadValue[byteOffset++])) << (sizeOfDataTypeBits_ - bitsLeft);
		bitsLeft -= 8;

		//		{ //print
		//			__SS__ << "Tmp: ";
		//			for(unsigned int i=0; i<sizeof(tmp); ++i)
		//				ss << std::hex << (int)((((char *)&tmp)[i]>>4)&0xF) <<
		//				(int)((((char *)&tmp)[i])&0xF) << " " << std::dec;
		//			ss << std::endl;
		//			__COUT__ << "\n" << ss.str();
		//		}
	}

	if (bitOffset)  //copy last partial byte
	{
		//		__COUT__ << "bitsLeft=" << bitsLeft <<
		//				" byteOffset=" << byteOffset <<
		//				" bitOffset=" << bitOffset <<
		//				std::endl;

		tmp |= (((unsigned long long)(universalReadValue[byteOffset])) &
			(0xFF >> (8 - bitsLeft)))
		       << (sizeOfDataTypeBits_ - bitsLeft);

		//		{ //print
		//			__SS__ << "Tmp: ";
		//			for(unsigned int i=0; i<sizeof(tmp); ++i)
		//				ss << std::hex << (int)((((char *)&tmp)[i]>>4)&0xF) <<
		//				(int)((((char *)&tmp)[i])&0xF) << " " << std::dec;
		//			ss << std::endl;
		//			__COUT__ << "\n" << ss.str();
		//		}
	}

	__COUT__ << "Temp Long Long Sample: " << tmp << std::endl;

	sample_.resize(0);  //clear a la sample_ = "";
	for (unsigned int i = 0; i < (sizeOfDataTypeBits_ / 8 +
				      ((universalDataBitOffset_ % 8) ? 1 : 0));
	     ++i)
		sample_.push_back(((char *)&tmp)[i]);

	__COUT__ << "sample_.size()= " << sample_.size() << std::endl;

	{  //print
		__SS__ << "Sample: ";
		for (unsigned int i = 0; i < sample_.size(); ++i)
			ss << std::hex << (int)((sample_[i] >> 4) & 0xF) << (int)((sample_[i]) & 0xF) << " " << std::dec;
		ss << std::endl;
		__COUT__ << "\n"
			 << ss.str();
	}
}

//========================================================================================================================
//clearAlarms
//	clear target alarm
// 	if -1 clear all
void FESlowControlsChannel::clearAlarms(int targetAlarm)
{
	if (targetAlarm == -1 || targetAlarm == 0)
		loloAlarmed_ = false;
	if (targetAlarm == -1 || targetAlarm == 1)
		loAlarmed_ = false;
	if (targetAlarm == -1 || targetAlarm == 2)
		hiAlarmed_ = false;
	if (targetAlarm == -1 || targetAlarm == 3)
		hihiAlarmed_ = false;
}

//========================================================================================================================
//checkAlarms
//	if current value is a new alarm, set alarmed and add alarm packet to buffer
//
//	return mask of alarms that fired during this check.
char FESlowControlsChannel::checkAlarms(std::string &txBuffer)
{
	//procedure:
	//	for each alarm type
	//		determine type and get value as proper type
	//			i.e.:
	//				0 -- unsigned long long
	//				1 -- long long
	//				2 -- float
	//				3 -- double
	//			do comparison with the type
	//				alarm alarms

	int  useType	  = -1;
	char createPacketMask = 0;

	if (dataType_[dataType_.size() - 1] == 'b')  //if ends in 'b' then take that many bits
		useType = 0;
	else if (dataType_ == "char")
		useType = 1;
	else if (dataType_ == "unsigned char")
		useType = 0;
	else if (dataType_ == "short")
		useType = 1;
	else if (dataType_ == "unsigned short")
		useType = 0;
	else if (dataType_ == "int")
		useType = 1;
	else if (dataType_ == "unsigned int")
		useType = 0;
	else if (dataType_ == "long long")
		useType = 1;
	else if (dataType_ == "unsigned long long")
		useType = 0;
	else if (dataType_ == "float")
		useType = 2;
	else if (dataType_ == "double")
		useType = 3;

	if (useType == 0)  //unsigned long long
	{
		__COUT__ << "Using unsigned long long for alarms." << std::endl;
		//lolo
		if ((!loloAlarmed_ || !latchAlarms_) &&
		    *((unsigned long long *)&sample_[0]) <=
			*((unsigned long long *)&lolo_[0]))
		{
			loloAlarmed_ = true;
			createPacketMask |= 1 << 0;
		}
		//lo
		if ((!loAlarmed_ || !latchAlarms_) &&
		    *((unsigned long long *)&sample_[0]) <=
			*((unsigned long long *)&lo_[0]))
		{
			loAlarmed_ = true;
			createPacketMask |= 1 << 1;
		}
		//hi
		if ((!hiAlarmed_ || !latchAlarms_) &&
		    *((unsigned long long *)&sample_[0]) >=
			*((unsigned long long *)&hi_[0]))
		{
			hiAlarmed_ = true;
			createPacketMask |= 1 << 2;
		}
		//hihi
		if ((!hihiAlarmed_ || !latchAlarms_) &&
		    *((unsigned long long *)&sample_[0]) >=
			*((unsigned long long *)&hihi_[0]))
		{
			hihiAlarmed_ = true;
			createPacketMask |= 1 << 3;
		}
	}
	else if (useType == 1)  //long long
	{
		__COUT__ << "Using long long for alarms." << std::endl;
		//lolo
		if ((!loloAlarmed_ || !latchAlarms_) &&
		    *((long long *)&sample_[0]) <=
			*((long long *)&lolo_[0]))
		{
			loloAlarmed_ = true;
			createPacketMask |= 1 << 0;
		}
		//lo
		if ((!loAlarmed_ || !latchAlarms_) &&
		    *((long long *)&sample_[0]) <=
			*((long long *)&lo_[0]))
		{
			loAlarmed_ = true;
			createPacketMask |= 1 << 1;
		}
		//hi
		if ((!hiAlarmed_ || !latchAlarms_) &&
		    *((long long *)&sample_[0]) >=
			*((long long *)&hi_[0]))
		{
			hiAlarmed_ = true;
			createPacketMask |= 1 << 2;
		}
		//hihi
		if ((!hihiAlarmed_ || !latchAlarms_) &&
		    *((long long *)&sample_[0]) >=
			*((long long *)&hihi_[0]))
		{
			hihiAlarmed_ = true;
			createPacketMask |= 1 << 3;
		}
	}
	else if (useType == 2)  //float
	{
		__COUT__ << "Using float for alarms." << std::endl;
		//lolo
		if ((!loloAlarmed_ || !latchAlarms_) &&
		    *((float *)&sample_[0]) <=
			*((float *)&lolo_[0]))
		{
			loloAlarmed_ = true;
			createPacketMask |= 1 << 0;
		}
		//lo
		if ((!loAlarmed_ || !latchAlarms_) &&
		    *((float *)&sample_[0]) <=
			*((float *)&lo_[0]))
		{
			loAlarmed_ = true;
			createPacketMask |= 1 << 1;
		}
		//hi
		if ((!hiAlarmed_ || !latchAlarms_) &&
		    *((float *)&sample_[0]) >=
			*((float *)&hi_[0]))
		{
			hiAlarmed_ = true;
			createPacketMask |= 1 << 2;
		}
		//hihi
		if ((!hihiAlarmed_ || !latchAlarms_) &&
		    *((float *)&sample_[0]) >=
			*((float *)&hihi_[0]))
		{
			hihiAlarmed_ = true;
			createPacketMask |= 1 << 3;
		}
	}
	else if (useType == 3)  //double
	{
		__COUT__ << "Using double for alarms." << std::endl;
		//lolo
		if ((!loloAlarmed_ || !latchAlarms_) &&
		    *((double *)&sample_[0]) <=
			*((double *)&lolo_[0]))
		{
			loloAlarmed_ = true;
			createPacketMask |= 1 << 0;
		}
		//lo
		if ((!loAlarmed_ || !latchAlarms_) &&
		    *((double *)&sample_[0]) <=
			*((double *)&lo_[0]))
		{
			loAlarmed_ = true;
			createPacketMask |= 1 << 1;
		}
		//hi
		if ((!hiAlarmed_ || !latchAlarms_) &&
		    *((double *)&sample_[0]) >=
			*((double *)&hi_[0]))
		{
			hiAlarmed_ = true;
			createPacketMask |= 1 << 2;
		}
		//hihi
		if ((!hihiAlarmed_ || !latchAlarms_) &&
		    *((double *)&sample_[0]) >=
			*((double *)&hihi_[0]))
		{
			hihiAlarmed_ = true;
			createPacketMask |= 1 << 3;
		}
	}

	//create array helper for packet gen
	std::string *alarmValueArray[] = {
	    &lolo_, &lo_, &hi_, &hihi_};

	if (monitoringEnabled_)  //only create packet if monitoring
	{
		//create a packet for each bit in mask
		char checkMask = 1;  //use mask to maintain return mask
		for (int i = 0; i < 4; ++i, checkMask <<= 1)
			if (createPacketMask & checkMask)
			{
				//create alarm packet:
				//  1B type (0: value, 1: loloalarm, 2: loalarm, 3: hioalarm, 4: hihialarm)
				//	1B sequence count from channel
				//	8B time
				//	4B sz of name
				//	name
				//	1B sz of alarm value in bytes
				//	1B sz of alarm value in bits
				//	alarm value

				__COUT__ << "Create packet type " << i + 1 << " alarm value = " << *alarmValueArray[i] << std::endl;

				__COUT__ << "before txBuffer sz=" << txBuffer.size() << std::endl;
				txBuffer.push_back(i + 1);			//alarm packet type
				txBuffer.push_back(txPacketSequenceNumber_++);  //sequence counter and increment

				txBuffer.resize(txBuffer.size() + sizeof(lastSampleTime_));
				memcpy(&txBuffer[txBuffer.size() - sizeof(lastSampleTime_)], &lastSampleTime_, sizeof(lastSampleTime_));

				unsigned int tmpSz = fullChannelName_.size();

				txBuffer.resize(txBuffer.size() + sizeof(tmpSz));
				memcpy(&txBuffer[txBuffer.size() - sizeof(tmpSz)], &tmpSz, sizeof(tmpSz));

				txBuffer += fullChannelName_;

				txBuffer.push_back((unsigned char)(*alarmValueArray[i]).size());  //size in bytes
				txBuffer.push_back((unsigned char)sizeOfDataTypeBits_);		  //size in bits

				txBuffer += (*alarmValueArray[i]);
				__COUT__ << "after txBuffer sz=" << txBuffer.size() << std::endl;

				{  //print
					__SS__ << "txBuffer: \n";
					for (unsigned int i = 0; i < txBuffer.size(); ++i)
					{
						ss << std::hex << (int)((txBuffer[i] >> 4) & 0xF) << (int)((txBuffer[i]) & 0xF) << " " << std::dec;
						if (i % 8 == 7) ss << std::endl;
					}
					ss << std::endl;
					__COUT__ << "\n"
						 << ss.str();
				}
			}
	}

	return createPacketMask;
}
