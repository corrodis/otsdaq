#ifndef _ots_StringMacros_h_
#define _ots_StringMacros_h_

#include "otsdaq-core/Macros/CoutMacros.h"

#include <map>
#include <set>
#include <typeinfo>  // operator typeid
#include <vector>
#include <memory> //shared_ptr

namespace ots
{
struct StringMacros
{
  private:  // private constructor because all static members, should never instantiate
	        // this class
	StringMacros(void);
	~StringMacros(void);

  public:
	// Here is the list of static helper functions:
	//
	//		wildCardMatch
	//		inWildCardSet
	//		getWildCardMatchFromMap
	//
	//		decodeURIComponent
	//		convertEnvironmentVariables
	//		isNumber
	//		getNumber
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

	static bool wildCardMatch(const std::string& needle,
	                          const std::string& haystack,
	                          unsigned int*      priorityIndex = 0);
	static bool inWildCardSet(const std::string&           needle,
	                          const std::set<std::string>& haystack);

	//========================================================================================================================
	// getWildCardMatchFromMap ~
	//	returns value if needle is in haystack otherwise throws exception
	//	(considering wildcards AND match priority as defined by
	// StringMacros::wildCardMatch)
	template<class T>
	static T& getWildCardMatchFromMap(
	    const std::string&        needle,
	    std::map<std::string, T>& haystack,
	    std::string*              foundKey = 0);  // defined in included .icc source

	static std::string decodeURIComponent(const std::string& data);
	static std::string convertEnvironmentVariables(const std::string& data);
	static bool        isNumber(const std::string& stringToCheck);

	template<class T>
	static bool        getNumber(const std::string& s,
	                             T& retValue);  // defined in included .icc source
	static std::string getTimestampString(const std::string& linuxTimeInSeconds);
	static std::string getTimestampString(const time_t& linuxTimeInSeconds = time(0));

	// special validation ignoring any table info - just assuming type string
	template<class T>
	static T validateValueForDefaultStringDataType(
	    const std::string& value,
	    bool doConvertEnvironmentVariables = true);  // defined in included .icc source
	static std::string validateValueForDefaultStringDataType(
	    const std::string& value, bool doConvertEnvironmentVariables = true);

	static void getSetFromString(const std::string&     inputString,
	                             std::set<std::string>& setToReturn,
	                             const std::set<char>&  delimiter  = {',', '|', '&'},
	                             const std::set<char>&  whitespace = {
                                     ' ', '\t', '\n', '\r'});

	template<class T /*value type*/,
	         class S = std::string /*name string or const string*/>
	static void getMapFromString(
	    const std::string&    inputString,
	    std::map<S, T>&       mapToReturn,
	    const std::set<char>& pairPairDelimiter  = {',', '|', '&'},
	    const std::set<char>& nameValueDelimiter = {'=', ':'},
	    const std::set<char>& whitespace         = {' ', '\t', '\n', '\r'});
	static void getMapFromString(
	    const std::string&                  inputString,
	    std::map<std::string, std::string>& mapToReturn,
	    const std::set<char>&               pairPairDelimiter  = {',', '|', '&'},
	    const std::set<char>&               nameValueDelimiter = {'=', ':'},
	    const std::set<char>&               whitespace         = {' ', '\t', '\n', '\r'});
	static void getVectorFromString(
	    const std::string&        inputString,
	    std::vector<std::string>& listToReturn,
	    const std::set<char>&     delimiter        = {',', '|', '&'},
	    const std::set<char>&     whitespace       = {' ', '\t', '\n', '\r'},
	    std::vector<char>*        listOfDelimiters = 0);

	//*ToString declarations (template definitions are in included .icc source)
	template<class T>
	static std::string mapToString(const std::map<std::string, T>& mapToReturn,
	                               const std::string& primaryDelimeter   = ", ",
	                               const std::string& secondaryDelimeter = ": ");
	template<class T>
	static std::string mapToString(
	    const std::map<std::pair<std::string, std::string>, T>& mapToReturn,
	    const std::string&                                      primaryDelimeter = ", ",
	    const std::string& secondaryDelimeter                                    = ": ");
	template<class T>
	static std::string mapToString(
	    const std::map<std::pair<std::string, std::pair<std::string, std::string>>, T>&
	                       mapToReturn,
	    const std::string& primaryDelimeter   = ", ",
	    const std::string& secondaryDelimeter = ": ");
	template<class T>
	static std::string mapToString(
	    const std::map<std::string, std::pair<std::string, T>>& mapToReturn,
	    const std::string&                                      primaryDelimeter = ", ",
	    const std::string& secondaryDelimeter                                    = ": ");
	template<class T>
	static std::string mapToString(
	    const std::map<std::string, std::map<std::string, T>>& mapToReturn,
	    const std::string&                                     primaryDelimeter   = ", ",
	    const std::string&                                     secondaryDelimeter = ": ");
	template<class T>
	static std::string mapToString(const std::map<std::string, std::set<T>>& mapToReturn,
	                               const std::string& primaryDelimeter   = ", ",
	                               const std::string& secondaryDelimeter = ": ");
	template<class T>
	static std::string mapToString(
	    const std::map<std::string, std::vector<T>>& mapToReturn,
	    const std::string&                           primaryDelimeter   = ", ",
	    const std::string&                           secondaryDelimeter = ": ");
	static std::string mapToString(const std::map<std::string, uint8_t>& mapToReturn,
	                               const std::string& primaryDelimeter   = ", ",
	                               const std::string& secondaryDelimeter = ": ");

	template<class T>
	static std::string setToString(const std::set<T>& setToReturn,
	                               const std::string& delimeter = ", ");
	static std::string setToString(const std::set<uint8_t>& setToReturn,
	                               const std::string&       delimeter = ", ");
	template<class S, class T>
	static std::string setToString(const std::set<std::pair<S, T>>& setToReturn,
	                                  const std::string& primaryDelimeter   = ", ",
	                                  const std::string& secondaryDelimeter = ":");

	template<class T>
	static std::string vectorToString(const std::vector<T>& setToReturn,
	                                  const std::string&    delimeter = ", ");
	static std::string vectorToString(const std::vector<uint8_t>& setToReturn,
	                                  const std::string&          delimeter = ", ");
	template<class S, class T>
	static std::string vectorToString(const std::vector<std::pair<S, T>>& setToReturn,
	                                  const std::string& primaryDelimeter   = "; ",
	                                  const std::string& secondaryDelimeter = ":");

	static std::string demangleTypeName(const char* name);
	static std::string stackTrace();
	static std::string exec(const char* cmd);

	static char* otsGetEnvironmentVarable(const char* name, const std::string& location, const unsigned int& line);

};  // end StringMarcos static class

#include "otsdaq-core/Macros/StringMacros.icc"  //define template functions

}  // namespace ots
#endif
