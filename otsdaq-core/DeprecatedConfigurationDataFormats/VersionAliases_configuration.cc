#include "otsdaq-core/ConfigurationPluginDataFormats/VersionAliases.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
VersionAliases::VersionAliases (void)
    : ConfigurationBase ("VersionAliases")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//VersionAlaisesInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="VersionAliases">
	//    <VIEW Name="VERSION_ALIASES" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="VersionAlias"    StorageName="VERSION_ALIAS"      DataType="VARCHAR2" />
	//      <COLUMN Name="Version"	       StorageName="VERSION"	        DataType="NUMBER"	/>
	//      <COLUMN Name="KOC"             StorageName="KOC"                DataType="VARCHAR2" />
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
VersionAliases::~VersionAliases (void)
{
}

//==============================================================================
void VersionAliases::init (ConfigurationManager *configManager)
{
	/*
    std::string       keyName;
    unsigned int keyValue;
    theKeys_.clear();
    for(unsigned int row=0; row<ConfigurationBase::configurationData_.getNumberOfRows(); row++)
    {
        ConfigurationBase::configurationData_.getValue(keyName,row,ConfigurationAlias);
        ConfigurationBase::configurationData_.getValue(keyValue,row,ConfigurationGroupKeyId);
        theKeys_[keyName] = keyValue;
    }
	 */
}

DEFINE_OTS_CONFIGURATION (VersionAliases)
