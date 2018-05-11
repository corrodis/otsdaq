#include "otsdaq-core/Macros/StringMacros.h"


using namespace ots;

//==============================================================================
//wildCardMatch
//	find needle in haystack
//		allow needle to have leading and/or trailing wildcard '*'
bool StringMacros::wildCardMatch(const std::string& needle, const std::string& haystack)
try
{
//	__COUT__ << "\t\t wildCardMatch: " << needle <<
//			" =in= " << haystack << " ??? " <<
//			std::endl;

	if(needle.size() == 0)
		return true; //if empty needle, always "found"

	if(needle[0] == '*' && //leading wildcard
					needle[needle.size()-1] == '*' ) //and trailing wildcard
		return std::string::npos != haystack.find(needle.substr(1,needle.size()-2));

	if(needle[0] == '*') //leading wildcard
		return needle.substr(1) ==
				haystack.substr(haystack.size() - (needle.size()-1));

	if(needle[needle.size()-1] == '*') //trailing wildcard
		return needle.substr(0,needle.size()-1) ==
				haystack.substr(0,needle.size()-1);

	//else //no wildcards
	return needle == haystack;
}
catch(...)
{
	return false; //if out of range
}


//========================================================================================================================
//inWildCardSet ~
//	returns true if needle is in haystack (considering wildcards)
bool StringMacros::inWildCardSet(const std::string needle, const std::set<std::string>& haystack)
{
	for(const auto& haystackString : haystack)
		//use wildcard match, flip needle parameter.. because we want haystack to have the wildcards
		if(StringMacros::wildCardMatch(haystackString,needle)) return true;
	return false;
}

//========================================================================================================================
//inWildCardSet ~
//	returns true if needle is in haystack (considering wildcards)
const std::string& StringMacros::getWildCardMatchFromMap(const std::string needle,
		const std::map<std::string, std::string>& haystack)
{
	for(const auto& haystackPair : haystack)
		//use wildcard match, flip needle parameter.. because we want haystack to have the wildcards
		if(StringMacros::wildCardMatch(haystackPair.first,needle)) return haystackPair.second;

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

//==============================================================================
//decodeURIComponent
//	converts all %## to the ascii character
std::string StringMacros::decodeURIComponent(const std::string &data)
{
	std::string decodeURIString(data.size(),0); //init to same size
	unsigned int j=0;
	for(unsigned int i=0;i<data.size();++i,++j)
	{
		if(data[i] == '%')
		{
			//high order hex nibble digit
			if(data[i+1] > '9') //then ABCDEF
				decodeURIString[j] += (data[i+1]-55)*16;
			else
				decodeURIString[j] += (data[i+1]-48)*16;

			//low order hex nibble digit
			if(data[i+2] > '9')	//then ABCDEF
				decodeURIString[j] += (data[i+2]-55);
			else
				decodeURIString[j] += (data[i+2]-48);

			i+=2; //skip to next char
		}
		else
			decodeURIString[j] = data[i];
	}
	decodeURIString.resize(j);
	return decodeURIString;
}

//==============================================================================
//convertEnvironmentVariables ~
//	static function
std::string StringMacros::convertEnvironmentVariables(const std::string& data)
{
	std::string converted = data;
	if(data.find("${") != std::string::npos)
	{
		unsigned int begin = data.find("${");
		unsigned int end   = data.find("}");
		std::string envVariable = data.substr(begin+2, end-begin-2);
		//std::cout << "Converted Value: " << envVariable << std::endl;
		if(getenv(envVariable.c_str()) != nullptr)
		{
			return convertEnvironmentVariables(converted.replace(begin,end-begin+1,getenv(envVariable.c_str())));
		}
		else
		{
			//static function so can't use normal __SS__ (with table name in it)
			std::stringstream ss;
			ss << __COUT_HDR__ <<
					("The environmental variable: " + envVariable +
					" is not set! Please make sure you set it before continuing!") << std::endl;
			__COUT_ERR__ << ss.str();
			throw std::runtime_error(ss.str());
		}
	}
	return converted;
}

//==============================================================================
//isNumber ~~
//	returns true if hex ("0x.."), binary("b..."), or base10 number
bool StringMacros::isNumber(const std::string& s)
{
	//__COUT__ << "string " << s << std::endl;
	if(s.find("0x") == 0) //indicates hex
	{
		//__COUT__ << "0x found" << std::endl;
		for(unsigned int i=2;i<s.size();++i)
		{
			if(!((s[i] >= '0' && s[i] <= '9') ||
					(s[i] >= 'A' && s[i] <= 'F') ||
					(s[i] >= 'a' && s[i] <= 'f')
			))
			{
				//__COUT__ << "prob " << s[i] << std::endl;
				return false;
			}
		}
		//return std::regex_match(s.substr(2), std::regex("^[0-90-9a-fA-F]+"));
	}
	else if(s[0] == 'b') //indicates binary
	{
		//__COUT__ << "b found" << std::endl;

		for(unsigned int i=1;i<s.size();++i)
		{
			if(!((s[i] >= '0' && s[i] <= '1')
			))
			{
				//__COUT__ << "prob " << s[i] << std::endl;
				return false;
			}
		}
	}
	else
	{
		//__COUT__ << "base 10 " << std::endl;
		for(unsigned int i=0;i<s.size();++i)
			if(!((s[i] >= '0' && s[i] <= '9') ||
					s[i] == '.' ||
					s[i] == '+' ||
					s[i] == '-'))
				return false;
		//Note: std::regex crashes in unresolvable ways (says Ryan.. also, stop using libraries)
		//return std::regex_match(s, std::regex("^(\\-|\\+)?[0-9]*(\\.[0-9]+)?"));
	}

	return true;
}

//==============================================================================
// validateValueForDefaultStringDataType
//
std::string StringMacros::validateValueForDefaultStringDataType(const std::string& value,
		bool doConvertEnvironmentVariables)
{
	return doConvertEnvironmentVariables?
			StringMacros::convertEnvironmentVariables(value):
			value;
}

//==============================================================================
//getSetFromString
//	extracts the set of elements from string that uses a delimiter
//		ignoring whitespace
void StringMacros::getSetFromString(const std::string& inputString,
		std::set<std::string>& setToReturn, const std::set<char>& delimiter,
		const std::set<char>& whitespace)
{
	unsigned int i=0;
	unsigned int j=0;

	//go through the full string extracting elements
	//add each found element to set
	for(;j<inputString.size();++j)
		if((whitespace.find(inputString[j]) != whitespace.end() || //ignore leading white space or delimiter
				delimiter.find(inputString[j]) != delimiter.end())
				&& i == j)
			++i;
		else if((whitespace.find(inputString[j]) != whitespace.end() || //trailing white space or delimiter indicates end
				delimiter.find(inputString[j]) != delimiter.end())
				&& i != j) // assume end of element
		{
			//__COUT__ << "Set element found: " <<
			//		inputString.substr(i,j-i) << std::endl;

			setToReturn.emplace(inputString.substr(i,j-i));

			//setup i and j for next find
			i = j+1;
		}

	if(i != j) //last element check (for case when no concluding ' ' or delimiter)
		setToReturn.emplace(inputString.substr(i,j-i));
}

//==============================================================================
//getMapFromString
//	extracts the map of name-value pairs from string that uses two s
//		ignoring whitespace
void StringMacros::getMapFromString(const std::string& inputString,
		std::map<std::string,std::string>& mapToReturn,
		const std::set<char>& pairPairDelimiter, const std::set<char>& nameValueDelimiter,
		const std::set<char>& whitespace)
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

				auto /*pair<it,success>*/ emplaceReturn = mapToReturn.emplace(std::pair<std::string,std::string>(
						name,
						validateValueForDefaultStringDataType(
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
		auto /*pair<it,success>*/ emplaceReturn = mapToReturn.emplace(std::pair<std::string,std::string>(
				name,
				validateValueForDefaultStringDataType(
						inputString.substr(i,j-i)) //value
		));

		if(!emplaceReturn.second)
		{
			__COUT__ << "Ignoring repetitive value ('" <<  inputString.substr(i,j-i) <<
					"') and keeping current value ('" << emplaceReturn.first->second << "'). " << __E__;
		}
	}
}


//==============================================================================
//mapToString
std::string StringMacros::mapToString(const std::map<std::string,uint8_t>& mapToReturn,
		const std::string& primaryDelimeter, const std::string& secondaryDelimeter)
{
	std::stringstream ss;
	for(auto& mapPair:mapToReturn)
		ss << mapPair.first << secondaryDelimeter << (unsigned int)mapPair.second << primaryDelimeter;
	return ss.str();
}

//==============================================================================
//setToString
std::string StringMacros::setToString(const std::set<uint8_t>& setToReturn, const std::string& delimeter)
{
	std::stringstream ss;
	bool first = true;
	for(auto& setValue:setToReturn)
	{
		if(first) first = false;
		else ss << delimeter;
		ss << (unsigned int)setValue;
	}
	return ss.str();
}


#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>


//==============================================================================
//demangleTypeName
std::string StringMacros::demangleTypeName(const char* name)
{
	int status = -4; // some arbitrary value to eliminate the compiler warning

	// enable c++11 by passing the flag -std=c++11 to g++
	std::unique_ptr<char, void(*)(void*)> res {
		abi::__cxa_demangle(name, NULL, NULL, &status),
				std::free
	};

	return (status==0) ? res.get() : name ;
}

#else //does nothing if not g++
//==============================================================================
//getMapFromString
//
std::string StringMacros::demangleTypeName(const char* name)
{
	return name;
}


#endif







