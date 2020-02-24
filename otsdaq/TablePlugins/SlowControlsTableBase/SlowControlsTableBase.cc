#include "otsdaq/TablePlugins/SlowControlsTableBase/SlowControlsTableBase.h"


using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "SlowControlsTableBase"


//==============================================================================
// TableBase
//	If a valid string pointer is passed in accumulatedExceptions
//	then allowIllegalColumns is set for InfoReader
//	If accumulatedExceptions pointer = 0, then illegal columns throw std::runtime_error
// exception
SlowControlsTableBase::SlowControlsTableBase(std::string tableName, std::string* accumulatedExceptions /* =0 */)
: TableBase(tableName,accumulatedExceptions)
{
}  // end constuctor()

//==============================================================================
// SlowControlsTableBase
//	Default constructor should never be used because table type is lost
SlowControlsTableBase::SlowControlsTableBase(void) 
: TableBase("SlowControlsTableBase")
{
	__SS__ << "Should not call void constructor, table type is lost!" << __E__;
	__SS_THROW__;
}  // end illegal default constructor()

//==============================================================================
SlowControlsTableBase::~SlowControlsTableBase(void) {}  // end destructor()

