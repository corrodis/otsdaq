#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
//#include "otsdaq-core/DataTypes/DataStructs.h"

#include <xoap/Method.h>
#include <xdaq/NamespaceURI.h>
#include <xoap/MessageReference.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include <xoap/MessageFactory.h>
#pragma GCC diagnostic pop
#include <xoap/SOAPPart.h>
#include <xoap/SOAPEnvelope.h>
#include <xoap/SOAPBody.h>
#include <xoap/domutils.h>
#include <xoap/AttachmentPart.h>

//#include <iostream>
//#include <sys/time.h>





using namespace ots;

//========================================================================================================================
SOAPMessenger::SOAPMessenger(xdaq::Application* application) :
					theApplication_(application)
{
}

//========================================================================================================================
SOAPMessenger::SOAPMessenger(const SOAPMessenger& aSOAPMessenger) :
					theApplication_(aSOAPMessenger.theApplication_)
{
}

//========================================================================================================================
std::string SOAPMessenger::receive(const xoap::MessageReference& message, SOAPCommand& soapCommand)
{
	return receive(message, soapCommand.getParametersRef());
}

//========================================================================================================================
std::string SOAPMessenger::receive(const xoap::MessageReference& message)
{
	//NOTE it is assumed that there is only 1 command for each message (that's why we use begin)
	return (message->getSOAPPart().getEnvelope().getBody().getChildElements()).begin()->getElementName().getLocalName();
}

//========================================================================================================================
std::string SOAPMessenger::receive(const xoap::MessageReference& message, SOAPParameters& parameters)
{
	xoap::SOAPEnvelope        envelope    	= message->getSOAPPart().getEnvelope();
	std::vector<xoap::SOAPElement> bodyList = envelope.getBody().getChildElements();
	xoap::SOAPElement         command     	= bodyList[0];
	std::string               commandName 	= command.getElementName().getLocalName();
	xoap::SOAPName            name        	= envelope.createName("Key");

	for (SOAPParameters::iterator it=parameters.begin(); it!=parameters.end(); it++)
	{
		name = envelope.createName(it->first);

		try
		{
			it->second = command.getAttributeValue(name);
			//if( parameters.getParameter(it->first).isEmpty() )
			//{
			//    __COUT__ << "Complaint from " << (theApplication_->getApplicationDescriptor()->getClassName()) << std::endl;
			//    __COUT__ << " : Parameter "<< it->first
			//    << " does not exist in the list of incoming parameters!" << std::endl;
			//    __COUT__ << "It could also be because you passed an empty std::string" << std::endl;
			//    //assert(0);
			//};
		}
		catch (xoap::exception::Exception& e)
		{
			__COUT__ << "Parameter " << it->first << " does not exist in the list of incoming parameters!" << std::endl;
			XCEPT_RETHROW(xoap::exception::Exception,"Looking for parameter that does not exist!",e);
		}

	}

	return commandName;
}

//========================================================================================================================
// in xdaq 
//    xdaq::ApplicationDescriptor* sourceptr;
// void getURN()
//
std::string SOAPMessenger::send(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* ind,
		xoap::MessageReference message)

{
	return receive(sendWithSOAPReply(ind,message));
}
//{
//	//const_cast away the const
//	//	so that this line is compatible with slf6 and slf7 versions of xdaq
//	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
//	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d =
//			const_cast<xdaq::ApplicationDescriptor*>(ind);
//	try
//	{
//		message->getMimeHeaders()->setHeader("Content-Location", d->getURN());
//
//		__COUT__ << d->getURN() << __E__;
//		__COUT__ << SOAPUtilities::translate(message) << __E__;
//		std::string mystring;
//		message->writeTo(mystring);
//		__COUT__<< mystring << std::endl;
//
//		xoap::MessageReference reply = theApplication_->getApplicationContext()->postSOAP(message,
//				*(theApplication_->getApplicationDescriptor()),
//				*d);
//		std::string replyString = receive(reply);
//		__COUT__ << "replyString " << replyString << std::endl;
//		return replyString;
//	}
//	catch (xdaq::exception::Exception& e)
//	{
//		__COUT__ << "This application failed to send a SOAP message to "
//				<< d->getClassName() << " instance " << d->getInstance()
//				<< " re-throwing exception = " << xcept::stdformat_exception_history(e);
//		std::string mystring;
//		message->writeTo(mystring);
//		__COUT__<< mystring << std::endl;
//		XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
//	}
//}

//========================================================================================================================
std::string SOAPMessenger::send(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, SOAPCommand soapCommand)

{
	if(soapCommand.hasParameters())
		return send(d, soapCommand.getCommand(), soapCommand.getParameters());
	else
		return send(d, soapCommand.getCommand());
}

//========================================================================================================================
std::string SOAPMessenger::send(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string command)

{
	xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(command);
	return send(d, message);
}

//========================================================================================================================
std::string SOAPMessenger::send(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* ind, std::string cmd,
		SOAPParameters parameters)

{
	return receive(sendWithSOAPReply(ind,cmd,parameters));
}

//{
//	//const_cast away the const
//	//	so that this line is compatible with slf6 and slf7 versions of xdaq
//	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
//	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d =
//			const_cast<xdaq::ApplicationDescriptor*>(ind);
//
//	xoap::MessageReference message;
//	try
//	{
//		message = SOAPUtilities::makeSOAPMessageReference(cmd, parameters);
//		message->getMimeHeaders()->setHeader("Content-Location", d->getURN());
//
//		__COUT__ << d->getURN() << __E__;
//		__COUT__ << SOAPUtilities::translate(message) << __E__;
//		std::string mystring;
//		message->writeTo(mystring);
//		__COUT__<< mystring << std::endl;
//
//		xoap::MessageReference reply = theApplication_->getApplicationContext()->postSOAP(message,
//				*(theApplication_->getApplicationDescriptor()), *d);
//		std::string replyString = receive(reply);
//		return replyString;
//	}
//	catch (xdaq::exception::Exception& e)
//	{
//		__COUT__ << "This application failed to send a SOAP message to "
//				<< d->getClassName() << " instance " << d->getInstance()
//				<< " with command = " << cmd
//				<< " re-throwing exception = " << xcept::stdformat_exception_history(e)<<std::endl;
//
//		std::string mystring;
//		message->writeTo(mystring);
//		__COUT_ERR__ << mystring;
//		XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
//	}
//
//}

//========================================================================================================================
xoap::MessageReference SOAPMessenger::sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* ind,
		std::string cmd)

{
	return sendWithSOAPReply(ind, SOAPUtilities::makeSOAPMessageReference(cmd));
}
//{
//	//const_cast away the const
//	//	so that this line is compatible with slf6 and slf7 versions of xdaq
//	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
//	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d =
//			const_cast<xdaq::ApplicationDescriptor*>(ind);
//	try
//	{
//		xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(cmd);
//		message->getMimeHeaders()->setHeader("Content-Location", d->getURN());
//
//		__COUT__ << d->getURN() << __E__;
//		__COUT__ << SOAPUtilities::translate(message) << __E__;
//		std::string mystring;
//		message->writeTo(mystring);
//		__COUT__<< mystring << std::endl;
//
//		xoap::MessageReference reply = theApplication_->getApplicationContext()->postSOAP(message,
//				*(theApplication_->getApplicationDescriptor()),
//				*d);
//		return reply;
//	}

//	catch (xdaq::exception::Exception& e)
//	{
//		__COUT__ << "This application failed to send a SOAP message to "
//				<< d->getClassName() << " instance " << d->getInstance()
//				<< " with command = " << cmd
//				<< " re-throwing exception = " << xcept::stdformat_exception_history(e)<<std::endl;
//		XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
//	}
//
//}

//========================================================================================================================
xoap::MessageReference SOAPMessenger::sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor *ind,
		xoap::MessageReference message)

{
	//const_cast away the const
	//	so that this line is compatible with slf6 and slf7 versions of xdaq
	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d =
			const_cast<xdaq::ApplicationDescriptor*>(ind);
	try
	{
		message->getMimeHeaders()->setHeader("Content-Location", d->getURN());

		//__COUT__ << d->getURN() << __E__;
		//__COUT__ << SOAPUtilities::translate(message) << __E__;
		std::string mystring;
		message->writeTo(mystring);
		//__COUT__<< mystring << std::endl;

		xoap::MessageReference reply = theApplication_->getApplicationContext()->postSOAP(message,
				*(theApplication_->getApplicationDescriptor()),
				*d);
		return reply;
	}
	catch (xdaq::exception::Exception& e)
	{
		__COUT__ << "This application failed to send a SOAP message to "
				<< d->getClassName() << " instance " << d->getInstance()
				<< " re-throwing exception = " << xcept::stdformat_exception_history(e);
		std::string mystring;
		message->writeTo(mystring);
		__COUT_ERR__<< mystring << std::endl;
		XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
	}
}

//========================================================================================================================
xoap::MessageReference SOAPMessenger::sendWithSOAPReply(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* ind,
		std::string cmd, SOAPParameters parameters)

{
	return sendWithSOAPReply(ind, SOAPUtilities::makeSOAPMessageReference(cmd, parameters));
}
//{
//	//const_cast away the const
//	//	so that this line is compatible with slf6 and slf7 versions of xdaq
//	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
//	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d =
//			const_cast<xdaq::ApplicationDescriptor*>(ind);
//	try
//	{
//		xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(cmd, parameters);
//		message->getMimeHeaders()->setHeader("Content-Location", d->getURN());
//		xoap::MessageReference reply = theApplication_->getApplicationContext()->postSOAP(message,
//				*(theApplication_->getApplicationDescriptor()), *d);
//		return reply;
//	}
//	catch (xdaq::exception::Exception& e)
//	{
//		__COUT__ << "This application failed to send a SOAP message to "
//				<< d->getClassName() << " instance " << d->getInstance()
//				<< " with command = " << cmd
//				<< " re-throwing exception = " << xcept::stdformat_exception_history(e);
//		XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
//	}
//
//}

//========================================================================================================================
/*
std::string SOAPMessenger::send(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d, std::string cmd, std::string filepath) 
{

    try
    {
        __COUT__ << "SOAP XML file path : " << filepath << std::endl;
        xoap::MessageReference message = xoap::createMessage();
        xoap::SOAPPart soap = message->getSOAPPart();
        xoap::SOAPEnvelope envelope = soap.getEnvelope();
        xoap::AttachmentPart * attachment;
        attachment = message->createAttachmentPart();
        attachment->setContent(filepath);
        attachment->setContentId("SOAPTEST1");
        attachment->addMimeHeader("Content-Description", "This is a SOAP message with attachments");
        message->addAttachmentPart(attachment);
        xoap::SOAPName command = envelope.createName(cmd, "xdaq", XDAQ_NS_URI);
        xoap::SOAPBody body= envelope.getBody();
        body.addBodyElement(command);
#ifdef DEBUGMSG
        std::string mystring;
        message->writeTo(mystring);
        __COUT__ << "SOAP Message : "<< mystring <<std::endl << std::endl;
#endif
        message->getMimeHeaders()->setHeader("Content-Location", d->getURN());
        xoap::MessageReference reply=theApplication_->getApplicationContext()->postSOAP(message, *(theApplication_->getApplicationDescriptor()), *d);
        std::string replyString = receive(reply);
        return replyString;
    }
    catch (xdaq::exception::Exception& e)
    {
        XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
    }
}
 */
//========================================================================================================================
std::string SOAPMessenger::sendStatus(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* ind,
		std::string message)

{
	//const_cast away the const
	//	so that this line is compatible with slf6 and slf7 versions of xdaq
	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* d =
			const_cast<xdaq::ApplicationDescriptor*>(ind);

	std::string cmd = "StatusNotification";
	try
	{
		timeval tv;	    //keep track of when the message comes
		gettimeofday(&tv,NULL);

		std::stringstream ss;
		SOAPParameters parameters;
		parameters.addParameter("Description",message);
		ss.str(""); ss << tv.tv_sec;
		parameters.addParameter("Time",ss.str());
		ss.str(""); ss << tv.tv_usec;
		parameters.addParameter("usec",ss.str());
		return send(d, cmd, parameters);
	}
	catch (xdaq::exception::Exception& e)
	{
		__COUT__ << "This application failed to send a SOAP error message to "
				<< d->getClassName() << " instance " << d->getInstance()
				<< " with command = " << cmd
				<< " re-throwing exception = " << xcept::stdformat_exception_history(e) << std::endl;
		XCEPT_RETHROW(xdaq::exception::Exception,"Failed to send SOAP command.",e);
	}
}
