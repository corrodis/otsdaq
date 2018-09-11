#ifndef _ots_String_Macros_h_
#define _ots_String_Macros_h_

#include "otsdaq-core/Macros/CoutMacros.h"

#include <vector>
#include <set>
#include <map>
#include <typeinfo>		// operator typeid

namespace ots
{

struct StringMacros
{

	//Here is the list of static helper functions:
	//
	//		wildCardMatch
	//		inWildCardSet
	//		getWildCardMatchFromMap
	//
	//		decodeURIComponent
	//		convertEnvironmentVariables
	//		isNumber
	//		extractAndCalculateNumber
	//		validateValueForDefaultStringDataType
	//
	//		getSetFromString
	//		getVectorFromString
	//		getMapFromString
	//
	//		setToString
	//		vectorToString
	//		mapToString
	//
	//		demangleTypeName

static bool					wildCardMatch							(const std::string& needle, 		const std::string& 							haystack,	unsigned int* priorityIndex = 0);
static bool					inWildCardSet							(const std::string  needle, 		const std::set<std::string>& 				haystack);


//========================================================================================================================
//getWildCardMatchFromMap ~
//	returns value if needle is in haystack otherwise throws exception
//	(considering wildcards AND match priority as defined by StringMacros::wildCardMatch)
template<class T>
static const T&				getWildCardMatchFromMap					(const std::string  needle, 		const std::map<std::string,T>& 				haystack)
{
	unsigned int matchPriority, bestMatchPriority = 0; //lowest number matters most for matches

	if(!haystack.size())
	{
		__SS__ << "Needle '" << needle << "' not found in EMPTY wildcard haystack:" << __E__;
		__SS_ONLY_THROW__;
	}

	//__COUT__ << StringMacros::mapToString(haystack) << __E__;

	std::string bestMatchKey;
	for(const auto& haystackPair : haystack)
		//use wildcard match, flip needle parameter.. because we want haystack to have the wildcards
		//	check resulting priority to see if we are done with search (when priority=1, can be done)
		if(StringMacros::wildCardMatch(haystackPair.first,needle,
				&matchPriority))
		{
			if(matchPriority == 1) //found best possible match, so done
				return haystackPair.second;

			if(!bestMatchPriority || matchPriority < bestMatchPriority)
			{
				//found new best match
				bestMatchPriority = matchPriority;
				bestMatchKey = haystackPair.first;
			}
		}

	if(bestMatchPriority) //found a match, although not exact, i.e. wildcard was used
	{
		//__COUTV__(bestMatchPriority);
		//__COUTV__(bestMatchKey);
		//__COUT__ << "value found: " << haystack.at(bestMatchKey) << __E__;
		return haystack.at(bestMatchKey);
	}

	__SS__ << "Needle '" << needle << "' not found in wildcard haystack:" << __E__;
	bool first = true;
	for(const auto& haystackPair : haystack)
		if(first)
		{
			ss << ", " << haystackPair.first;
			first = false;
		}
		else
			ss << ", " << haystackPair.first;
	__SS_THROW__;
}

static std::string		 	decodeURIComponent 						(const std::string& data);
static std::string 			convertEnvironmentVariables				(const std::string& data);
static bool 	        	isNumber           						(const std::string& stringToCheck);
static std::string			binaryToHexString						(const char *binaryBuffer, unsigned int numberOfBytes, const std::string& resultPreamble = "", const std::string& resultDelimiter = "");

template<class T>
static bool 	        	getNumber								(const std::string& s, T& retValue)
{
	//extract set of potential numbers and operators
	std::vector<std::string> numbers;
	std::vector<char> ops;

	StringMacros::getVectorFromString(s,numbers,
			/*delimiter*/ 	std::set<char>({'+','-','*','/'}),
			/*whitespace*/ 	std::set<char>({' ','\t','\n','\r'}),
			&ops);

	retValue = 0; //initialize

	T tmpValue;
	unsigned int i = 0;
	bool verified;
	for(const auto& number:numbers)
	{
		if(number.size() == 0) continue; //skip empty numbers

		//verify that this number looks like a number
		//	for integer types, allow hex and binary
		//	for all types allow base10

		verified = false;

		//check integer types
		if(typeid(unsigned int) == typeid(retValue) ||
				typeid(int) == typeid(retValue) ||
				typeid(unsigned long long) == typeid(retValue) ||
				typeid(long long) == typeid(retValue) ||
				typeid(unsigned long) == typeid(retValue) ||
				typeid(long) == typeid(retValue) ||
				typeid(unsigned short) == typeid(retValue) ||
				typeid(short) == typeid(retValue) ||
				typeid(uint8_t) == typeid(retValue))
		{
			if(number.find("0x") == 0) //indicates hex
			{
				//__COUT__ << "0x found" << std::endl;
				for(unsigned int i=2;i<number.size();++i)
				{
					if(!((number[i] >= '0' && number[i] <= '9') ||
							(number[i] >= 'A' && number[i] <= 'F') ||
							(number[i] >= 'a' && number[i] <= 'f')
					))
					{
						//__COUT__ << "prob " << number[i] << std::endl;
						return false;
					}
				}
				verified = true;
			}
			else if(number[0] == 'b') //indicates binary
			{
				//__COUT__ << "b found" << std::endl;

				for(unsigned int i=1;i<number.size();++i)
				{
					if(!((number[i] >= '0' && number[i] <= '1')
					))
					{
						//__COUT__ << "prob " << number[i] << std::endl;
						return false;
					}
				}
				verified = true;
			}
		}

		//if not verified above, for all types, check base10
		if(!verified)
			for(unsigned int i=0;i<number.size();++i)
				if(!((number[i] >= '0' && number[i] <= '9') ||
						number[i] == '.' ||
						number[i] == '+' ||
						number[i] == '-'))
					return false;

		//at this point, this number is confirmed to be a number of some sort
		// so convert to temporary number
		if(typeid(double) == typeid(retValue))
			tmpValue = strtod(number.c_str(),0);
		else if(typeid(float) == typeid(retValue))
			tmpValue = strtof(number.c_str(),0);
		else if(typeid(unsigned int) == typeid(retValue) ||
				typeid(int) == typeid(retValue) ||
				typeid(unsigned long long) == typeid(retValue) ||
				typeid(long long) == typeid(retValue) ||
				typeid(unsigned long) == typeid(retValue) ||
				typeid(long) == typeid(retValue) ||
				typeid(unsigned short) == typeid(retValue) ||
				typeid(short) == typeid(retValue) ||
				typeid(uint8_t) == typeid(retValue))
		{
			if(number.size() > 2 && number[1] == 'x') //assume hex value
				tmpValue = (T)strtol(number.c_str(),0,16);
			else if(number.size() > 1 && number[0] == 'b') //assume binary value
				tmpValue = (T)strtol(number.substr(1).c_str(),0,2); //skip first 'b' character
			else
				tmpValue = (T)strtol(number.c_str(),0,10);
		}
		else
		{
			__SS__ << "Invalid type '" <<
				StringMacros::demangleTypeName(typeid(retValue).name()) <<
				"' requested for a numeric string. Data was '" <<
				number << "'" << __E__;
			__SS_THROW__;
		}

		//apply operation
		if(i == 0) 	//first value, no operation, just take value
			retValue = tmpValue;
		else 		//there is some sort of op
		{
			if(0 && i==1) //display what we are dealing with
			{
				__COUTV__(StringMacros::vectorToString(numbers));
				__COUTV__(StringMacros::vectorToString(ops));
			}
			//__COUT__ << "Intermediate value = " << retValue << __E__;

			switch(ops[i-1])
			{
			case '+':
				retValue += tmpValue;
				break;
			case '-':
				retValue -= tmpValue;
				break;
			case '*':
				retValue *= tmpValue;
				break;
			case '/':
				retValue /= tmpValue;
				break;
			default:
				__SS__ << "Unrecognized operation '" << ops[i-1] << "' found!" << __E__ <<
					"Numbers: " <<
					StringMacros::vectorToString(numbers) << __E__ <<
					"Operations: " <<
					StringMacros::vectorToString(ops) << __E__;
				__SS_THROW__;
			}
			//__COUT__ << "Op " << ops[i-1] << " intermediate value = " << retValue << __E__;
		}

		++i; //increment index for next number/op
	} //end number loop

	return true; //number was valid and is passed by reference in retValue
} //end static getNumber()


//special validation ignoring any table info - just assuming type string
template<class T>
static T 					validateValueForDefaultStringDataType	(const std::string& value, bool doConvertEnvironmentVariables=true)
{
	T retValue;
	try
	{
		//__COUTV__(value);
		std::string data = doConvertEnvironmentVariables?convertEnvironmentVariables(value):
				value;

		//Note: allow string column types to convert to number (since user's would likely expect this to work)
		if(StringMacros::getNumber(data,retValue))
			return retValue;
		else
		{
			__SS__ << "Invalid type '" <<
					StringMacros::demangleTypeName(typeid(retValue).name()) <<
					"' requested for a non-numeric string (must request std::string). Data was '" <<
					data << "'" << __E__;
			__SS_THROW__;
		}
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Failed to validate the string value for the data type  '" <<
				StringMacros::demangleTypeName(typeid(retValue).name()) <<
				"' requested. " << __E__ << e.what() << __E__;
		__SS_THROW__;
	}
}
static std::string 			validateValueForDefaultStringDataType	(const std::string& value, bool doConvertEnvironmentVariables=true);


static void					getSetFromString 						(const std::string& inputString, std::set<std::string>& setToReturn, const std::set<char>& delimiter = {',','|','&'}, const std::set<char>& whitespace = {' ','\t','\n','\r'});
static void					getVectorFromString						(const std::string& inputString, std::vector<std::string>& listToReturn, const std::set<char>& delimiter = {',','|','&'}, const std::set<char>& whitespace = {' ','\t','\n','\r'}, std::vector<char>* listOfDelimiters = 0);
template<class T>
static void					getMapFromString 						(const std::string& inputString, std::map<std::string,T>& mapToReturn, const std::set<char>& pairPairDelimiter = {',','|','&'}, const std::set<char>& nameValueDelimiter = {'=',':'}, const std::set<char>& whitespace = {' ','\t','\n','\r'})
try
{
	unsigned int i=0;
	unsigned int j=0;
	std::string name;
	bool needValue = false;

	//go through the full string extracting map pairs
	//add each found pair to map
	for(;j<inputString.size();++j)
		if(!needValue) //finding name
		{
			if((whitespace.find(inputString[j]) != whitespace.end() || //ignore leading white space or delimiter
					pairPairDelimiter.find(inputString[j]) != pairPairDelimiter.end())
					&& i == j)
				++i;
			else if((whitespace.find(inputString[j]) != whitespace.end() || //trailing white space or delimiter indicates end
					nameValueDelimiter.find(inputString[j]) != nameValueDelimiter.end())
					&& i != j) // assume end of map name
			{
				//__COUT__ << "Map name found: " <<
				//		inputString.substr(i,j-i) << std::endl;

				name = inputString.substr(i,j-i); //save name, for concluding pair

				needValue = true; //need value now

				//setup i and j for next find
				i = j+1;
			}
		}
		else //finding value
		{
			if((whitespace.find(inputString[j]) != whitespace.end() || //ignore leading white space or delimiter
					nameValueDelimiter.find(inputString[j]) != nameValueDelimiter.end())
					&& i == j)
				++i;
			else if((whitespace.find(inputString[j]) != whitespace.end() || //trailing white space or delimiter indicates end
					pairPairDelimiter.find(inputString[j]) != pairPairDelimiter.end())
					&& i != j) // assume end of value name
			{
				//__COUT__ << "Map value found: " <<
				//		inputString.substr(i,j-i) << std::endl;

				auto /*pair<it,success>*/ emplaceReturn = mapToReturn.emplace(std::pair<std::string,T>(
						name,
						validateValueForDefaultStringDataType<T>(
								inputString.substr(i,j-i)) //value
				));

				if(!emplaceReturn.second)
				{
					__COUT__ << "Ignoring repetitive value ('" <<  inputString.substr(i,j-i) <<
							"') and keeping current value ('" << emplaceReturn.first->second << "'). " << __E__;
				}

				needValue = false; //need name now

				//setup i and j for next find
				i = j+1;
			}
		}

	if(i != j) //last value (for case when no concluding ' ' or delimiter)
	{
		auto /*pair<it,success>*/ emplaceReturn = mapToReturn.emplace(std::pair<std::string,T>(
				name,
				validateValueForDefaultStringDataType<T>(
						inputString.substr(i,j-i)) //value
		));

		if(!emplaceReturn.second)
		{
			__COUT__ << "Ignoring repetitive value ('" <<  inputString.substr(i,j-i) <<
					"') and keeping current value ('" << emplaceReturn.first->second << "'). " << __E__;
		}
	}
}
catch(const std::runtime_error &e)
{
	__SS__ << "Error while extracting a map from the string '" <<
			inputString << "'... is it a valid map?" << __E__ << e.what() << __E__;
	__SS_THROW__;
}

static void					getMapFromString 						(const std::string& inputString, std::map<std::string,std::string>& mapToReturn, const std::set<char>& pairPairDelimiter = {',','|','&'}, const std::set<char>& nameValueDelimiter = {'=',':'}, const std::set<char>& whitespace = {' ','\t','\n','\r'});
template<class T>
static std::string			mapToString								(const std::map<std::string,T>& mapToReturn, const std::string& primaryDelimeter = ", ", const std::string& secondaryDelimeter = ": ")
{
	std::stringstream ss;
	bool first = true;
	for(auto& mapPair:mapToReturn)
	{
		if(first) first = false;
		else ss << primaryDelimeter;
		ss << mapPair.first << secondaryDelimeter <<
			mapPair.second;
	}
	return ss.str();
}
template<class T>
static std::string			mapToString								(const std::map<std::pair<std::string,std::string>,T>& mapToReturn, const std::string& primaryDelimeter = ", ", const std::string& secondaryDelimeter = ": ")
{
	//this is a pretty specific map format (comes from the merge function in ConfigurationBase)

	std::stringstream ss;
	bool first = true;
	for(auto& mapPair:mapToReturn)
	{
		if(first) first = false;
		else ss << primaryDelimeter;
		ss << mapPair.first.first << "/" << mapPair.first.second << secondaryDelimeter <<
			mapPair.second;
	}
	return ss.str();
}
template<class T>
static std::string			mapToString								(const std::map<std::pair<std::string,std::pair<std::string,std::string>>,T>& mapToReturn, const std::string& primaryDelimeter = ", ", const std::string& secondaryDelimeter = ": ")
{
	//this is a pretty specific map format (comes from the merge function in ConfigurationBase)
	std::stringstream ss;
	bool first = true;
	for(auto& mapPair:mapToReturn)
	{
		if(first) first = false;
		else ss << primaryDelimeter;
		ss << mapPair.first.first << "/" << mapPair.first.second.first << "," << mapPair.first.second.second <<
				secondaryDelimeter <<
			mapPair.second;
	}
	return ss.str();
}
template<class T>
static std::string			mapToString								(const std::map<std::string,std::pair<std::string,T> >& mapToReturn, const std::string& primaryDelimeter = ", ", const std::string& secondaryDelimeter = ": ")
{
	//this is a pretty specific map format (comes from the get aliases functions in ConfigurationManager)
	std::stringstream ss;
	bool first = true;
	for(auto& mapPair:mapToReturn)
	{
		if(first) first = false;
		else ss << primaryDelimeter;
		ss << mapPair.first << "/" << mapPair.second.first << secondaryDelimeter << mapPair.second.second;
	}
	return ss.str();
}
template<class T>
static std::string			mapToString								(const std::map<std::string,std::map<std::string,T> >& mapToReturn, const std::string& primaryDelimeter = ", ", const std::string& secondaryDelimeter = ": ")
{
	//this is a pretty specific map format (comes from the get aliases functions in ConfigurationManager)
	std::stringstream ss;
	bool first = true;
	for(auto& mapPair:mapToReturn)
	{
		if(first) first = false;
		else ss << primaryDelimeter;
		ss << mapPair.first;
		for(auto& secondMapPair:mapPair.second)
			ss << "/" << secondMapPair.first << secondaryDelimeter << secondMapPair.second;
	}
	return ss.str();
}
static std::string			mapToString								(const std::map<std::string,uint8_t>& mapToReturn, const std::string& primaryDelimeter = ", ", const std::string& secondaryDelimeter = ": ");
template<class T>
static std::string			setToString								(const std::set<T>& setToReturn, const std::string& delimeter = ", ")
{
	std::stringstream ss;
	bool first = true;
	for(auto& setValue:setToReturn)
	{
		if(first) first = false;
		else ss << delimeter;
		ss << setValue;
	}
	return ss.str();
}
static std::string			setToString								(const std::set<uint8_t>& setToReturn, const std::string& delimeter = ", ");
template<class T>
static std::string			vectorToString							(const std::vector<T>& setToReturn, const std::string& delimeter = ", ")
{
	std::stringstream ss;
	bool first = true;
	for(auto& setValue:setToReturn)
	{
		if(first) first = false;
		else ss << delimeter;
		ss << setValue;
	}
	return ss.str();
}
static std::string			vectorToString							(const std::vector<uint8_t>& setToReturn, const std::string& delimeter = ", ");

static std::string 			demangleTypeName						(const char* name);

}; //end class
} //end namespace
#endif
