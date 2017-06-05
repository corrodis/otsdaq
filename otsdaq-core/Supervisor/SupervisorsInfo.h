#ifndef _ots_SupervisorsInfo_h
#define _ots_SupervisorsInfo_h

#include <map>
#include <xdata/xdata.h>
#include "SupervisorInfo.h"

namespace ots
{

typedef std::map<xdata::UnsignedIntegerT, SupervisorInfo> SupervisorsInfoMap;

class SupervisorDescriptorInfoBase;

class SupervisorsInfo
{
	
public:
    SupervisorsInfo (void);
    ~SupervisorsInfo(void);

    void init(const SupervisorDescriptorInfoBase& supervisorDescriptorInfo);
    SupervisorInfo& getSupervisorInfo                    (void);
    SupervisorInfo& getFESupervisorInfo                  (xdata::UnsignedIntegerT instance);
    //SupervisorInfo& getARTDAQFESupervisorInfo          (xdata::UnsignedIntegerT instance);
    SupervisorInfo& getARTDAQFEDataManagerSupervisorInfo (xdata::UnsignedIntegerT instance);
    SupervisorInfo& getARTDAQDataManagerSupervisorInfo   (xdata::UnsignedIntegerT instance);
    SupervisorInfo& getVisualSupervisorInfo              (void);

private:
    SupervisorInfo     theSupervisorInfo_;
	SupervisorsInfoMap theFESupervisorsInfo_;
	//SupervisorsInfoMap theARTDAQFESupervisorsInfo_;
	SupervisorsInfoMap theARTDAQFEDataManagerSupervisorsInfo_;
	SupervisorsInfoMap theARTDAQDataManagerSupervisorsInfo_;
    SupervisorInfo     theVisualSupervisorInfo_;

};

}

#endif
