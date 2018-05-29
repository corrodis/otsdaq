#include "otsdaq-core/Macros/StringMacros.h"


using namespace ots;

//==============================================================================
//wildCardMatch
//	find needle in haystack
//		allow needle to have leading and/or trailing wildcard '*'
//		consider priority in matching, no mater the order in the haystack:
//			- 0: no match!
//			- 1: highest priority is exact match
//			- 2: next highest is partial TRAILING-wildcard match
//			- 3: next highest is partial LEADING-wildcard match
//			- 4: lowest priority is partial full-wildcard match
//		return priority found by reference
bool StringMacros::wildCardMatch(const std::string& needle, const std::string& haystack,
		unsigned int* priorityIndex)
try
{
//	__COUT__ << "\t\t wildCardMatch: " << needle <<
//			" =in= " << haystack << " ??? " <<
//			std::endl;

	//empty needle
	if(needle.size() == 0)
	{
		if(priorityIndex) *priorityIndex = 1; //consider an exact match, to stop higher level loops
		return true; //if empty needle, always "found"
	}

	//only wildcard
	if(needle == "*")
	{
		if(priorityIndex) *priorityIndex = 5; //only wildcard, is lowest priority
		return true; //if empty needle, always "found"
	}

	//no wildcards
	if(needle == haystack)
	{
		if(priorityIndex) *priorityIndex = 1; //an exact match
		return true;
	}

	//trailing wildcard
	if(needle[needle.size()-1] == '*' &&
		needle.substr(0,needle.size()-1) ==
				haystack.substr(0,needle.size()-1))
	{
		if(priorityIndex) *priorityIndex = 2; //trailing wildcard match
		return true;
	}

	//leading wildcard
	if(needle[0] == '*' && needle.substr(1) ==
			haystack.substr(haystack.size() - (needle.size()-1)))
	{
		if(priorityIndex) *priorityIndex = 3; //leading wildcard match
		return true;
	}

	//leading wildcard and trailing wildcard
	if(needle[0] == '*' &&
					needle[needle.size()-1] == '*' &&
		std::string::npos != haystack.find(needle.substr(1,needle.size()-2)))
	{
		if(priorityIndex) *priorityIndex = 4; //leading and trailing wildcard match
		return true;
	}

	//else no match
	if(priorityIndex) *priorityIndex = 0; //no match
	return false;
}
catch(...)
{
	if(priorityIndex) *priorityIndex = 0; //no match
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
//	static recursive function
//
//	allows environment variables entered as $NAME or ${NAME}
std::string StringMacros::convertEnvironmentVariables(const std::string& data)
{
	size_t begin = data.find("$");
	if(begin != std::string::npos)
	{
		size_t end;
		std::string envVariable;
		std::string converted = data; //make copy to modify

		if(data[begin+1] == '{')//check if using ${NAME} syntax
		{
			end = data.find("}",begin+2);
			envVariable = data.substr(begin+2, end-begin-2);
			++end; //replace the closing } too!
		}
		else					//else using $NAME syntax
		{
			//end is first non environment variable character
			for(end = begin+1; end < data.size(); ++end)
				if(!((data[end] >= '0' && data[end] <= '9') ||
								(data[end] >= 'A' && data[end] <= 'Z') ||
								(data[end] >= 'a' && data[end] <= 'z') ||
								data[end] == '-' || data[end] == '_' ||
								data[end] == '.' || data[end] == ':'))
					break; //found end
			envVariable = data.substr(begin+1, end-begin-1);
		}
		//__COUTV__(data);
		//__COUTV__(envVariable);
		char *envResult = getenv(envVariable.c_str());

		if(envResult)
		{
			//proceed recursively
			return convertEnvironmentVariables(converted.replace(begin,end-begin,envResult));
		}
		else
		{
			__SS__ <<
					("The environmental variable '" + envVariable +
					"' is not set! Please make sure you set it before continuing!") << std::endl;
			__SS_THROW__;
		}
	}
	//else no environment variables found in string
	//__COUT__ << "Result: " << data << __E__;
	return data;
}

//==============================================================================
//isNumber ~~
//	returns true if one or many numbers separated by operations (+,-,/,*) is
//		present in the string.
//	Numbers can be hex ("0x.."), binary("b..."), or base10.
bool StringMacros::isNumber(const std::string& s)
{
	//extract set of potential numbers and operators
	std::vector<std::string> numbers;
	std::vector<char> ops;

	StringMacros::getVectorFromString(s,numbers,
			/*delimiter*/ 	std::set<char>({'+','-','*','/'}),
			/*whitespace*/ 	std::set<char>({' ','\t','\n','\r'}),
			&ops);

	//__COUTV__(StringMacros::vectorToString(numbers));
	//__COUTV__(StringMacros::vectorToString(ops));

	for(const auto& number:numbers)
	{
		if(number.size() == 0) continue; //skip empty numbers

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
			//return std::regex_match(number.substr(2), std::regex("^[0-90-9a-fA-F]+"));
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
		}
		else
		{
			//__COUT__ << "base 10 " << std::endl;
			for(unsigned int i=0;i<number.size();++i)
				if(!((number[i] >= '0' && number[i] <= '9') ||
						number[i] == '.' ||
						number[i] == '+' ||
						number[i] == '-'))
					return false;
			//Note: std::regex crashes in unresolvable ways (says Ryan.. also, stop using libraries)
			//return std::regex_match(s, std::regex("^(\\-|\\+)?[0-9]*(\\.[0-9]+)?"));
		}
	}

	//__COUT__ << "yes " << std::endl;

	//all numbers are numbers
	return true;
} //end isNumber()

//==============================================================================
// validateValueForDefaultStringDataType
//
std::string StringMacros::validateValueForDefaultStringDataType(const std::string& value,
		bool doConvertEnvironmentVariables)
try
{
	return doConvertEnvironmentVariables?
			StringMacros::convertEnvironmentVariables(value):
			value;
}
catch(const std::runtime_error& e)
{
	__SS__ << "Failed to validate value for default string data type. " << __E__ << e.what() << __E__;
	__SS_THROW__;
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
} //end getSetFromString()

//==============================================================================
//getVectorFromString
//	extracts the list of elements from string that uses a delimiter
//		ignoring whitespace
//	optionally returns the list of delimiters encountered, which may be useful
//		for extracting which operator was used
//
//	Note: lists are returned as vectors
//	Note: the size() of delimiters will be one less than the size() of the returned values
void StringMacros::getVectorFromString(const std::string& inputString,
		std::vector<std::string>& listToReturn, const std::set<char>& delimiter,
		const std::set<char>& whitespace, std::vector<char>* listOfDelimiters)
{
	unsigned int i=0;
	unsigned int j=0;
	std::set<char>::iterator delimeterSearchIt;
	char lastDelimiter;
	bool isDelimiter;

	//__COUT__ << inputString << __E__;

	//go through the full string extracting elements
	//add each found element to set
	for(;j<inputString.size();++j)
	{
		//__COUT__ << (char)inputString[j] << __E__;

		delimeterSearchIt = delimiter.find(inputString[j]);
		isDelimiter = delimeterSearchIt != delimiter.end();

		//__COUT__ << (char)inputString[j] << " " << (char)lastDelimiter << __E__;

		if((whitespace.find(inputString[j]) != whitespace.end() || //ignore leading white space or delimiter
				isDelimiter)
				&& i == j)
			++i;
		else if((whitespace.find(inputString[j]) != whitespace.end() || //trailing white space or delimiter indicates end
				isDelimiter)
				&& i != j) // assume end of element
		{
			//__COUT__ << "Set element found: " <<
			//		inputString.substr(i,j-i) << std::endl;

			if(listOfDelimiters && listToReturn.size())
				listOfDelimiters->push_back(lastDelimiter);
			listToReturn.push_back(inputString.substr(i,j-i));


			//setup i and j for next find
			i = j+1;
		}

		if(isDelimiter)
			lastDelimiter = *delimeterSearchIt;
	}

	if(i != j) //last element check (for case when no concluding ' ' or delimiter)
	{
		if(listOfDelimiters && listToReturn.size())
			listOfDelimiters->push_back(lastDelimiter);
		listToReturn.push_back(inputString.substr(i,j-i));
	}

	//assert that there is one less delimiter than values
	if(listOfDelimiters && listToReturn.size() - 1 != listOfDelimiters->size())
	{
		__SS__ << "There is a mismatch in delimiters to entries (should be one less delimiter): " <<
				listOfDelimiters->size() <<
				" vs " << listToReturn.size()  << __E__ <<
				"Entries: " <<
 				StringMacros::vectorToString(listToReturn) << __E__ <<
				"Delimiters: " <<
 				StringMacros::vectorToString(*listOfDelimiters) << __E__;
		__SS_THROW__;
	}

} //end getVectorFromString()

//==============================================================================
//getMapFromString
//	extracts the map of name-value pairs from string that uses two s
//		ignoring whitespace
void StringMacros::getMapFromString(const std::string& inputString,
		std::map<std::string,std::string>& mapToReturn,
		const std::set<char>& pairPairDelimiter, const std::set<char>& nameValueDelimiter,
		const std::set<char>& whitespace)
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
catch(const std::runtime_error &e)
{
	__SS__ << "Error while extracting a map from the string '" <<
			inputString << "'... is it a valid map?" << __E__ << e.what() << __E__;
	__SS_THROW__;
}


//==============================================================================
//mapToString
std::string StringMacros::mapToString(const std::map<std::string,uint8_t>& mapToReturn,
		const std::string& primaryDelimeter, const std::string& secondaryDelimeter)
{
	std::stringstream ss;
	bool first = true;
	for(auto& mapPair:mapToReturn)
	{
		if(first) first = false;
		else ss << primaryDelimeter;
		ss << mapPair.first << secondaryDelimeter << (unsigned int)mapPair.second;
	}
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

//==============================================================================
//vectorToString
std::string StringMacros::vectorToString(const std::vector<uint8_t>& setToReturn, const std::string& delimeter)
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







