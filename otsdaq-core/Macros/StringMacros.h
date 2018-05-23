#ifndef _ots_String_Macros_h_
#define _ots_String_Macros_h_

#include "otsdaq-core/Macros/CoutMacros.h"

#include <set>
#include <map>
#include <typeinfo>		// operator typeid

namespace ots
{

struct StringMacros
{

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
		throw std::runtime_error(ss.str());
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
	throw std::runtime_error(ss.str());
}

static std::string		 	decodeURIComponent 						(const std::string& data);
static std::string 			convertEnvironmentVariables				(const std::string& data);
static bool 	        	isNumber           						(const std::string& s);
static bool 	        	extractAndCalculateNumber				(const std::string& s);


//special validation ignoring any table info - just assuming type string
template<class T>
static T 					validateValueForDefaultStringDataType	(const std::string& value, bool doConvertEnvironmentVariables=true)
try
{
	T retValue;
	std::string data = doConvertEnvironmentVariables?convertEnvironmentVariables(value):
			value;

//	if(extractAndCalculateNumber(data))
//	{
//
//	}

	if(isNumber(data))
	{
		//allow string conversion to integer for ease of use

		if(typeid(double) == typeid(retValue))
			retValue = strtod(data.c_str(),0);
		else if(typeid(float) == typeid(retValue))
			retValue = strtof(data.c_str(),0);
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
			if(data.size() > 2 && data[1] == 'x') //assume hex value
				retValue = (T)strtol(data.c_str(),0,16);
			else if(data.size() > 1 && data[0] == 'b') //assume binary value
				retValue = (T)strtol(data.substr(1).c_str(),0,2); //skip first 'b' character
			else
				retValue = (T)strtol(data.c_str(),0,10);
		}
		else
		{
			__SS__ << "Invalid type '" <<
				StringMacros::demangleTypeName(typeid(retValue).name()) <<
				"' requested for a numeric string. Data was '" <<
				data << "'" << __E__;
			__SS_THROW__;
		}

		return retValue;
	}
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
	__SS__ << "Failed to validate value for default string data type. " << __E__ << e.what() << __E__;
	__SS_THROW__;
}
static std::string 			validateValueForDefaultStringDataType	(const std::string& value, bool doConvertEnvironmentVariables=true);


static void					getSetFromString 						(const std::string& inputString, std::set<std::string>& setToReturn, const std::set<char>& delimiter = {',','|','&'}, const std::set<char>& whitespace = {' ','\t','\n','\r'});
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
static std::string			mapToString								(const std::map<std::string,T>& mapToReturn, const std::string& primaryDelimeter = ",", const std::string& secondaryDelimeter = ": ")
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
static std::string			mapToString								(const std::map<std::string,uint8_t>& mapToReturn, const std::string& primaryDelimeter = ",", const std::string& secondaryDelimeter = ": ");
template<class T>
static std::string			setToString								(const std::set<T>& setToReturn, const std::string& delimeter = ",")
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
static std::string			setToString								(const std::set<uint8_t>& setToReturn, const std::string& delimeter = ",");

static std::string 			demangleTypeName						(const char* name);

}; //end class
} //end namespace
#endif
