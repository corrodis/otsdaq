#ifndef _ots_ControlsVInterface_h_
#define _ots_ControlsVInterface_h_
#include "otsdaq-core/Configurable/Configurable.h"


#include <string>
#include <array>
namespace ots
{
  
  class ControlsVInterface : public Configurable
  {
    
  public:
    ControlsVInterface(const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath)
    : Configurable                  (theXDAQContextConfigTree, configurationPath)
    //, interfaceUID_                 (interfaceUID)
   // , interfaceType_                ("MADEUP_FIXME")//theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("ControlsInterfacePluginName").getValue<std::string>())
    {
    	std::cout << __PRETTY_FUNCTION__ << std::endl;

    }

    virtual ~ControlsVInterface (void){} 

    
    virtual void initialize        	(                       ) = 0;
    
    
    virtual void 	subscribe      	(std::string Name       ) = 0;
    virtual void 	subscribeJSON  	(std::string List       ) = 0;
    virtual void 	unsubscribe    	(std::string Name       ) = 0;
    virtual std::string getList 	(std::string format     ) = 0;
    virtual std::array<std::string, 4>  getCurrentValue (std::string Name) = 0;
    virtual std::array<std::string, 9>  getSettings     (std::string Name) = 0;   

protected:       
    std::string           interfaceUID_; 
    std::string           interfaceType_;                                                                                                  

  };
  
}
#endif
