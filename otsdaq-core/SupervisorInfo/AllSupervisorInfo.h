#ifndef _ots_AllSupervisorInfo_h
#define _ots_AllSupervisorInfo_h

#include <map>
#include <vector>

#include "otsdaq-core/SupervisorInfo/SupervisorInfo.h"
#include "otsdaq-core/SupervisorInfo/SupervisorDescriptorInfoBase.h"

namespace ots
{

////// type define AllSupervisorInfoMap
typedef std::map<unsigned int, const SupervisorInfo&> SupervisorInfoMap;

////// class define
class AllSupervisorInfo : public SupervisorDescriptorInfoBase
{

public:
    AllSupervisorInfo 																(void);
    AllSupervisorInfo																(xdaq::ApplicationContext* applicationContext);
    ~AllSupervisorInfo																(void);

    void 											init							(xdaq::ApplicationContext* applicationContext);
    void 											destroy							(void);

    //BOOLs
    bool											isWizardMode					(void) const { return theWizardInfo_?true:false; }


    //SETTERs
    void											setSupervisorStatus				(xdaq::Application* app, 		const std::string& status);
    void											setSupervisorStatus				(const SupervisorInfo& appInfo, const std::string& status);
    void											setSupervisorStatus				(const unsigned int& id,  		const std::string& status);


    //GETTERs (so searching and iterating is easier)
    const std::map<unsigned int, SupervisorInfo>&	getAllSupervisorInfo			(void) const { return allSupervisorInfo_; }
    const SupervisorInfoMap&						getAllFETypeSupervisorInfo		(void) const { return allFETypeSupervisorInfo_; }
    const SupervisorInfoMap&						getAllDMTypeSupervisorInfo		(void) const { return allDMTypeSupervisorInfo_; }
    const SupervisorInfoMap&						getAllLogbookTypeSupervisorInfo	(void) const { return allLogbookTypeSupervisorInfo_; }

    const SupervisorInfo&							getSupervisorInfo				(xdaq::Application* app) const;
    const SupervisorInfo& 							getGatewayInfo	                (void) const;
    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* 				getGatewayDescriptor            (void) const;
    const SupervisorInfo& 							getWizardInfo	                (void) const;
    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* 				getWizardDescriptor             (void) const;

    std::vector<const SupervisorInfo*>				getOrderedSupervisorDescriptors (const std::string& stateMachineCommand) const;

private:

    SupervisorInfo*    								theSupervisorInfo_;
    SupervisorInfo*    								theWizardInfo_;


    std::map<unsigned int, SupervisorInfo> 			allSupervisorInfo_;
    SupervisorInfoMap		 						allFETypeSupervisorInfo_, allDMTypeSupervisorInfo_, allLogbookTypeSupervisorInfo_;


};

}

#endif
