/*
 * RegisterView.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#include "otsdaq-core/ConfigurationDataFormats/RegisterView.h"

using namespace ots;

RegisterView::RegisterView(std::string name)
    : name_(name), version_(-1), comment_(""), author_(""), creationTime_(time(0))
{
}
//==============================================================================
RegisterView::~RegisterView()
{
	// TODO Auto-generated destructor stub
}
//==============================================================================
unsigned int RegisterView::findRow(unsigned int col, const std::string value) const
{
	for (unsigned int row = 0; row < theDataView_.size(); row++)
	{
		if (theDataView_[row][col] == value)
			return row;
	}
	//std::cout << __COUT_HDR_FL__ << "\tIn view: " << name_ << " I Can't find " << value << " in column " << col << " with name: " << registersInfo_[col].getName() << std::endl;
	assert(0);
	return 0;
}

//==============================================================================
unsigned int RegisterView::findRow(unsigned int col, const unsigned int value) const
{
	std::stringstream s;
	s << value;
	return findRow(col, s.str());
}
//Getters
//==============================================================================
std::string RegisterView::getName(void) const
{
	return name_;
}

//==============================================================================
int RegisterView::getVersion(void) const
{
	return version_;
}

//==============================================================================
std::string RegisterView::getComment(void) const
{
	return comment_;
}

//==============================================================================
std::string RegisterView::getAuthor(void) const
{
	return author_;
}
//==============================================================================
time_t RegisterView::getCreationTime(void) const
{
	return creationTime_;
}
//==============================================================================
const std::vector<ViewRegisterInfo>& RegisterView::getRegistersInfo(void) const
{
	return registersInfo_;
}
//==============================================================================
std::vector<ViewRegisterInfo>* RegisterView::getRegistersInfoPointer(void)
{
	return &registersInfo_;
}
//==============================================================================
const std::vector<ViewRegisterSequencerInfo>& RegisterView::getRegistersSequencerInfo(void) const
{
	return registersSequencerInfo_;
}
//==============================================================================
std::vector<ViewRegisterSequencerInfo>* RegisterView::getRegistersSequencerInfoPointer(void)
{
	return &registersSequencerInfo_;
}
//==============================================================================
unsigned int RegisterView::getNumberOfRows(void) const
{
	return theDataView_.size();
}

//==============================================================================
unsigned int RegisterView::getNumberOfColumns(void) const
{
	//return registersInfo_.getNumberOfColumns();
	return 1;
}
//Setters
//==============================================================================
void RegisterView::setName(std::string name)
{
	name_ = name;
}

//==============================================================================
void RegisterView::setVersion(int version)
{
	version_ = version;
}

//==============================================================================
void RegisterView::setVersion(char* version)
{
	version_ = atoi(version);
}

//==============================================================================
void RegisterView::setComment(std::string comment)
{
	comment_ = comment;
}

//==============================================================================
void RegisterView::setAuthor(std::string author)
{
	author_ = author;
}
//==============================================================================
void RegisterView::setCreationTime(time_t t)
{
	creationTime_ = t;
}
//==============================================================================
/*void RegisterView::reset (void)
{
    version_ = -1;
    comment_ = "";
    author_  + "";
    columnsInfo_.clear();
    theDataView_.clear();
    }*/
//==============================================================================
//returns index of added row, always is last row
//return -1 on failure
int RegisterView::addRow(void)
{
	int row = getNumberOfRows();
	theDataView_.resize(getNumberOfRows() + 1, std::vector<std::string>(getNumberOfColumns()));

	//fill each col of new row with default values
	for (unsigned int col = 0; col < getNumberOfColumns(); ++col) {
		theDataView_[row][col] = "DEFAULT";  //GET COLUMN NAMES
	}

	return row;
}

//==============================================================================
//returns true on success
//return false on failure
bool RegisterView::deleteRow(int r)
{
	if (r >= (int)getNumberOfRows()) return false;  //out of bounds

	theDataView_.erase(theDataView_.begin() + r);
	return true;
}
