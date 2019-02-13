#ifndef otsdaq_otsdaq_core_GatewaySupervisor_ARTDAQCommandable_h
#define otsdaq_otsdaq_core_GatewaySupervisor_ARTDAQCommandable_h

#include "artdaq/Application/Commandable.hh"
#include "artdaq/ExternalComms/CommanderInterface.hh"
#include "boost/thread.hpp"

namespace ots {
	class GatewaySupervisor;

	class ARTDAQCommandable : public artdaq::Commandable
	{
	public:
		explicit ARTDAQCommandable(GatewaySupervisor* super);
		virtual ~ARTDAQCommandable();

		void init(int commanderId, std::string commanderType);
	private:
		GatewaySupervisor* theSupervisor_;
		std::unique_ptr<artdaq::CommanderInterface> theCommander_;
		boost::thread commanderThread_;

	public:
		// Commandable Overrides
		// these methods provide the operations that are used by the state machine
		/**
		* \brief Perform the initialize transition.
		* \return Whether the transition succeeded
		*/
		bool do_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t) override;

		/**
		* \brief Perform the start transition.
		* \return Whether the transition succeeded
		*/
		bool do_start(art::RunID, uint64_t, uint64_t) override;

		/**
		* \brief Perform the stop transition.
		* \return Whether the transition succeeded
		*/
		bool do_stop(uint64_t, uint64_t) override;

		/**
		* \brief Perform the pause transition.
		* \return Whether the transition succeeded
		*/
		bool do_pause(uint64_t, uint64_t) override;

		/**
		* \brief Perform the resume transition.
		* \return Whether the transition succeeded
		*/
		bool do_resume(uint64_t, uint64_t) override;

		/**
		* \brief Perform the shutdown transition.
		* \return Whether the transition succeeded
		*/
		bool do_shutdown(uint64_t) override;

		/**
		* \brief Run a module-defined command with the given parameter string		*/
		bool do_meta_command(std::string const&, std::string const&) override;
	};
}

#endif //otsdaq_otsdaq_core_GatewaySupervisor_ARTDAQCommandable_h
