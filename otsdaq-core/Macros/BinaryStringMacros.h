#ifndef _ots_BinaryStringMacros_h_
#define _ots_BinaryStringMacros_h_

#include "otsdaq-core/Macros/CoutMacros.h"

#include <map>
#include <set>
#include <typeinfo>  // operator typeid
#include <vector>

namespace ots
{
struct BinaryStringMacros
{
  private:  //private constructor because all static members, should never instantiate this class
	BinaryStringMacros(void);
	~BinaryStringMacros(void);

  public:
	//Here is the list of static helper functions:
	//
	//		binaryToHexString
	//		binaryTo8ByteHexString
	//

	static std::string binaryToHexString(const char* binaryBuffer, unsigned int numberOfBytes, const std::string& resultPreamble = "", const std::string& resultDelimiter = "");
	static std::string binaryTo8ByteHexString(const std::string& binaryBuffer, const std::string& resultPreamble = "0x", const std::string& resultDelimiter = " ");

	template<class T>
	static void insertValueInBinaryString(
	    std::string& binaryBuffer, T value, unsigned int byteIndex = 0, unsigned int bitIndex = 0)
	{
		__COUTV__(sizeof(value));

		__COUT__ << "Original size of binary buffer: " << binaryBuffer.size() << __E__;

		byteIndex += bitIndex / 8;
		bitIndex %= 8;

		binaryBuffer.resize(
		    byteIndex +
		    sizeof(value) +
		    (bitIndex ? 1 : 0));

		if (bitIndex)
		{
			//special handling for bit shift on every byte

			unsigned char keepMask = ((unsigned char)(-1)) >> bitIndex;

			//first byte is a mix of what's there and value
			binaryBuffer[byteIndex] =
			    (binaryBuffer[byteIndex] & keepMask) |
			    (((unsigned char*)(&value))[0] << bitIndex);

			//middle bytes are mix of value i-1 and i
			for (unsigned int i = 1; i < sizeof(value); ++i)
			{
			}

			//last bytes is mix of value and what's there
		}

		memcpy(
		    (void*)&binaryBuffer[byteIndex + 0] /*dest*/,
		    (void*)&value /*src*/,
		    sizeof(value) /*numOfBytes*/);
		__COUT__ << "Final size of binary buffer: " << binaryBuffer.size() << __E__;

	}  //end insertValueInBinaryString
	//specialized for string value
	static void insertValueInBinaryString(
	    std::string& binaryBuffer, const std::string& value, unsigned int byteIndex = 0, unsigned int bitIndex = 0)
	{
	}

};  //end BinaryStringMacros static class
}  // namespace ots
#endif
