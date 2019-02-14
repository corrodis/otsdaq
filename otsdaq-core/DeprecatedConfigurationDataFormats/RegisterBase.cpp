/*
 * RegisterBase.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/RegisterBase.h"
#include "otsdaq-core/ConfigurationDataFormats/RegisterConfigurationInfoReader.h"
#include "otsdaq-core/ConfigurationDataFormats/RegisterSequencerInfoReader.h"

#include <algorithm>

using namespace ots;

RegisterBase::RegisterBase (std::string configurationName, std::string componentType)
    : ConfigurationBase (configurationName)
    , componentTypeName_ (componentType)
{
	RegisterConfigurationInfoReader configurationInfoReader;
	configurationInfoReader.read (this);

	RegisterSequencerInfoReader sequencerInfoReader;
	sequencerInfoReader.read (this);

	init ();
}

RegisterBase::~RegisterBase ()
{
	// TODO Auto-generated destructor stub
}
//==============================================================================
void RegisterBase::init (void)
{
	theComponentList_.clear ();

	//Loop through each component via the sequencer and add them to theComponentList_
	for (auto it : *mockupRegisterView_.getRegistersSequencerInfoPointer ())
	{
		//	if(theComponentList_.find(it->getComponentName()) != theComponentList_.end())
		//the insert function will have no effect if it already finds the key (component name) in theComponentList_; otherwise, let's add it
		theComponentList_.insert (std::make_pair (it.getComponentName (), Component (it.getComponentName ())));
		//theComponentList_ now atleast contains the component, now lets set the state that we are looking at
		auto regs = theComponentList_.find (it.getComponentName ())->second.getRegistersPointer ();
		std::find_if (regs->begin (), regs->end (), [it](auto r) { return r.getName () == it.getRegisterName (); })->setState (it.getState (), it.getValueSequencePair ());
	}
	//Iterate through our newly-filled theComponentList_
	for (auto aComponent : theComponentList_) {
		//Iterate through each register of the component
		for (auto registerFromConfiguration : *aComponent.second.getRegistersPointer ())
		{
			//Iterate through MockupView of the xxxRegisterConfiguration.xml
			for (auto aRegister : *mockupRegisterView_.getRegistersInfoPointer ())
			{
				//If the Register names match
				if (aRegister.getRegisterName () == registerFromConfiguration.getName ()
				    //&& aRegister.getComponentName() ==  registerFromConfiguration.getComponentName()
				) {
					registerFromConfiguration.fillRegisterInfo (aRegister.getBaseAddress (), aRegister.getSize (), aRegister.getAccess ());
				}
			}
		}
	}

	//Print for Debugging
	for (auto aComponent : theComponentList_) {
		aComponent.second.printInfo ();
	}

	//create new instance of "Component" and add to "theComponentList_; set Name according to XML
	//tempComponent = new Component(FROM_XML);
	//mockupRegisterView_.getRegistersInfoPointer();
	//theComponent_ = new Component(componentNameType_);

	//Add component based on sequencer XML

	/*
		for(std::vector<ViewRegisterInfo>::iterator it = mockupRegisterView_.getRegistersInfoPointer().begin(); it != mockupRegisterView_.getRegistersInfoPointer().end(); ++it){

			if(it->getTypeName == componentNameType_)
			{
				//Check the Component's "type_name"
				//Accordingly create instances of "Register" based on Configuration XML and add to Component
				theComponent_.addRegister(it->getregisterName(), it->getBaseAddress, it->getSize(), it->getAccess);

			}
		}*/

	//Fill the Registers based on the Sequencer XML File

	/*
    std::string       tmpDetectorID;
    std::string       tmpDACName;
    unsigned int tmpDACValue;
    for(unsigned int row=0; row<activeRegisterView_->getNumberOfRows(); row++)
    {
        activeRegisterView_->getValue(tmpDetectorID,row,rocNameColumn_);
        nameToROCDACsMap_[tmpDetectorID] = ROCDACs();
        ROCDACs& aROCDACs = nameToROCDACsMap_[tmpDetectorID];
        for(unsigned int col=firstDAC_; col<=lastDAC_; col++)*/
	/*
    	theComponentList_.pushback()
        {
        	activeRegisterView_->getValue(tmpDACValue,row,col);
            tmpDACName = activeRegisterView_->getColumnInfo(col).getName();
            aROCDACs.setDAC(tmpDACName,dacNameToDACAddress_[tmpDACName],tmpDACValue);
            //std::cout << __COUT_HDR_FL__ << "DAC. Name: " << tmpDACName << " addr: " << dacNameToDACAddress_[tmpDACName] << " val: " << tmpDACValue << std::endl;
        }
    }*/
}
//==============================================================================
const RegisterView& RegisterBase::getRegisterView () const
{
	return *activeRegisterView_;
}

//==============================================================================
RegisterView* RegisterBase::getRegisterViewPointer ()
{
	return activeRegisterView_;
}

//==============================================================================
RegisterView* RegisterBase::getMockupRegisterViewPointer ()
{
	return &mockupRegisterView_;
}
//==============================================================================
std::string RegisterBase::getTypeName ()
{
	return componentTypeName_;
}
