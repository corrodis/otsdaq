#include "otsdaq-core/ConfigurationPluginDataFormats/Configurations.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
Configurations::Configurations(void)
    : ConfigurationBase("Configurations")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//ConfigurationsInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="Configurations">
	//    <VIEW Name="CONFIGURATIONS" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="ConfigurationGroupKey" StorageName="CONFIGURATION_KEY" DataType="NUMBER"  />
	//      <COLUMN Name="KOC"	            StorageName="KOC" 	            DataType="VARCHAR2"/>
	//      <COLUMN Name="ConditionVersion" StorageName="CONDITION_VERSION" DataType="NUMBER"  />
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
Configurations::~Configurations(void)
{
}

//==============================================================================
void Configurations::init(ConfigurationManager *configManager)
{
	/*
        std::string       keyName;
        unsigned int keyValue;
        theKeys_.clear();
        for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
        {
            ConfigurationBase::activeConfigurationView_->getValue(keyName,row,ConfigurationAlias);
            ConfigurationBase::activeConfigurationView_->getValue(keyValue,row,ConfigurationGroupKeyId);
            theKeys_[keyName] = keyValue;
        }
	 */
}

//==============================================================================
bool Configurations::findKOC(ConfigurationGroupKey ConfigurationGroupKey, std::string koc) const
{
	unsigned int tmpConfigurationGroupKey;  //this is type to extract from table
	std::string  tmpKOC;
	for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpConfigurationGroupKey, row, ConfigurationGroupKeyAlias);
		if (ConfigurationGroupKey == tmpConfigurationGroupKey)
		{
			ConfigurationBase::activeConfigurationView_->getValue(tmpKOC, row, KOC);
			//std::cout << __COUT_HDR_FL__ << "Looking for KOC: " << tmpKOC << " for " << koc << std::endl;
			if (tmpKOC == koc)
				return true;
		}
	}
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "ERROR: Can't find KOC " << koc << std::endl;
	return false;
}

//==============================================================================
// getConditionVersion
//FIXME -- new ConfiguratoinGroup and ConfigurationGroupKey should be used!
ConfigurationVersion Configurations::getConditionVersion(const ConfigurationGroupKey &ConfigurationGroupKey,
                                                         std::string                  koc) const
{
	unsigned int tmpConfigurationGroupKey;  //this is type to extract from table
	std::string  tmpKOC;
	unsigned int conditionVersion;  //this is type to extract from table

	for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpConfigurationGroupKey, row, ConfigurationGroupKeyAlias);
		if (ConfigurationGroupKey == tmpConfigurationGroupKey)
		{
			ConfigurationBase::activeConfigurationView_->getValue(tmpKOC, row, KOC);
			if (tmpKOC == koc)
			{
				ConfigurationBase::activeConfigurationView_->getValue(conditionVersion, row, ConditionVersion);
				//std::cout << __COUT_HDR_FL__ << "\tConditionVersion " << ConditionVersion << std::endl;
				return ConfigurationVersion(conditionVersion);
			}
		}
	}
	std::cout << __COUT_HDR_FL__ << "****************************************************************************************************************************" << std::endl;
	std::cout << __COUT_HDR_FL__ << "\tCan't find KOC " << koc << " with ConfigurationGroupKey " << ConfigurationGroupKey << " in the Configurations view" << std::endl;
	std::cout << __COUT_HDR_FL__ << "****************************************************************************************************************************" << std::endl;
	__THROW__("Could not find koc for ConfigurationGroupKey");
	return ConfigurationVersion();  //return INVALID
}

//==============================================================================
// setConditionVersion
// returns 1 if no change occurred (because new version was same as existing)
// returns 0 on change success
int Configurations::setConditionVersionForView(ConfigurationView *   cfgView,
                                               ConfigurationGroupKey ConfigurationGroupKey,
                                               std::string           koc,
                                               ConfigurationVersion  newKOCVersion)
{
	//find first match of KOCAlias and ConfigurationGroupKey
	unsigned int row = 0;
	unsigned int tmpConfigurationGroupKey;  //this is type to extract from table
	std::string  tmpKOC;
	unsigned int tmpOldKOCVersion;  //this is type to extract from table
	for (row = 0; row < cfgView->getNumberOfRows(); row++)
	{
		cfgView->getValue(tmpConfigurationGroupKey, row, Configurations::ConfigurationGroupKeyAlias);
		if (ConfigurationGroupKey == tmpConfigurationGroupKey)
		{
			cfgView->getValue(tmpKOC, row, Configurations::KOC);
			if (tmpKOC == koc)
			{
				cfgView->getValue(tmpOldKOCVersion, row, Configurations::ConditionVersion);
				std::cout << __COUT_HDR_FL__ << "Found ConfigKey(" << ConfigurationGroupKey << ") and KOCAlias(" << koc << ") pair."
				          << " Current version is: " << tmpOldKOCVersion << " New version is: " << newKOCVersion << std::endl;
				if (newKOCVersion == tmpOldKOCVersion)
					return 1;  //no change necessary
				break;         //found row! exit search loop
			}
		}
	}

	//at this point have row to change

	std::cout << __COUT_HDR_FL__ << "Row found is:" << row << std::endl;
	cfgView->setValue(newKOCVersion, row, Configurations::ConditionVersion);
	std::cout << __COUT_HDR_FL__ << "Version changed to:" << newKOCVersion << std::endl;
	return 0;
}

//==============================================================================
//getListOfKocs
//	return list of Kind of Conditions that match ConfigurationGroupKey for the active configuration view.
//	for all KOCs regardless of ConfigurationGroupKey, set ConfigurationGroupKey = -1
//
std::set<std::string> Configurations::getListOfKocs(ConfigurationGroupKey ConfigurationGroupKey) const
{
	std::set<std::string> kocs;
	getListOfKocsForView(ConfigurationBase::activeConfigurationView_, kocs, ConfigurationGroupKey);
	return kocs;
}

//==============================================================================
//getListOfKocsForView
//	return list of Kind of Conditions that match ConfigurationGroupKey for the active configuration view.
//	for all KOCs regardless of ConfigurationGroupKey, set ConfigurationGroupKey = -1
//
void Configurations::getListOfKocsForView(ConfigurationView *cfgView, std::set<std::string> &kocs, ConfigurationGroupKey ConfigurationGroupKey) const
{
	if (!cfgView)
	{
		std::cout << __COUT_HDR_FL__ << "****************************************************************************************************************************" << std::endl;
		std::cout << __COUT_HDR_FL__ << "\tCan't find list of Kocs for null cfgView pointer" << std::endl;
		std::cout << __COUT_HDR_FL__ << "****************************************************************************************************************************" << std::endl;
		__THROW__("Null cfgView configuration view pointer");
	}

	std::string  tmpKOC;
	unsigned int tmpConfigurationGroupKey;  //this is type to extract from table

	for (unsigned int row = 0; row < cfgView->getNumberOfRows(); row++)
	{
		cfgView->getValue(tmpConfigurationGroupKey, row, ConfigurationGroupKeyAlias);
		if (ConfigurationGroupKey == ConfigurationGroupKey::INVALID ||
		    ConfigurationGroupKey == tmpConfigurationGroupKey)
		{
			cfgView->getValue(tmpKOC, row, KOC);
			kocs.insert(tmpKOC);
		}
	}
}

DEFINE_OTS_CONFIGURATION(Configurations)
