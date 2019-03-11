#ifndef _ots_CorePropertySupervisorBase_h_
#define _ots_CorePropertySupervisorBase_h_

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"
#include "otsdaq-core/TableCore/TableGroupKey.h"
#include "otsdaq-core/TablePluginDataFormats/XDAQContextTable.h"

#include "otsdaq-core/WebUsersUtilities/WebUsers.h"  //for WebUsers::RequestUserInfo

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop

namespace ots
{
// CorePropertySupervisorBase
//	This class provides supervisor property get and set functionality. It has member
// variables 		generally useful to the configuration of client supervisors.
class CorePropertySupervisorBase
{
	friend class GatewaySupervisor; // for access to indicateOtsAlive()

  public:
	CorePropertySupervisorBase(xdaq::Application* application);
	virtual ~CorePropertySupervisorBase(void);


	AllSupervisorInfo     allSupervisorInfo_;
	ConfigurationManager* theConfigurationManager_;


	virtual void setSupervisorPropertyDefaults(
	    void);  // override to control supervisor specific defaults
	virtual void forceSupervisorPropertyValues(void)
	{
		;
	}  // override to force supervisor property values (and ignore user settings)

	void getRequestUserInfo(WebUsers::RequestUserInfo& requestUserInfo);

	// supervisors should use these two static functions to standardize permissions
	// access:
	static void extractPermissionsMapFromString(
	    const std::string&                                  permissionsString,
	    std::map<std::string, WebUsers::permissionLevel_t>& permissionsMap);
	static bool doPermissionsGrantAccess(
	    std::map<std::string, WebUsers::permissionLevel_t>& permissionLevelsMap,
	    std::map<std::string, WebUsers::permissionLevel_t>& permissionThresholdsMap);

	ConfigurationTree getContextTreeNode(void) const
	{
		return theConfigurationManager_->getNode(
		    theConfigurationManager_->__GET_CONFIG__(XDAQContextTable)->getTableName());
	}
	ConfigurationTree getSupervisorTableNode(void) const
	{
		return getContextTreeNode().getNode(supervisorConfigurationPath_);
	}

	const std::string& getContextUID(void) const { return supervisorContextUID_; }
	const std::string& getSupervisorUID(void) const { return supervisorApplicationUID_; }
	const std::string& getSupervisorConfigurationPath(void) const
	{
		return supervisorConfigurationPath_;
	}



  protected:
	const std::string supervisorClass_;
	const std::string supervisorClassNoNamespace_;

  private:
	static void indicateOtsAlive(const CorePropertySupervisorBase* properties = 0);

	std::string supervisorContextUID_;
	std::string supervisorApplicationUID_;
	std::string supervisorConfigurationPath_;

  protected:
	// Supervisor Property names
	//	to access, use CorePropertySupervisorBase::getSupervisorProperty and
	// CorePropertySupervisorBase::setSupervisorProperty
	static const struct SupervisorProperties
	{
		SupervisorProperties()
		    : allSetNames_({&CheckUserLockRequestTypes,
		                    &RequireUserLockRequestTypes,
		                    &AutomatedRequestTypes,
		                    &AllowNoLoginRequestTypes,
		                    &NoXmlWhiteSpaceRequestTypes,
		                    &NonXMLRequestTypes})
		{
		}

		const std::string UserPermissionsThreshold = "UserPermissionsThreshold";
		const std::string UserGroupsAllowed        = "UserGroupsAllowed";
		const std::string UserGroupsDisallowed     = "UserGroupsDisallowed";

		const std::string CheckUserLockRequestTypes   = "CheckUserLockRequestTypes";
		const std::string RequireUserLockRequestTypes = "RequireUserLockRequestTypes";
		const std::string AutomatedRequestTypes       = "AutomatedRequestTypes";
		const std::string AllowNoLoginRequestTypes    = "AllowNoLoginRequestTypes";

		const std::string NoXmlWhiteSpaceRequestTypes = "NoXmlWhiteSpaceRequestTypes";
		const std::string NonXMLRequestTypes          = "NonXMLRequestTypes";

		const std::set<const std::string*> allSetNames_;
	} SUPERVISOR_PROPERTIES;

  private:
	// property private members
	void          checkSupervisorPropertySetup(void);
	volatile bool propertiesAreSetup_;

	// for public access to property map,..
	//	use CorePropertySupervisorBase::getSupervisorProperty and
	// CorePropertySupervisorBase::setSupervisorProperty
	std::map<std::string, std::string> propertyMap_;
	struct CoreSupervisorPropertyStruct
	{
		CoreSupervisorPropertyStruct()
		    : allSets_({&CheckUserLockRequestTypes,
		                &RequireUserLockRequestTypes,
		                &AutomatedRequestTypes,
		                &AllowNoLoginRequestTypes,
		                &NoXmlWhiteSpaceRequestTypes,
		                &NonXMLRequestTypes})
		{
		}

		std::map<std::string, WebUsers::permissionLevel_t> UserPermissionsThreshold;
		std::map<std::string, std::string>                 UserGroupsAllowed;
		std::map<std::string, std::string>                 UserGroupsDisallowed;

		std::set<std::string> CheckUserLockRequestTypes;
		std::set<std::string> RequireUserLockRequestTypes;
		std::set<std::string> AutomatedRequestTypes;
		std::set<std::string> AllowNoLoginRequestTypes;

		std::set<std::string> NoXmlWhiteSpaceRequestTypes;
		std::set<std::string> NonXMLRequestTypes;

		std::set<std::set<std::string>*> allSets_;
	} propertyStruct_;

  public:
	void resetPropertiesAreSetup(void)
	{
		propertiesAreSetup_ = false;
	}  // forces reload of properties from configuration
	ConfigurationTree getSupervisorTreeNode(void);

	void loadUserSupervisorProperties(void);
	template<class T>
	void setSupervisorProperty(const std::string& propertyName, const T& propertyValue)
	{
		std::stringstream ss;
		ss << propertyValue;
		setSupervisorProperty(propertyName, ss.str());
	}
	void setSupervisorProperty(const std::string& propertyName,
	                           const std::string& propertyValue);
	template<class T>
	void addSupervisorProperty(const std::string& propertyName, const T& propertyValue)
	{
		// prepend new values.. since map/set extraction takes the first value encountered
		std::stringstream ss;
		ss << propertyValue << " | " << getSupervisorProperty(propertyName);
		setSupervisorProperty(propertyName, ss.str());
	}
	void addSupervisorProperty(const std::string& propertyName,
	                           const std::string& propertyValue);
	template<class T>
	T getSupervisorProperty(const std::string& propertyName)
	{
		// check if need to setup properties
		checkSupervisorPropertySetup();

		auto it = propertyMap_.find(propertyName);
		if(it == propertyMap_.end())
		{
			__SS__ << "Could not find property named " << propertyName << __E__;
			__SS_THROW__;
		}
		return StringMacros::validateValueForDefaultStringDataType<T>(it->second);
	}
	std::string                 getSupervisorProperty(const std::string& propertyName);
	WebUsers::permissionLevel_t getSupervisorPropertyUserPermissionsThreshold(
	    const std::string& requestType);
};

}  // namespace ots

#endif
