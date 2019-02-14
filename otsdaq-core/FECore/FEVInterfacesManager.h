#ifndef _ots_FEVInterfacesManager_h_
#define _ots_FEVInterfacesManager_h_

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"

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
	void init(void);
	void destroy(void);
	void createInterfaces(void);

	//State Machine Methods
	void configure(void);
	void halt(void);
	void initialize(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber);
	void stop(void);

	void        universalRead(const std::string& interfaceID, char* address, char* returnValue);                                                                                                                                                                                                             //used by MacroMaker
	void        universalWrite(const std::string& interfaceID, char* address, char* writeValue);                                                                                                                                                                                                             //used by MacroMaker
	std::string getFEListString(const std::string& supervisorLid);                                                                                                                                                                                                                                           //used by MacroMaker
	std::string getFEMacrosString(const std::string& supervisorName, const std::string& supervisorLid);                                                                                                                                                                                                      //used by MacroMaker
	void        runFEMacro(const std::string& interfaceID, const FEVInterface::frontEndMacroStruct_t& feMacro, const std::string& inputArgs, std::string& outputArgs);                                                                                                                                       //used by MacroMaker and FE calling indirectly
	void        runFEMacro(const std::string& interfaceID, const std::string& feMacroName, const std::string& inputArgs, std::string& outputArgs);                                                                                                                                                           //used by MacroMaker
	void        runFEMacroByFE(const std::string& callingInterfaceID, const std::string& interfaceID, const std::string& feMacroName, const std::string& inputArgs, std::string& outputArgs);                                                                                                                //used by FE calling (i.e. FESupervisor)
	void        startFEMacroMultiDimensional(const std::string& requester, const std::string& interfaceID, const std::string& feMacroName, const bool enableSavingOutput, const std::string& outputFilePath, const std::string& outputFileRadix, const std::string& inputArgs);                              //used by iterator calling (i.e. FESupervisor)
	void        startMacroMultiDimensional(const std::string& requester, const std::string& interfaceID, const std::string& macroName, const std::string& macroString, const bool enableSavingOutput, const std::string& outputFilePath, const std::string& outputFileRadix, const std::string& inputArgs);  //used by iterator calling (i.e. FESupervisor)
	bool        checkMacroMultiDimensional(const std::string& interfaceID, const std::string& macroName);                                                                                                                                                                                                    //used by iterator calling (i.e. FESupervisor)

	unsigned int        getInterfaceUniversalAddressSize(const std::string& interfaceID);  //used by MacroMaker
	unsigned int        getInterfaceUniversalDataSize(const std::string& interfaceID);     //used by MacroMaker
	bool                allFEWorkloopsAreDone(void);                                       //used by Iterator, e.g.
	const FEVInterface& getFEInterface(const std::string& interfaceID) const;

	const std::map<std::string /*name*/, std::unique_ptr<FEVInterface> >& getFEInterfaces(void) const { return theFEInterfaces_; }
	FEVInterface*                                                         getFEInterfaceP(const std::string& interfaceID);

	//FE communication helpers
	std::mutex frontEndCommunicationReceiveMutex_;
	std::map<std::string /*targetInterfaceID*/,  //map of target to buffers organized by source
	         std::map<std::string /*requester*/,
	                  std::queue<std::string /*value*/> > >
	    frontEndCommunicationReceiveBuffer_;

	//multi-dimensional FE Macro helpers
	std::mutex macroMultiDimensionalDoneMutex_;
	std::map<std::string /*targetInterfaceID*/,  //set of active multi-dimensional Macro launches
	         std::string /*status := Active, Done, Error: <message> */>
	    macroMultiDimensionalStatusMap_;

  private:
	std::map<std::string /*name*/, std::unique_ptr<FEVInterface> > theFEInterfaces_;
	std::vector<std::string /*name*/>                              theFENamesByPriority_;

	//for managing transition iterations
	std::map<std::string /*name*/, bool /*isDone*/> stateMachinesIterationDone_;
	unsigned int                                    stateMachinesIterationWorkCount_;
	unsigned int                                    subIterationWorkStateMachineIndex_;
	void                                            preStateMachineExecution(unsigned int i);
	bool                                            postStateMachineExecution(unsigned int i);
	void                                            preStateMachineExecutionLoop(void);
	void                                            postStateMachineExecutionLoop(void);
};

}  // namespace ots

#endif
