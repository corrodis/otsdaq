/*
 * RegisterConfiguration.cpp
 *
 *  Created on: Jul 24, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/RegisterConfiguration.h"

using namespace ots;

RegisterConfiguration::RegisterConfiguration(std::string staticConfigurationName)
    : ConfigurationBase(staticConfigurationName)
{
	////////////////////////////////////////////////////////////////////////
	////WARNING: the names and the order MUST match the ones in the enum  //
	////////////////////////////////////////////////////////////////////////
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="***RegisterConfiguration">
	//	<VIEW Name="DATA" Type="File,Database,DatabaseTest">
	//	  <COLUMN Name="ComponentType"        StorageName="DETECTOR_TYPE"          DataType="VARCHAR2"		/>
	//	  <COLUMN Name="RegisterName"         StorageName="REGISTER_NAME"          	DataType="VARCHAR2" 	/>
	//	  <COLUMN Name="RegisterBaseAddress"  StorageName="REGISTER_BASE_ADDRESS"   DataType="NUMBER"		/>
	//	  <COLUMN Name="RegisterSize"         StorageName="REGISTER_SIZE"        	DataType="NUMBER"		/>
	//	  <COLUMN Name="RegisterAccess" 	  StorageName="REGISTER_ACCESS" 		DataType="VARCHAR2"  	/>
	//	</VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

RegisterConfiguration::~RegisterConfiguration()
{
	// TODO Auto-generated destructor stub
}

void RegisterConfiguration::init() {}
