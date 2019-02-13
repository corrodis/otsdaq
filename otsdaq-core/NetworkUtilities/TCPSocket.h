#ifndef _ots_TCPSocket_h_
#define _ots_TCPSocket_h_

#include <netinet/in.h>
#include <sys/types.h>
#include <mutex>
#include <string>
#include <vector>

#include "artdaq/DAQdata/TCPConnect.hh"
#include "artdaq/DAQdata/TCP_listen_fd.hh"

#define OTS_MAGIC 0x5F4F54534441515F /* _OTSDAQ_ */

namespace ots {

class TCPSocket {
 public:
  TCPSocket(const std::string& senderHost, unsigned int senderPort, int receiveBufferSize = 0x10000);
  TCPSocket(unsigned int listenPort, int sendBufferSize = 0x10000);
  virtual ~TCPSocket(void);

  void connect(double tmo_s = 10.0);

  int send(const uint8_t* data, size_t size);
  int send(const std::string& buffer);
  int send(const std::vector<uint32_t>& buffer);
  int send(const std::vector<uint16_t>& buffer);

  int receive(uint8_t* buffer, unsigned int timeoutSeconds, unsigned int timeoutUSeconds);
  int receive(std::string& buffer, unsigned int timeoutSeconds = 1, unsigned int timeoutUSeconds = 0);
  int receive(std::vector<uint32_t>& buffer, unsigned int timeoutSeconds = 1, unsigned int timeoutUSeconds = 0);

 protected:
  TCPSocket(void);

  std::string host_;
  unsigned int port_;

  int TCPSocketNumber_;
  int SendSocket_;
  bool isSender_;
  int bufferSize_;
  size_t chunkSize_;

  mutable std::mutex socketMutex_;

 private:
  struct MagicPacket {
    uint64_t ots_magic;
    unsigned int dest_port;
  };
  MagicPacket makeMagicPacket(unsigned int port) {
    MagicPacket m;
    m.ots_magic = OTS_MAGIC;
    m.dest_port = port;
    return m;
  }
  bool checkMagicPacket(MagicPacket magic) { return magic.ots_magic == OTS_MAGIC && magic.dest_port == port_; }
};

}  // namespace ots

#endif
