#ifndef _ots_WizardSupervisor_h
#define _ots_WizardSupervisor_h

#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"

#include <xdaq/Application.h>
#include "otsdaq-core/Macros/XDAQApplicationMacros.h"
#include <xgi/Method.h>

#include <xoap/SOAPEnvelope.h>
#include <xoap/SOAPBody.h>
#include <xoap/domutils.h>
#include <xoap/Method.h>

#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPCookie.h>
#include <cgicc/HTMLDoctype.h>
#include <cgicc/HTTPHeader.h>

#include <string>
#include <map>

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"

namespace ots
{

class HttpXmlDocument;

//WizardSupervisor
//	This class is a xdaq application.
//
//	It is instantiated by the xdaq context when otsdaq is in "Wiz Mode."
//
//	It is different from the "Normal Mode" Gateway Supervisor in that it does not have a state machine
//	and does not inherit properties from CorePropertySupervisorBase. The assumption is
//	that only admins have access to wiz mode, and they have access to all features of it.
class WizardSupervisor: public xdaq::Application, public SOAPMessenger
{

public:

    XDAQ_INSTANTIATOR();

    WizardSupervisor         						(xdaq::ApplicationStub *) throw (xdaq::exception::Exception);
    virtual ~WizardSupervisor						(void);


    void 						init                  				(void);
    void 						destroy                     		(void);

    void 						generateURL            				(void);
    static void 				printURL							(WizardSupervisor *ptr, std::string securityCode);

    void 						Default                    			(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void 						verification               		 	(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void						request		                		(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void						requestIcons                		(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);

    void 						editSecurity                		(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void 						UserSettings              			(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void 						tooltipRequest                		(xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void                        toggleSecurityCodeGeneration        (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
	std::string     			validateUploadFileType      		(const std::string fileType);
    void            			cleanUpPreviews             		();
    void            			savePostPreview             		(std::string &subject, std::string &text, const std::vector<cgicc::FormFile> &files, std::string creator, HttpXmlDocument *xmldoc = nullptr);
    std::string 				exec								(const char* cmd);

    //External Supervisor XOAP handlers
    xoap::MessageReference 		supervisorSequenceCheck 			(xoap::MessageReference msg) throw (xoap::exception::Exception);
    xoap::MessageReference 		supervisorLastConfigGroupRequest	(xoap::MessageReference msg) throw (xoap::exception::Exception);

private:

    std::string					securityCode_;
    bool                        defaultSequence_;
    std::vector<std::string>    allowedFileUploadTypes_, matchingFileUploadTypes_;


    std::string					supervisorClass_;
    std::string					supervisorClassNoNamespace_;

    enum {
        	ADMIN_PERMISSIONS_THRESHOLD = 255,
        	EXPERIMENT_NAME_MIN_LENTH = 3,
        	EXPERIMENT_NAME_MAX_LENTH = 25,
        	USER_DATA_EXPIRATION_TIME = 60*20, //20 minutes
        };

};

}

#endif
