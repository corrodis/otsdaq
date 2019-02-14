#include "otsdaq-core/GatewaySupervisor/ARTDAQCommandable.h"
#include "otsdaq-core/GatewaySupervisor/GatewaySupervisor.h"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"

ots::ARTDAQCommandable::ARTDAQCommandable(GatewaySupervisor * super)
	: artdaq::Commandable()
	, theSupervisor_(super)
	, theCommander_(nullptr)
{}

ots::ARTDAQCommandable::~ARTDAQCommandable()
{
	if (theCommander_ != nullptr)
	{
		theCommander_->send_shutdown(0);
		if (commanderThread_.joinable()) commanderThread_.join();
	}
}

void ots::ARTDAQCommandable::init(int commanderId, std::string commanderType)
{
	fhicl::ParameterSet ps;
	ps.put<int>("id", commanderId);
	ps.put<std::string>("commanderPluginType", commanderType);

	theCommander_ = artdaq::MakeCommanderPlugin(ps, *this);
	boost::thread::attributes attrs;
	attrs.set_stack_size(4096 * 2000); // 8 MB
	commanderThread_ = boost::thread(attrs, boost::bind(&artdaq::CommanderInterface::run_server, theCommander_.get()));
}

bool ots::ARTDAQCommandable::do_initialize(fhicl::ParameterSet const & ps, uint64_t, uint64_t)
{
	std::vector<std::string> parameters;
	parameters.push_back(ps.get<std::string>("config_name"));

	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Initialize",
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	if (ret == "")
		ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Configure",
															theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															"ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

bool ots::ARTDAQCommandable::do_start(art::RunID, uint64_t, uint64_t)
{
	std::vector<std::string> parameters;
	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Start",
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

bool ots::ARTDAQCommandable::do_stop(uint64_t, uint64_t)
{
	std::vector<std::string> parameters;
	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Stop",
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

bool ots::ARTDAQCommandable::do_pause(uint64_t, uint64_t)
{
	std::vector<std::string> parameters;
	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Pause",
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

bool ots::ARTDAQCommandable::do_resume(uint64_t, uint64_t)
{
	std::vector<std::string> parameters;
	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Resume",
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

bool ots::ARTDAQCommandable::do_shutdown(uint64_t)
{
	std::vector<std::string> parameters;
	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Halt",
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	if (ret == "")
		ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, "Shutdown",
															theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															"ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

bool ots::ARTDAQCommandable::do_meta_command(std::string const & command, std::string const & args)
{
	std::vector<std::string> parameters;
	parameters.push_back(args);
	auto ret = theSupervisor_->attemptStateMachineTransition(nullptr, nullptr, command,
															 theSupervisor_->activeStateMachineName_, theSupervisor_->activeStateMachineWindowName_,
															 "ARTDAQCommandable", parameters);
	report_string_ = ret;
	return ret == "";
}

