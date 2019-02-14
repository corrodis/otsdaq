#ifndef ots_CgiDataUtilities_h
#define ots_CgiDataUtilities_h

#include <xgi/Method.h>  //for cgicc::Cgicc
#include <string>

namespace ots {

class CgiDataUtilities {
       public:
	CgiDataUtilities(){};
	~CgiDataUtilities(){};

	static std::string getOrPostData(cgicc::Cgicc& cgi, const std::string& needle);
	static std::string postData(cgicc::Cgicc& cgi, const std::string& needle);
	static std::string getData(cgicc::Cgicc& cgi, const std::string& needle);

	static int getOrPostDataAsInt(cgicc::Cgicc& cgi, const std::string& needle);
	static int postDataAsInt(cgicc::Cgicc& cgi, const std::string& needle);
	static int getDataAsInt(cgicc::Cgicc& cgi, const std::string& needle);

	static std::string decodeURIComponent(const std::string& data);
};

}  // namespace ots

#endif  //ots_CgiDataUtilities_h
