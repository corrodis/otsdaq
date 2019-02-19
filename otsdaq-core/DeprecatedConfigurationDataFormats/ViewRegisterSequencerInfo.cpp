/*
 * ViewRegisterSequencerInfo.cpp
 *
 *  Created on: Aug 3, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/ViewRegisterSequencerInfo.h"

using namespace ots;

ViewRegisterSequencerInfo::ViewRegisterSequencerInfo(std::string componentName,
                                                     std::string registerName,
                                                     int         registerValue,
                                                     int         sequenceNumber,
                                                     std::string state)
    : componentName_(componentName)
    , registerName_(registerName)
    , state_(state)
    , valueSequencePair_(std::make_pair(sequenceNumber, registerValue))
//	initializeState_(initializeState),
//	configureState_	(configureState_),
//	startState_		(startState),
//	haltState_		(haltState),
//	pauseState_		(pauseState),
//	resumeState_	(resumeState)

{
	// TODO Auto-generated constructor stub
}

ViewRegisterSequencerInfo::~ViewRegisterSequencerInfo()
{
	// TODO Auto-generated destructor stub
}
//==============================================================================
void ViewRegisterSequencerInfo::setState(std::string         state,
                                         std::pair<int, int> valueSequencePair)
{
	state_ = state;
	if(state == "INITIALIZE")
	{
		initializeState_ = valueSequencePair;
	}
	else if(state == "CONFIGURATION")
	{
		configurationState_ = valueSequencePair;
	}
	else if(state == "START")
	{
		startState_ = valueSequencePair;
	}
	else if(state == "HALT")
	{
		haltState_ = valueSequencePair;
	}
	else if(state == "PAUSE")
	{
		pauseState_ = valueSequencePair;
	}
	else if(state == "RESUME")
	{
		resumeState_ = valueSequencePair;
	}
}
//==============================================================================
const std::string& ViewRegisterSequencerInfo::getComponentName(void) const
{
	return componentName_;
}
//==============================================================================
const std::string& ViewRegisterSequencerInfo::getRegisterName(void) const
{
	return registerName_;
}
//==============================================================================
const std::string& ViewRegisterSequencerInfo::getState(void) const { return state_; }
//==============================================================================
const std::pair<int, int>& ViewRegisterSequencerInfo::getValueSequencePair(void) const
{
	if(state_ == "INITIALIZE")
	{
		return initializeState_;
	}
	else if(state_ == "CONFIGURATION")
	{
		return configurationState_;
	}
	else if(state_ == "START")
	{
		return startState_;
	}
	else if(state_ == "HALT")
	{
		return haltState_;
	}
	else if(state_ == "PAUSE")
	{
		return pauseState_;
	}
	else if(state_ == "RESUME")
	{
		return resumeState_;
	}
	return valueSequencePair_;
}
