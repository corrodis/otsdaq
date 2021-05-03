#include "otsdaq/RootUtilities/DQMHistosBase.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/RootUtilities/VisualDataManager.h"

#include <TDirectory.h>
#include <TFile.h>
#include <TStyle.h>

#include <iostream>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "DQMHistos"
#define mfSubject_ (std::string("DQMHistos"))

//==============================================================================
DQMHistosBase::DQMHistosBase(void) : theFile_(nullptr), myDirectory_(nullptr) { gStyle->SetPalette(1); }

//==============================================================================
DQMHistosBase::~DQMHistosBase(void) { closeFile(); }

//==============================================================================
bool DQMHistosBase::isFileOpen(void)
{
	if(theFile_ == nullptr || !theFile_->IsOpen())
		return false;
	return true;
}

//==============================================================================
void DQMHistosBase::openFile(std::string fileName)
{
	closeFile();
	myDirectory_ = nullptr;
	theFile_     = theDataManager_->openFile(fileName);
	
	theFile_->cd();
	myDirectory_ = theFile_;
	
}

//==============================================================================
void DQMHistosBase::save(void)
{
	if(theFile_ != nullptr)
		theFile_->Write();
}

//==============================================================================
void DQMHistosBase::closeFile(void)
{
	if(theFile_ != nullptr)
	{
		if(theFile_->IsOpen())
			theFile_->Close();
		theFile_ = nullptr;
	}
}

//==============================================================================
TObject* DQMHistosBase::get(std::string name)
{
	if(theFile_ != nullptr)
		return theFile_->Get(name.c_str());
	return 0;
}
