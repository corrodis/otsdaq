#ifndef _ots_SOAPMessenger_h
#define _ots_SOAPMessenger_h

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include <xcept/tools.h>


#include "otsdaq-core/Macros/CoutHeaderMacros.h" /* for XDAQ_CONST_CALL */
#include <string>

namespace ots
{
class SOAPCommand;
class SOAPParameters;

class SOAPMessenger : public virtual toolbox::lang::Class
{
public:

    SOAPMessenger(xdaq::Application* application);
    SOAPMessenger(const SOAPMessenger& aSOAPMessenger);

    std::string   receive(const xoap::MessageReference& message);
    std::string   receive(const xoap::MessageReference& message, SOAPCommand&    soapCommand);
    std::string   receive(const xoap::MessageReference& message, SOAPParameters& parameters);

    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, xoap::MessageReference message)                 ;
    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, SOAPCommand soapCommand)                        ;
    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command)                            ;
    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command, SOAPParameters parameters) ;
    std::string   sendStatus(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string message)                            ;

    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, xoap::MessageReference message)                   ;
    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, SOAPCommand soapCommand)                        ;
    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command)                              ;
    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command, SOAPParameters parameters) ;

protected:
    xdaq::Application* theApplication_;
};
}
#endif
