#ifndef _ots_AllSupervisorInfo_h
#define _ots_AllSupervisorInfo_h

#include <map>
#include <vector>

#include "otsdaq/SupervisorInfo/SupervisorDescriptorInfoBase.h"
#include "otsdaq/SupervisorInfo/SupervisorInfo.h"

namespace ots
{
////// type define AllSupervisorInfoMap
typedef std::map<unsigned int, const SupervisorInfo&> SupervisorInfoMap;

////// class define
// AllSupervisorInfo
//	xdaq Supervisors can use this class to gain access to
//	info for all supervisors in the xdaq Context. Supervisors
//	are organized by type/class. Note that if a supervisor is
//	encountered in the xdaq context that is of unknown type, then
//	it is ignored and not organized.
//
//	Supervisors should call init to setup data members of this class.
//
//	This class, when in normal mode, also interprets the active configuration
//	to associate configuration UID/names to the supervisors in the xdaq context.
//	In wizard mode, UID/name is taken from class name.
class AllSupervisorInfo : public SupervisorDescriptorInfoBase
{
  public:
	AllSupervisorInfo(void);
	AllSupervisorInfo(xdaq::ApplicationContext* applicationContext);
	~AllSupervisorInfo(void);

	void init(xdaq::ApplicationContext* applicationContext);
	void destroy(void);

	// BOOLs
	bool isWizardMode(void) const { return theWizardInfo_ ? true : false; }
	bool isMacroMakerMode(void) const { return AllSupervisorInfo::MACROMAKER_MODE; }

	// SETTERs
	void setSupervisorStatus(xdaq::Application* app, const std::string& status);
	void setSupervisorStatus(const SupervisorInfo& appInfo, const std::string& status);
	void setSupervisorStatus(const unsigned int& id, const std::string& status);

	void setSupervisorProgress(const SupervisorInfo& appInfo, const unsigned int progress);
	void setSupervisorProgress(const unsigned int& id, const unsigned int progress);

	// GETTERs (so searching and iterating is easier)
	const std::map<unsigned int /* lid */, SupervisorInfo>& getAllSupervisorInfo(void) const { return allSupervisorInfo_; }
	const SupervisorInfoMap&                                getAllFETypeSupervisorInfo(void) const { return allFETypeSupervisorInfo_; }
	const SupervisorInfoMap&                                getAllDMTypeSupervisorInfo(void) const { return allDMTypeSupervisorInfo_; }
	const SupervisorInfoMap&                                getAllLogbookTypeSupervisorInfo(void) const { return allLogbookTypeSupervisorInfo_; }
	const SupervisorInfoMap&                                getAllMacroMakerTypeSupervisorInfo(void) const { return allMacroMakerTypeSupervisorInfo_; }

	const SupervisorInfo& getSupervisorInfo(xdaq::Application* app) const;
	const SupervisorInfo& getGatewayInfo(void) const;
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getGatewayDescriptor(void) const;
	const SupervisorInfo&                        getWizardInfo(void) const;
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getWizardDescriptor(void) const;

	std::vector<std::vector<const SupervisorInfo*>> getOrderedSupervisorDescriptors(const std::string& stateMachineCommand) const;

  private:
	SupervisorInfo* theSupervisorInfo_;
	SupervisorInfo* theWizardInfo_;

	std::map<unsigned int /* lid */, SupervisorInfo> allSupervisorInfo_;
	SupervisorInfoMap allFETypeSupervisorInfo_, allDMTypeSupervisorInfo_, allLogbookTypeSupervisorInfo_, allMacroMakerTypeSupervisorInfo_;

	static const bool MACROMAKER_MODE;
};

}  // namespace ots

#endif
