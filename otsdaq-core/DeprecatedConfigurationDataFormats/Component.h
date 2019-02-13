/*
 * Component.h
 *
 *  Created on: Aug 3, 2015
 *      Author: parilla
 */

#ifndef HARDWARE_H_
#define HARDWARE_H_
#include <list>
#include <map>
#include <string>
#include "Register.h"

namespace ots {

class Component {
 public:
  Component(std::string name, std::string typeName = "");
  virtual ~Component();
  void addRegister(std::string name);
  void addRegister(std::string name, std::string baseAddress, int size, std::string access, int globalSequencePosition,
                   int globalValue);
  void addRegister(std::string name, std::string baseAddress, int size, std::string access,
                   int initializeSequencePosition, int initializeValue, int configureSequencePosition,
                   int configureValue);
  void addRegister(std::string name, std::string baseAddress, int size, std::string access,
                   int initializeSequencePosition, int initializeValue, int configureSequencePosition,
                   int configureValue, int startSequencePosition, int startValue, int haltSequencePosition,
                   int haltValue, int pauseSequencePosition, int pauseValue, int resumeSequencePosition,
                   int resumeValue);
  void setState(std::string state, std::pair<int, int> sequenceValuePair);
  // Getters
  std::list<Register> getRegisters(void);
  std::list<Register>* getRegistersPointer(void);
  std::string getComponentName(void);
  std::string getTypeName(void);

  // Print
  std::string printPair(std::pair<int, int>);
  void printInfo(void);

 protected:
  std::list<Register> registers_;
  std::string componentName_;
  std::string typeName_;
  // Register*				tempRegister_		;
};

}  // namespace ots

#endif /* HARDWARE_H_ */
