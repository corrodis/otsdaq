#ifndef _ots_ConfigurationBase_h_
#define _ots_ConfigurationBase_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationView.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationVersion.h"
#include <string>
#include <map>
#include <list>

namespace ots
{

class ConfigurationManager;
	
//e.g. configManager->__SELF_NODE__;  //to get node referring to this configuration
#define __SELF_NODE__ getNode(getConfigurationName())

class ConfigurationBase
{

public:

    const unsigned int MAX_VIEWS_IN_CACHE; //Each inheriting configuration class could have varying amounts of cache

    ConfigurationBase														();
    ConfigurationBase														(std::string configurationName, std::string *accumulatedExceptions=0);

    virtual ~ConfigurationBase												(void);

    //Methods
    virtual void 			 			init								(ConfigurationManager *configManager);

    void         			 			destroy								(void) {;}
    void         			 			reset  								(bool keepTemporaryVersions=false);
    void								deactivate							(void);
    bool								isActive							(void);

    void         			 			print								(std::ostream &out = std::cout) const; //always prints active view

    std::string				 			getTypeId							(void);

    void 					 			setupMockupView  					(ConfigurationVersion version);
    void 					 			changeVersionAndActivateView		(ConfigurationVersion temporaryVersion, ConfigurationVersion version);
    bool 					 			isStored        					(const ConfigurationVersion &version) const;
    bool 					 			eraseView       					(ConfigurationVersion version);
    void 					 			trimCache  							(unsigned int trimSize = -1);
    void 					 			trimTemporary						(ConfigurationVersion targetVersion = ConfigurationVersion());
    ConfigurationVersion				checkForDuplicate					(ConfigurationVersion needleVersion, ConfigurationVersion ignoreVersion = ConfigurationVersion()) const;

    //Getters
    const std::string&       			getConfigurationName   				(void) const;
    const std::string&       			getConfigurationDescription 		(void) const;
    std::set<ConfigurationVersion>  	getStoredVersions      				(void) const;
    
    const ConfigurationView& 			getView         					(void) const;
          ConfigurationView* 			getViewP        					(void);
          ConfigurationView* 			getMockupViewP  					(void);
    const ConfigurationVersion& 		getViewVersion      				(void) const; //always the active one

    ConfigurationView* 		 			getTemporaryView					(ConfigurationVersion temporaryVersion);
    ConfigurationVersion	 			getNextTemporaryVersion				() const;
    ConfigurationVersion	 			getNextVersion						() const;

    //Setters
    void 					 			setConfigurationName				(const std::string &configurationName);
    void 					 			setConfigurationDescription			(const std::string &configurationDescription);
    bool 					 			setActiveView       				(ConfigurationVersion version);
    ConfigurationVersion				copyView  							(const ConfigurationView &sourceView, ConfigurationVersion destinationVersion, const std::string &author);
    ConfigurationVersion				mergeViews 							(const ConfigurationView &sourceViewA, const ConfigurationView &sourceViewB, ConfigurationVersion destinationVersion, const std::string &author, const std::string &mergeApproach /*rename,replace,skip*/, std::map<std::string /*original uidB*/, std::string /*converted uidB*/>& uidConversionMap, bool doNotMakeDestinationVersion);

    ConfigurationVersion	 			createTemporaryView					(ConfigurationVersion sourceViewVersion = ConfigurationVersion(), ConfigurationVersion destTemporaryViewVersion = ConfigurationVersion::getNextTemporaryVersion()); //source of -1, from MockUp, else from valid view version

    static std::string					convertToCaps						(std::string &str, bool isConfigName=false);

    bool 								latestAndMockupColumnNumberMismatch	(void) const;

    unsigned int             			getNumberOfStoredViews 	(void) const;

protected:
    std::string 			configurationName_;
    std::string 			configurationDescription_;

    ConfigurationView* 		activeConfigurationView_;
    ConfigurationView  		mockupConfigurationView_;

    //Version and data associated to make it work like a cache.
    //It will be very likely just 1 version
    //NOTE: must be very careful to setVersion of view after manipulating (e.g. copy from different version view)
    std::map<ConfigurationVersion, ConfigurationView> configurationViews_;

};
}

#endif
