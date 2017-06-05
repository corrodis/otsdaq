#include "otsdaq-core/DataDecoders/DataDecoder.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>
#include <cassert>
#include <unistd.h>

using namespace ots;



//========================================================================================================================
DataDecoder::DataDecoder(void)
{}

//========================================================================================================================
DataDecoder::~DataDecoder(void)
{}

//========================================================================================================================
void DataDecoder::convertBuffer(const std::string& buffer, std::queue<uint32_t>& convertedBuffer, bool invert)
{
	for(unsigned int i=0; i<buffer.length(); i+=4 )
		convertedBuffer.push(convertBuffer(buffer, i, invert));
}

//========================================================================================================================
uint32_t DataDecoder::convertBuffer(const std::string& buffer, unsigned int bufferIndex, bool invert)
{
	if(invert)
		return   (unsigned int)(((unsigned char)buffer[bufferIndex  ]) << 24)
				+ (unsigned int)(((unsigned char)buffer[bufferIndex+1]) << 16)
				+ (unsigned int)(((unsigned char)buffer[bufferIndex+2]) << 8)
				+ (unsigned int)( (unsigned char)buffer[bufferIndex+3]);
	else
		return   (unsigned int)((unsigned char)buffer[bufferIndex  ])
				+ (unsigned int)(((unsigned char)buffer[bufferIndex+1]) << 8)
				+ (unsigned int)(((unsigned char)buffer[bufferIndex+2]) << 16)
				+ (unsigned int)(((unsigned char)buffer[bufferIndex+3]) << 24);

}

//========================================================================================================================
bool DataDecoder::isBCOHigh(uint32_t data)
{
	return bcoDataDecoder_.isBCOHigh(data);
}

//========================================================================================================================
bool DataDecoder::isBCOLow(uint32_t data)
{
	return bcoDataDecoder_.isBCOLow(data);
}

//========================================================================================================================
bool DataDecoder::isTriggerLow(uint32_t data)
{
	return triggerDataDecoder_.isTriggerLow(data);
}

//========================================================================================================================
bool DataDecoder::isTriggerHigh(uint32_t data)
{
	return triggerDataDecoder_.isTriggerHigh(data);
}

//========================================================================================================================
bool DataDecoder::isFSSRData(uint32_t data)
{
	return FSSRDataDecoder_.isFSSR(data);
}

//========================================================================================================================
bool DataDecoder::isVIPICData(uint32_t data)
{
	return VIPICDataDecoder_.isVIPIC(data);
}

//========================================================================================================================
bool DataDecoder::isPSI46Data(uint32_t data)
{
	return PSI46DataDecoder_.isPSI46(data);
}

//========================================================================================================================
bool DataDecoder::isPSI46DigData(uint32_t data)
{
	return PSI46DigDataDecoder_.isPSI46Dig(data);
}

//========================================================================================================================
uint64_t DataDecoder::mergeBCOHighAndLow(uint32_t dataBCOHigh, uint32_t dataBCOLow)
{
	return bcoDataDecoder_.mergeBCOHighAndLow(bcoDataDecoder_.decodeBCOHigh(dataBCOHigh),bcoDataDecoder_.decodeBCOLow(dataBCOLow));
}

//========================================================================================================================
uint64_t DataDecoder::mergeTriggerHighAndLow(uint32_t dataTriggerHigh, uint32_t dataTriggerLow)
{
	return triggerDataDecoder_.mergeTriggerHighAndLow(triggerDataDecoder_.decodeTriggerHigh(dataTriggerHigh),triggerDataDecoder_.decodeTriggerLow(dataTriggerLow));
}

//========================================================================================================================
void DataDecoder::insertBCOHigh(uint64_t& bco, uint32_t dataBCOHigh)
{
	bcoDataDecoder_.insertBCOHigh(bco,dataBCOHigh);
}

//========================================================================================================================
void DataDecoder::insertBCOLow(uint64_t& bco, uint32_t dataBCOLow)
{
	bcoDataDecoder_.insertBCOLow(bco,dataBCOLow);
}

//========================================================================================================================
void DataDecoder::decodeData(uint32_t data, DetectorDataBase** decodedData)
{
	if(isFSSRData(data))
	{
		FSSRDataDecoder_.decode(data);
		*decodedData = (DetectorDataBase*)(&FSSRDataDecoder_);
	}
	else if(isVIPICData(data))
	{
		VIPICDataDecoder_.decode(data);
		*decodedData = (DetectorDataBase*)(&VIPICDataDecoder_);
	}
	else if(isPSI46DigData(data))
	{
		PSI46DigDataDecoder_.decode(data);
		*decodedData = (DetectorDataBase*)(&PSI46DigDataDecoder_);
	}
	else
		*decodedData = 0;
}
