#ifndef _ots_SupervisorInfo_h_
#define _ots_SupervisorInfo_h_

#include <string>
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"
#include "otsdaq-core/Macros/CoutMacros.h" /* also for XDAQ_CONST_CALL */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "xdaq/Application.h"       /* for  xdaq::ApplicationDescriptor */
#include "xdaq/ContextDescriptor.h" /* for  xdaq::ContextDescriptor */
#pragma GCC diagnostic pop
#include "otsdaq-core/Macros/XDAQApplicationMacros.h"

namespace ots
{
class SupervisorInfo
{
  public:
	//when no configuration, e.g. Wizard Mode, then
	// name and contextName are derived from the class name and LID
	SupervisorInfo(
	    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* descriptor,
	    const std::string& name, const std::string& contextName)
	    : descriptor_(descriptor), contextDescriptor_(descriptor ? descriptor->getContextDescriptor() : 0), name_(name),  // this is the config app name
	    contextName_(contextName)
	    ,  // this is the config parent context name
	    id_(descriptor ? descriptor->getLocalId() : 0)
	    , class_(descriptor ? descriptor->getClassName() : "")
	    , contextURL_(contextDescriptor_ ? contextDescriptor_->getURL() : "")
	    , URN_(descriptor ? descriptor->getURN() : "")
	    , URL_(contextURL_ + "/" + URN_)
	    , port_(0)
	    , status_("Unknown")
	{
		//when no configuration, e.g. Wizard Mode, then
		// name and contextName are derived from the class name and LID
		if (name_ == "") name_ = id_;
		if (contextName_ == "") contextName_ = class_;

		//  __COUTV__(name_);
		//  __COUTV__(contextName_);
		//  __COUTV__(id_);
		//  __COUTV__(contextURL_);
		//  __COUTV__(URN_);
		//  __COUTV__(URL_);
		//  __COUTV__(port_);
		//  __COUTV__(class_);
	}

	~SupervisorInfo(void)
	{
		;
	}

	//BOOLs	-------------------
	bool isGatewaySupervisor(void) const { return class_ == XDAQContextConfiguration::GATEWAY_SUPERVISOR_CLASS; }
	bool isWizardSupervisor(void) const { return class_ == XDAQContextConfiguration::WIZARD_SUPERVISOR_CLASS; }
	bool isTypeFESupervisor(void) const { return XDAQContextConfiguration::FETypeClassNames_.find(class_) != XDAQContextConfiguration::FETypeClassNames_.end(); }
	bool isTypeDMSupervisor(void) const { return XDAQContextConfiguration::DMTypeClassNames_.find(class_) != XDAQContextConfiguration::DMTypeClassNames_.end(); }
	bool isTypeLogbookSupervisor(void) const { return XDAQContextConfiguration::LogbookTypeClassNames_.find(class_) != XDAQContextConfiguration::LogbookTypeClassNames_.end(); }
	bool isTypeMacroMakerSupervisor(void) const { return XDAQContextConfiguration::MacroMakerTypeClassNames_.find(class_) != XDAQContextConfiguration::MacroMakerTypeClassNames_.end(); }
	bool isTypeConfigurationGUISupervisor(void) const { return XDAQContextConfiguration::ConfigurationGUITypeClassNames_.find(class_) != XDAQContextConfiguration::ConfigurationGUITypeClassNames_.end(); }
	bool isTypeChatSupervisor(void) const { return XDAQContextConfiguration::ChatTypeClassNames_.find(class_) != XDAQContextConfiguration::ChatTypeClassNames_.end(); }
	bool isTypeConsoleSupervisor(void) const { return XDAQContextConfiguration::ConsoleTypeClassNames_.find(class_) != XDAQContextConfiguration::ConsoleTypeClassNames_.end(); }

	//Getters -------------------
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getDescriptor(void) const { return descriptor_; }
	const xdaq::ContextDescriptor*               getContextDescriptor(void) const { return contextDescriptor_; }
	const std::string&                           getName(void) const { return name_; }
	const std::string&                           getContextName(void) const { return contextName_; }
	const unsigned int&                          getId(void) const { return id_; }
	const std::string&                           getClass(void) const { return class_; }
	const std::string&                           getStatus(void) const { return status_; }
	const std::string&                           getURL(void) const { return contextURL_; }
	const std::string&                           getURN(void) const { return URN_; }
	const std::string&                           getFullURL(void) const { return URL_; }
	const uint16_t&                              getPort(void) const { return port_; }

	//Setters -------------------
	void setStatus(const std::string& status) { status_ = status; }
	void clear(void)
	{
		descriptor_        = 0;
		contextDescriptor_ = 0;
		name_              = "";
		id_                = 0;
		contextName_       = "";
		status_            = "Unknown";
	}

  private:
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* descriptor_;
	XDAQ_CONST_CALL xdaq::ContextDescriptor* contextDescriptor_;
	std::string                              name_;
	std::string                              contextName_;
	unsigned int                             id_;
	std::string                              class_;
	std::string                              contextURL_;
	std::string                              URN_;
	std::string                              URL_;
	uint16_t                                 port_;
	std::string                              status_;
};

}  // namespace ots

#endif
