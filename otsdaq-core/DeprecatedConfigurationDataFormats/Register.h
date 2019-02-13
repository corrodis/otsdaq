/*
 * Register.h
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#ifndef _ots_Register_h_
#define _ots_Register_h_
#include <string>

namespace ots {

class Register {
 public:
  Register(std::string name);
  virtual ~Register();

  // Setters
  void setState(std::string state, std::pair<int, int> valueSequencePair);
  void fillRegisterInfo(std::string registerBaseAddress, int registerSize, std::string registerAccess);
  void setInitialize(std::pair<int, int> initialize);
  void setConfigure(std::pair<int, int> configure);
  void setStart(std::pair<int, int> start);
  void setHalt(std::pair<int, int> halt);
  void setPause(std::pair<int, int> pause);
  void setResume(std::pair<int, int> resume);

  // Getters
  std::string getName(void);
  std::string getBaseAddress(void);
  int getSize(void);
  std::string getAccess(void);
  std::pair<int, int> getInitialize(void);
  std::pair<int, int> getConfigure(void);
  std::pair<int, int> getStart(void);
  std::pair<int, int> getHalt(void);
  std::pair<int, int> getPause(void);
  std::pair<int, int> getResume(void);

 protected:
  std::string registerName_;
  std::string registerBaseAddress_;
  int registerSize_;
  std::string registerAccess_;

  // pair  sequenced position : value
  // position of -1 translates to off
  std::pair<int, int> initialize_;
  std::pair<int, int> configuration_;
  std::pair<int, int> start_;
  std::pair<int, int> halt_;
  std::pair<int, int> pause_;
  std::pair<int, int> resume_;
};

}  // namespace ots

#endif /* REGISTER_H_ */
