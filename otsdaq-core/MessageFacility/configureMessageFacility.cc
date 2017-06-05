#include "otsdaq-core/MessageFacility/configureMessageFacility.hh"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/make_ParameterSet.h"
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace BFS = boost::filesystem;

void ots::configureMessageFacility(char const* progname)
{
	if(!messagefacility_initialized)
	{
		std::cout << __COUT_HDR_FL__ << "#######################################################################" << std::endl;
		std::cout << __COUT_HDR_FL__ << "Initializing MessageFacility with progname " << progname << std::endl;
		std::cout << __COUT_HDR_FL__ << "#######################################################################" << std::endl;
		std::string logPathProblem = "";
		std::string logfileName = "";
		char* logRootString = getenv("OTSDAQ_LOG_ROOT");
		char* logFhiclCode = getenv("OTSDAQ_LOG_FHICL");

		if (logRootString != nullptr)
		{
			if (! BFS::exists(logRootString))
			{
				logPathProblem = "Log file root directory ";
				logPathProblem.append(logRootString);
				logPathProblem.append(" does not exist!");
			}
			logfileName = std::string(logRootString);
		}


		std::ostringstream ss;
		ss << "debugModules:[\"*\"]  statistics:[\"stats\"] "
				<< "  destinations : { "
				//     << "    console : { "
				//     << "      type : \"cout\" threshold : \"DEBUG\" "
				//     << "      noTimeStamps : true noLineBreaks: true"
				//     << "    } "
				;

		/*if (logfileName.length() > 0)
		{
			ss << "    multifile : { "
					<< "      type : \"MultiFile\" threshold : \"DEBUG\" "
					<< "      base_directory : \"" << logfileName << "\" "
					<< "      append : true "
					<< "      use_hostname: true use_application: true "
					<< "    } ";
		}*/

		if (logFhiclCode != nullptr)
		{
			std::ifstream logfhicl( logFhiclCode );

			if ( logfhicl.is_open() )
			{
				std::stringstream fhiclstream;
				fhiclstream << logfhicl.rdbuf();
				ss << fhiclstream.str();
			} else
			{
			   //throw cet::exception("configureMessageFacility")
				std::cout << __COUT_HDR_FL__ <<
				"Unable to open requested fhicl file \"" <<
				logFhiclCode << "\".";
				return;
			}
		}

		ss << "  } ";

		std::cout << __COUT_HDR_FL__ << "Configuring MessageFacility with Parameter Set: " << ss.str() << std::endl;
		fhicl::ParameterSet pset;
		std::string pstr(ss.str());
		fhicl::make_ParameterSet(pstr, pset);

		mf::StartMessageFacility(mf::MessageFacilityService::MultiThread,
				pset);

		if (logPathProblem.size() > 0)
		{
			mf::LogError(progname) << logPathProblem;
		}
		messagefacility_initialized = true;
	}
}

void ots::setMsgFacAppName(const std::string& appType)
{
	std::string appName(appType);

	char hostname[256];
	if (gethostname(&hostname[0], 256) == 0) {
		std::string hostString(hostname);
		size_t pos = hostString.find(".");
		if (pos != std::string::npos && pos > 2) {
			hostString = hostString.substr(0, pos);
		}
		appName.append("-");
		appName.append(hostString);
	}
	mf::SetApplicationName(appName);
	mf::SetModuleName(appName);
	mf::SetContext(appName);
}
