#include "otsdaq-core/ConfigurationPluginDataFormats/TemplateConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include <iostream>
#include <string>

using namespace ots;

//==============================================================================
TemplateConfiguration::TemplateConfiguration(void)
: ConfigurationBase("TemplateConfiguration")
{
	//TemplateConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="TemplateConfiguration">
	//    <VIEW Name="TEMPLATE_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="ColumnName" StorageName="COLUMN_NAME" DataType="VARCHAR2"/>
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>

}

//==============================================================================
TemplateConfiguration::~TemplateConfiguration(void)
{
}

//==============================================================================
void TemplateConfiguration::init(ConfigurationManager *configManager)
{
	//do something to validate or refactor table
	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	//	__COUT__ << configManager->getNode(this->getConfigurationName()).getValueAsString()
	//		  											  << std::endl;

	std::string value;
	auto childrenMap = configManager->__SELF_NODE__.getChildren();
	for(auto &childPair: childrenMap)
	{
		//do something for each row in table
		__COUT__ << childPair.first << std::endl;
		//		__COUT__ << childPair.second.getNode(colNames_.colColumnName_) << std::endl;
		childPair.second.getNode(colNames_.colColumnName_	).getValue(value);
	}
}

DEFINE_OTS_CONFIGURATION(TemplateConfiguration)
