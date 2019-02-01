#ifndef _ots_SupervisorConfigurationBase_h_
#define _ots_SupervisorConfigurationBase_h_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "xdaq/Application.h"
#pragma GCC diagnostic pop
#include "otsdaq-core/Macros/XDAQApplicationMacros.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <string>
#include <map>

namespace ots
{
// key is the crate number
typedef std::map<xdata::UnsignedIntegerT, XDAQ_CONST_CALL xdaq::ApplicationDescriptor*> SupervisorDescriptors;

class SupervisorDescriptorInfoBase
{

	friend class SupervisorInfo; //"Friend" class needs access to private members
public:

    SupervisorDescriptorInfoBase 												(void);
    SupervisorDescriptorInfoBase												(xdaq::ApplicationContext* applicationContext);
    virtual ~SupervisorDescriptorInfoBase										(void);

    //XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getSupervisorDescriptor 	    (void) const;
    //XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getWizardDescriptor 	        (void) const;

protected:
    virtual void init												(xdaq::ApplicationContext* applicationContext);
    virtual void destroy											();


    const SupervisorDescriptors& getAllDescriptors               	(void) const;
//    const SupervisorDescriptors& getAllFEDescriptors               	(void) const;

//    const SupervisorDescriptors& getDataManagerDescriptors        (void) const;
//    const SupervisorDescriptors& getFEDescriptors                 (void) const;
//    //const SupervisorDescriptors& getDTCDescriptors                (void) const;
//    const SupervisorDescriptors& getFEDataManagerDescriptors      (void) const;
//    //const SupervisorDescriptors& getARTDAQFEDescriptors         (void) const;
//    const SupervisorDescriptors& getARTDAQFEDataManagerDescriptors(void) const;
//    const SupervisorDescriptors& getARTDAQDataManagerDescriptors  (void) const;
//    const SupervisorDescriptors& getARTDAQBuilderDescriptors      (void) const;
//    const SupervisorDescriptors& getARTDAQAggregatorDescriptors   (void) const;
//    const SupervisorDescriptors& getVisualDescriptors             (void) const;


   // XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getLogbookDescriptor 	         (void) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getDataManagerDescriptor        (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getFEDescriptor        	     (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getDTCDescriptor        	     (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getFEDataManagerDescriptor      (xdata::UnsignedIntegerT instance) const;
//    //xdaq::ApplicationDescriptor* getARTDAQFEDescriptor          (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getARTDAQFEDataManagerDescriptor(xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getARTDAQDataManagerDescriptor  (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getARTDAQBuilderDescriptor      (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getARTDAQAggregatorDescriptor   (xdata::UnsignedIntegerT instance) const;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* getVisualDescriptor   	         (xdata::UnsignedIntegerT instance) const;

 //   std::string 								getFEURL               (xdata::UnsignedIntegerT instance) const;

//    std::string getARTDAQFEURL         (xdata::UnsignedIntegerT instance) const;
//    std::string getARTDAQBuilderURL    (xdata::UnsignedIntegerT instance) const;
//    std::string getARTDAQAggregatorURL (xdata::UnsignedIntegerT instance) const;

protected:
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* theSupervisor_;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* theWizard_;
//    XDAQ_CONST_CALL xdaq::ApplicationDescriptor* theLogbookSupervisor_;
//
//    SupervisorDescriptors        theVisualSupervisors_;
//    SupervisorDescriptors        theDataManagerSupervisors_;
//    SupervisorDescriptors        theFESupervisors_;
//    SupervisorDescriptors        theDTCSupervisors_;
//    SupervisorDescriptors        theFEDataManagerSupervisors_;
//    //SupervisorDescriptors        theARTDAQFESupervisors_;
//    SupervisorDescriptors        theARTDAQFEDataManagerSupervisors_;
//    SupervisorDescriptors        theARTDAQDataManagerSupervisors_;
//    SupervisorDescriptors        theARTDAQBuilderSupervisors_;
//    SupervisorDescriptors        theARTDAQAggregatorSupervisors_;

    SupervisorDescriptors		 allSupervisors_;
    //SupervisorDescriptors		 allFETypeSupervisors_;
};
}
#endif
