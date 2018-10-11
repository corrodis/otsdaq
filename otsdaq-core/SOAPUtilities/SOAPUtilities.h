#ifndef _ots_SOAPUtilities_h
#define _ots_SOAPUtilities_h

#include <xoap/MessageReference.h>

#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include <string>

namespace ots
{

class SOAPUtilities
{
public:

    static xoap::MessageReference makeSOAPMessageReference(SOAPCommand soapCommand);
    static xoap::MessageReference makeSOAPMessageReference(std::string command);
    static xoap::MessageReference makeSOAPMessageReference(std::string command, SOAPParameters parameters);
    static xoap::MessageReference makeSOAPMessageReference(std::string command, std::string fileName);

    static void                   addParameters           (xoap::MessageReference& message, SOAPParameters parameters);

    static SOAPCommand translate (const xoap::MessageReference& message);
private:
    SOAPUtilities(void);
    ~SOAPUtilities(void);

};
}
#endif
