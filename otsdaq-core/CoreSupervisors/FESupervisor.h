#ifndef _ots_FESupervisor_h_
#define _ots_FESupervisor_h_

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

namespace ots
{

class FEVInterfacesManager;
class FESupervisor: public CoreSupervisorBase
{

public:

    XDAQ_INSTANTIATOR();

    FESupervisor                    (xdaq::ApplicationStub * s) ;
    virtual ~FESupervisor           (void);

    xoap::MessageReference 			macroMakerSupervisorRequest  	(xoap::MessageReference message )  	;
    virtual xoap::MessageReference 	workLoopStatusRequest		  	(xoap::MessageReference message )  	;


protected:
    FEVInterfacesManager*		   	extractFEInterfaceManager();
};

}

#endif
