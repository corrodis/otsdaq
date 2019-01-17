#ifndef _ots_FEVInterfacesManager_h_
#define _ots_FEVInterfacesManager_h_

#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/Configurable/Configurable.h"
#include <map>
#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include "otsdaq-core/FECore/FEVInterface.h"

namespace ots
{
//FEVInterfacesManager
//	This class is a virtual class that handles a collection of front-end interface plugins.
class FEVInterfacesManager : public VStateMachine, public Configurable
{
public:

	FEVInterfacesManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath);
    virtual ~FEVInterfacesManager(void);

    //Methods
    void init              (void);
    void destroy           (void);
    void createInterfaces  (void);

    //State Machine Methods
    void configure         (void);
    void halt              (void);
    void initialize        (void);
    void pause             (void);
    void resume            (void);
    void start             (std::string runNumber);
    void stop              (void);



    void				universalRead	 					(const std::string& interfaceID, char* address, char* returnValue); //used by MacroMaker
    void 			    universalWrite	  					(const std::string& interfaceID, char* address, char* writeValue); //used by MacroMaker
    std::string  	    getFEListString						(const std::string& supervisorLid);	//used by MacroMaker
    std::string  	    getFEMacrosString					(const std::string& supervisorName, const std::string& supervisorLid);	//used by MacroMaker
    void		  	    runFEMacro							(const std::string& interfaceID, const FEVInterface::frontEndMacroStruct_t& feMacro, const std::string& inputArgs, std::string& outputArgs);	//used by MacroMaker and FE calling indirectly
    void		  	    runFEMacro							(const std::string& interfaceID, const std::string& feMacroName, const std::string& inputArgs, std::string& outputArgs);	//used by MacroMaker
    void		  	    runFEMacroByFE						(const std::string& callingInterfaceID, const std::string& interfaceID, const std::string& feMacroName, const std::string& inputArgs, std::string& outputArgs);	//used by FE calling (i.e. FESupervisor)
    unsigned int		getInterfaceUniversalAddressSize	(const std::string& interfaceID); 	//used by MacroMaker
    unsigned int		getInterfaceUniversalDataSize		(const std::string& interfaceID);	//used by MacroMaker
    bool				allFEWorkloopsAreDone				(void); //used by Iterator, e.g.
    const FEVInterface&	getFEInterface						(const std::string& interfaceID) const;


    //FE communication helpers
	std::mutex									frontEndCommunicationReceiveMutex_;
	std::map<std::string /*targetInterfaceID*/,  //map of target to buffers organized by source
		std::map<std::string /*sourceInterfaceID*/,
			std::queue<std::string /*value*/> > >  	frontEndCommunicationReceiveBuffer_;

private:

	FEVInterface*		getFEInterfaceP		(const std::string& interfaceID);
    std::map<std::string /*name*/, std::unique_ptr<FEVInterface> > 	theFEInterfaces_;
    std::vector<std::string /*name*/> 								theFENamesByPriority_;


    //for managing transition iterations
    std::map<std::string /*name*/, bool /*isDone*/ > 				stateMachinesIterationDone_;
    unsigned int					stateMachinesIterationWorkCount_;
    unsigned int					subIterationWorkStateMachineIndex_;
    void							preStateMachineExecution(unsigned int i);
    bool							postStateMachineExecution(unsigned int i);
    void							preStateMachineExecutionLoop(void);
    void							postStateMachineExecutionLoop(void);


};

}

#endif
