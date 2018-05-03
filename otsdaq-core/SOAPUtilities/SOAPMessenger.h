#ifndef _ots_SOAPMessenger_h
#define _ots_SOAPMessenger_h

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include <xcept/tools.h>


#include "otsdaq-core/Macros/CoutMacros.h" /* for XDAQ_CONST_CALL */
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

    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, xoap::MessageReference message)                 throw (xdaq::exception::Exception);
    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, SOAPCommand soapCommand)                        throw (xdaq::exception::Exception);
    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command)                            throw (xdaq::exception::Exception);
    std::string   send      (XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command, SOAPParameters parameters) throw (xdaq::exception::Exception);
    std::string   sendStatus(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string message)                            throw (xdaq::exception::Exception);

    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, xoap::MessageReference message)                   throw (xdaq::exception::Exception);
    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, SOAPCommand soapCommand)                        throw (xdaq::exception::Exception);
    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command)                              throw (xdaq::exception::Exception);
    xoap::MessageReference sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command, SOAPParameters parameters) throw (xdaq::exception::Exception);

protected:
    xdaq::Application* theApplication_;
};
}
#endif
