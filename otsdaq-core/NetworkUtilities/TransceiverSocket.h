#ifndef _ots_TransceiverSocket_h_
#define _ots_TransceiverSocket_h_

#include "otsdaq-core/NetworkUtilities/TransmitterSocket.h"
#include "otsdaq-core/NetworkUtilities/ReceiverSocket.h"

#include <string>

namespace ots
{

class TransceiverSocket : public TransmitterSocket, public ReceiverSocket
{
public:
             TransceiverSocket(std::string IPAddress, unsigned int port=0);
    virtual ~TransceiverSocket(void);
protected:
    TransceiverSocket(void);

};

}

#endif
