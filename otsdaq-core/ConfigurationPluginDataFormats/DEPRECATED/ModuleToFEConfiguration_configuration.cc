#include "otsdaq-core/ConfigurationPluginDataFormats/ModuleToFEConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
ModuleToFEConfiguration::ModuleToFEConfiguration(void)
: ConfigurationBase("ModuleToFEConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//ModuleToFEConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="ModuleToFEConfiguration">
	//    <VIEW Name="MODULE_TO_FE_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="ModuleName"   StorageName="MODULE_NAME"   DataType="VARCHAR2"  />
	//      <COLUMN Name="ModuleType"   StorageName="MODULE_TYPE"   DataType="VARCHAR2"  />
	//      <COLUMN Name="FEWName"      StorageName="FEW_NAME"      DataType="NUMBER"    />
	//      <COLUMN Name="FEWType"      StorageName="FEW_TYPE"      DataType="VARCHAR2"  />
	//      <COLUMN Name="FERName"      StorageName="FER_NAME"      DataType="NUMBER"    />
	//      <COLUMN Name="FERType"      StorageName="FER_TYPE"      DataType="VARCHAR2"  />
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>

}

//==============================================================================
ModuleToFEConfiguration::~ModuleToFEConfiguration(void)
{
}

//==============================================================================
void ModuleToFEConfiguration::init(ConfigurationManager *configManager)
{
	/*
std::string       enumValue1;
    unsigned int enumValue2;
    for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
    {
        ConfigurationBase::activeConfigurationView_->getValue(enumValue1,row,Enum1);
        ConfigurationBase::activeConfigurationView_->getValue(enumValue2,row,Enum2);
    }
	 */
}

//==============================================================================
std::list<std::string> ModuleToFEConfiguration::getFEWModulesList(unsigned int FEWNumber) const
{
	std::string       moduleName;
	unsigned int tmpFEW;
	std::list<std::string> list;
	for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpFEW,row,FEWName);
		if(tmpFEW == FEWNumber)
		{
			ConfigurationBase::activeConfigurationView_->getValue(moduleName,row,ModuleName);
			list.push_back(moduleName);
		}
	}
	return list;
}

//==============================================================================
std::list<std::string> ModuleToFEConfiguration::getFERModulesList(unsigned int FERNumber) const
{
	std::string       moduleName;
	unsigned int tmpFER;
	std::list<std::string> list;
	for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpFER,row,FERName);
		if(tmpFER == FERNumber)
		{
			ConfigurationBase::activeConfigurationView_->getValue(moduleName,row,ModuleName);
			list.push_back(moduleName);
		}
	}
	return list;
}

DEFINE_OTS_CONFIGURATION(ModuleToFEConfiguration)
