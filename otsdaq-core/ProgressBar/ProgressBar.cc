#include "otsdaq-core/ProgressBar/ProgressBar.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <dirent.h>  //for DIR
#include <sys/stat.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace ots;

//========================================================================================================================
ProgressBar::ProgressBar()
    : cProgressBarFilePath_(std::string(getenv("SERVICE_DATA_PATH")) +
                            "/ProgressBarData/")
    , cProgressBarFileExtension_(".txt")
    , totalStepsFileName_("")
    , stepCount_(0)
    , stepsToComplete_(0)
    , started_(false)
{
	std::string path = cProgressBarFilePath_;
	DIR*        dir  = opendir(path.c_str());
	if(dir)
		closedir(dir);
	else if(-1 == mkdir(path.c_str(), 0755))
	{
		// lets create the service folder (for first time)
		std::cout << __COUT_HDR_FL__ << "Service directory creation failed: " << path
		          << std::endl;
		assert(false);
	}
}

//========================================================================================================================
//		reset() ~~
//		Resets progress bar to 0% complete
void ProgressBar::reset(std::string file, std::string lineNumber, int id)
{
	stepCount_       = 0;
	stepsToComplete_ = 0;

	// try to load stepsToComplete based on file, lineNumber and id
	char fn[1000];
	sprintf(fn, "%s_%s_%d", file.c_str(), lineNumber.c_str(), id);

	for(unsigned int c = 0; c < strlen(fn); ++c)
		if(!((fn[c] >= '0' && fn[c] <= '9') || (fn[c] >= 'a' && fn[c] <= 'z') ||
		     (fn[c] >= 'A' && fn[c] <= 'Z')))
			fn[c] = '_';
	totalStepsFileName_ = cProgressBarFilePath_ + fn + cProgressBarFileExtension_;
	//	std::cout << __COUT_HDR_FL__ << totalStepsFileName_ << std::endl;

	FILE* fp = fopen(totalStepsFileName_.c_str(), "r");
	if(fp)
	{
		fscanf(fp, "%d", &stepsToComplete_);
		fclose(fp);
		//		std::cout << __COUT_HDR_FL__ << "File Found - stepsToComplete = " <<
		// stepsToComplete_ << std::endl;
	}
	else
		std::cout << __COUT_HDR_FL__ << "File Not there" << std::endl;

	started_ = true;
}

//========================================================================================================================
void ProgressBar::step()
{
	std::lock_guard<std::mutex> lock(theMutex_);  // lock out for remainder of scope
	++stepCount_;
	// std::cout << __COUT_HDR_FL__  << totalStepsFileName_ << " " <<
	// readPercentageString() << "% complete" << std::endl;
}

//========================================================================================================================
void ProgressBar::complete()
{
	step();  // consider complete as a step
	stepsToComplete_ = stepCount_;
	started_         = false;

	// done, save steps to file

	//	std::cout << __COUT_HDR_FL__ << totalStepsFileName_ << std::endl;

	FILE* fp = fopen(totalStepsFileName_.c_str(), "w");
	if(fp)
	{
		fprintf(fp, "%d", stepsToComplete_);
		fclose(fp);
	}
	else
		std::cout << __COUT_HDR_FL__ << "Critical ERROR!" << std::endl;
}

//========================================================================================================================
// return percentage complete as integer
int ProgressBar::read()
{
	if(!started_)
		return 100;  // if no progress started, always return 100% complete

	std::lock_guard<std::mutex> lock(theMutex_);  // lock out for remainder of scope
	if(stepsToComplete_)
		return stepCount_ * 100.0 / stepsToComplete_;

	return stepCount_ ? 50 : 0;
}

//========================================================================================================================
// return percentage complete as std::string
std::string ProgressBar::readPercentageString()
{
	char pct[5];
	sprintf(pct, "%d", read());
	return pct;
}
