#ifndef _ots_MPIInitializer_h_
#define _ots_MPIInitializer_h_

#include "artdaq/Application/MPI2/MPISentry.hh"
#include "artdaq/DAQrate/quiet_mpi.hh"

#include <string>
#include <memory>

namespace ots
{

class MPIInitializer
{
public:

	MPIInitializer(void);
    ~MPIInitializer(void);
    void init(std::string name, artdaq::TaskType taskType);

    MPI_Comm getLocalGroupComm(void){return local_group_comm_;}
	int getRank() { return rank_; }

private:
    MPI_Comm local_group_comm_;
	int rank_;
	std::unique_ptr<artdaq::MPISentry> mpiSentry_;
};

}

#endif
