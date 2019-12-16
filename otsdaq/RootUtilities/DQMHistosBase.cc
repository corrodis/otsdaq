#include "otsdaq/RootUtilities/DQMHistosBase.h"
#include "otsdaq/Macros/CoutMacros.h"

#include <TDirectory.h>
#include <TFile.h>
#include <TObject.h>
#include <TStyle.h>

#include <iostream>

using namespace ots;

//========================================================================================================================
DQMHistosBase::DQMHistosBase(void) : theFile_(nullptr), myDirectory_(nullptr) { gStyle->SetPalette(1); }

//========================================================================================================================
DQMHistosBase::~DQMHistosBase(void) { closeFile(); }

//========================================================================================================================
void DQMHistosBase::openFile(std::string fileName)
{
	closeFile();
	myDirectory_ = nullptr;
	theFile_     = TFile::Open(fileName.c_str(), "RECREATE");
	theFile_->cd();
}

//========================================================================================================================
void DQMHistosBase::save(void)
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Saving file!" << std::endl;
	if(theFile_ != nullptr)
		theFile_->Write();
}

//========================================================================================================================
void DQMHistosBase::closeFile(void)
{
	if(theFile_ != nullptr)
	{
		theFile_->Close();
		theFile_ = nullptr;
	}
}

//========================================================================================================================
TObject* DQMHistosBase::get(std::string name)
{
	if(theFile_ != nullptr)
		return theFile_->Get(name.c_str());
	return 0;
}
