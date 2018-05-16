#ifndef _ots_FEVInterfacesManager_h_
#define _ots_FEVInterfacesManager_h_

//#include "artdaq/Application/MPI2/MPISentry.hh"
//#include "artdaq/DAQrate/quiet_mpi.hh"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/Configurable/Configurable.h"

#include <map>
#include <string>
#include <memory>

namespace ots
{
class FEVInterface;

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



    int					universalRead	 					(const std::string &interfaceID, char* address, char* returnValue); //used by MacroMaker
    void 			    universalWrite	  					(const std::string &interfaceID, char* address, char* writeValue); //used by MacroMaker
    std::string  	    getFEListString						(const std::string &supervisorLid);	//used by MacroMaker
    std::string  	    getFEMacrosString					(const std::string &supervisorLid);	//used by MacroMaker
    void		  	    runFEMacro							(const std::string &interfaceID, const std::string &feMacroName, const std::string &inputArgs, std::string &outputArgs);	//used by MacroMaker
    unsigned int		getInterfaceUniversalAddressSize	(const std::string &interfaceID); 	//used by MacroMaker
    unsigned int		getInterfaceUniversalDataSize		(const std::string &interfaceID);	//used by MacroMaker
    bool				allFEWorkloopsAreDone				(void); //used by Iterator, e.g.

    //void progDAC           (std::string dacName, std::string rocName="*");
    //void write             (long long address, const std::vector<long long>& data);
    //void read              (long long address,       std::vector<long long>& data);
//    void initHardware      (void);



protected:
//    virtual std::unique_ptr<FEVInterface> interpretInterface (
//    		const unsigned int interfaceId,
//    		const std::string typeName, const std::string firmwareName,
//    		const FEInterfaceConfigurationBase* frontEndHardwareConfiguration);

private:


    std::map<std::string /*name*/, std::unique_ptr<FEVInterface> > theFEInterfaces_;

};

}

#endif
