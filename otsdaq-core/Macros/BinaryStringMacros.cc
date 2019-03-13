#include "otsdaq-core/Macros/BinaryStringMacros.h"

using namespace ots;

//========================================================================================================================
// binaryToHexString
//	convert a data buffer of <len> bytes to a hex string 2*len characters long
//	Note: no preamble is applied by default (but "0x" could be nice)
//
//	Note: this is used with defaults by VisualSupervisor
std::string BinaryStringMacros::binaryToHexString(const char*        binaryBuffer,
                                                  unsigned int       numberOfBytes,
                                                  const std::string& resultPreamble,
                                                  const std::string& resultDelimiter)
{
	std::string dest;
	dest.reserve(numberOfBytes * 2);
	char hexstr[3];

	for(unsigned int i = 0; i < numberOfBytes; ++i)
	{
		sprintf(hexstr, "%02X", (unsigned char)binaryBuffer[i]);
		if(i)
			dest += resultDelimiter;
		dest += hexstr;
	}
	return resultPreamble + dest;
}  // end binaryToHexString

//========================================================================================================================
// binaryTo8ByteHexString
//	convert a data buffer string a hex string
//		8 bytes at a time with the least significant byte last.
//	Note: no preamble is applied by default (but "0x" could be nice)
std::string BinaryStringMacros::binaryTo8ByteHexString(const std::string& binaryBuffer,
                                                       const std::string& resultPreamble,
                                                       const std::string& resultDelimiter)
{
	std::string dest;
	dest.reserve(binaryBuffer.size() * 2 +
	             resultDelimiter.size() * (binaryBuffer.size() / 8) +
	             resultPreamble.size());
	char hexstr[3];

	dest += resultPreamble;

	unsigned int j = 0;
	for(; j + 8 < binaryBuffer.size(); j += 8)
	{
		if(j)
			dest += resultDelimiter;
		for(unsigned int k = 0; k < 8; ++k)
		{
			sprintf(hexstr, "%02X", (unsigned char)binaryBuffer[7 - k + j * 8]);
			dest += hexstr;
		}
	}
	for(unsigned int k = binaryBuffer.size() - 1; k >= j; --k)
	{
		sprintf(hexstr, "%02X", (unsigned char)binaryBuffer[k]);
		dest += hexstr;
		if(k == 0)
			break;  // to handle unsigned numbers when j is 0
	}

	return dest;
}  // end binaryTo8ByteHexString
