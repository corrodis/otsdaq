#ifndef _ots_SOAPParameters_h
#define _ots_SOAPParameters_h

#include "otsdaq-core/SOAPUtilities/DataStructs.h"
#include "otsdaq-core/SOAPUtilities/SOAPParameter.h"

#include <string>

namespace ots
{

class SOAPParameters : public Parameters<std::string,std::string>
{
public:
    SOAPParameters(void);
    SOAPParameters(const std::string& name, const std::string& value="");
    SOAPParameters(SOAPParameter parameter);
    ~SOAPParameters(void);
    void addParameter(const std::string& name, const std::string& value="");
    void addParameter(const std::string& name, const int value);

};
}
#endif
