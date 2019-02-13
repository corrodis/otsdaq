/*
 * RegisterConfiguration.cpp
 *
 *  Created on: Jul 24, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/RegisterSequencer.h"

using namespace ots;

RegisterSequencer::RegisterSequencer(std::string staticConfigurationName) : ConfigurationBase(staticConfigurationName) {
  ////////////////////////////////////////////////////////////////////////
  ////WARNING: the names and the order MUST match the ones in the enum  //
  ////////////////////////////////////////////////////////////////////////
  //<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
  //<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
  //  <CONFIGURATION Name="***RegisterConfiguration">
  //    <VIEW Name="DATA" Type="File,Database,DatabaseTest">
  //      <COLUMN Name="ComponentName"        StorageName="DETECTOR_ID"                DataType="VARCHAR2"
  //      /> <COLUMN Name="RegisterName"         StorageName="REGISTER_NAME"             	DataType="VARCHAR2"
  //      />
  //      <COLUMN Name="RegisterValue"		StorageName="REGISTER_VALUE"             	DataType="NUMBER"
  //      /> <COLUMN Name="SequencerNumber"      StorageName="SEQUENCE_NUMBER"         		DataType="NUMBER"
  //      />
  //      <COLUMN Name="State"          		StorageName="STATE" DataType="VARCHAR2" 	/>
  //    </VIEW>
  //  </CONFIGURATION>
  //</ROOT>
}

RegisterSequencer::~RegisterSequencer() {
  // TODO Auto-generated destructor stub
}

void RegisterSequencer::init() {}
