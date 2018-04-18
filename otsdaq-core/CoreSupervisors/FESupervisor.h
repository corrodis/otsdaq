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

    FESupervisor                    (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~FESupervisor           (void);

    xoap::MessageReference 			macroMakerSupervisorRequest  	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    virtual xoap::MessageReference 	workLoopStatusRequest		  	(xoap::MessageReference message )  	throw (xoap::exception::Exception);


protected:
    FEVInterfacesManager*		   	extractFEInterfaceManager();
};

}

#endif
