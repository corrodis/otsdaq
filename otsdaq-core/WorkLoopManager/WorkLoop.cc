#include "otsdaq-core/WorkLoopManager/WorkLoop.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <toolbox/task/WorkLoopFactory.h>

#include <iostream>
#include <unistd.h>

using namespace ots;


//========================================================================================================================
WorkLoop::WorkLoop(std::string name)
: continueWorkLoop_(false)
, cWorkLoopName_   (name)
, cWorkLoopType_   ("waiting")
, workLoop_        (0)
, job_             (toolbox::task::bind(this, &WorkLoop::workLoopThread, cWorkLoopName_))
{
}

//========================================================================================================================
WorkLoop::~WorkLoop(void)
{
  mf::LogDebug("WorkLoop") << "Destroying WorkLoop: " << cWorkLoopName_ << std::endl;
	if(stopWorkLoop())
		toolbox::task::getWorkLoopFactory()->removeWorkLoop(cWorkLoopName_, cWorkLoopType_);
	mf::LogDebug("WorkLoop") << "Destroyed WorkLoop: " << cWorkLoopName_ << std::endl;
}

//========================================================================================================================
void WorkLoop::startWorkLoop(void)
{
	//__COUT__ << "Starting WorkLoop: " << cWorkLoopName_ << std::endl;
	continueWorkLoop_ = true;
	try
	{
		workLoop_ = toolbox::task::getWorkLoopFactory()->getWorkLoop(cWorkLoopName_, cWorkLoopType_);
	}
	catch (xcept::Exception & e)
	{
		__COUT__ << "ERROR: Can't create WorkLoop job for " << cWorkLoopName_ << std::endl;
		stopWorkLoop();
	}

	if(workLoop_->isActive()) return;//This might be a consumer producer running at the same time so it has been activated already

	try
	{
		workLoop_->submit(job_);
	}
	catch (xcept::Exception & e)
	{
		__COUT__ << "ERROR: Can't submit WorkLoop job for " << cWorkLoopName_ << std::endl;
		stopWorkLoop();
	}

	try
	{
		workLoop_->activate();
	}
	catch (xcept::Exception & e)
	{
		__COUT__ << "ERROR: Can't activate WorkLoop job for " << cWorkLoopName_
				<< " Very likely because the name " << cWorkLoopName_  << " is not unique!" << std::endl;
		stopWorkLoop();
	}
}

//========================================================================================================================
bool WorkLoop::stopWorkLoop()
{
	__COUT__ << "Stopping WorkLoop: " << cWorkLoopName_ << std::endl;

	continueWorkLoop_ = false;
	if(workLoop_ == 0)
	{
		__COUT__ << "MEASSAGE: WorkLoop " << cWorkLoopName_ << " was not created at all! This message will be commented in the future" << std::endl;
		return false;
	}

	__COUT__ << "initially workLoop_->isActive() " << (workLoop_->isActive()?"yes":"no") << std::endl;

	try
	{
		//THis method waits until the workloop job returns! Super cool!
		workLoop_->cancel();
	}
	catch (xcept::Exception & e)
	{
		__COUT__ << "WARNING: Can't cancel WorkLoop job for " << cWorkLoopName_ <<
				" because probably it has never been activated!" << std::endl;

		__COUT__ << "workLoop_->isActive() " << (workLoop_->isActive()?"yes":"no") << std::endl;
		return true;
	}

	try
	{
		workLoop_->remove(job_);
	}
	catch (xcept::Exception & e)
	{
		// ATTENTION!
		// If the workloop job thread returns false, then the workloop job is automatically removed and it can't be removed again
		// Leaving this try catch allows me to be general in the job threads so I can return true (repeat loop) or false ( loop only once)
		// without crashing
		__COUT__ << "WARNING: Can't remove request WorkLoop: " << cWorkLoopName_ << std::endl;
		__COUT__ << "workLoop_->isActive() " << (workLoop_->isActive()?"yes":"no") << std::endl;
	}

	__COUT__ << "Stopped WorkLoop: " << cWorkLoopName_ << std::endl;
	__COUT__ << "workLoop_->isActive() " << (workLoop_->isActive()?"yes":"no") << std::endl;
	return true;
}

//========================================================================================================================
const std::string& WorkLoop::getWorkLoopName(void)
{
	return cWorkLoopName_;
}

//========================================================================================================================
bool WorkLoop::isActive(void)
{
	return workLoop_->isActive() && continueWorkLoop_;
}
