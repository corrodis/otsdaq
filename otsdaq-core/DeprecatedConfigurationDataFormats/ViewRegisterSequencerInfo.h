/*
 * ViewRegisterSequencerInfo.h
 *
 *  Created on: Aug 3, 2015
 *      Author: parilla
 */

#ifndef VIEWREGISTERSEQUENCERINFO_H_
#define VIEWREGISTERSEQUENCERINFO_H_
#include <string>

namespace ots {

class ViewRegisterSequencerInfo {
public:
	ViewRegisterSequencerInfo(std::string componentName,				std::string registerName,		int registerValue,		int sequenceNumber,		std::string state);
//							  std::pair<int, int> initializeState, 		std::pair<int, int> configurationState,
//							  std::pair<int, int> startState, 			std::pair<int, int> haltState,
//							  std::pair<int, int> pauseState, 			std::pair<int, int> resumeState);
	virtual ~ViewRegisterSequencerInfo();

	//Setters
	void						setState				(std::string state, std::pair <int, int> valueSequencePair);

	//Getters
  	const std::string& 			getComponentName    	(void) const;
  	const std::string& 			getRegisterName	 		(void) const;
  	const std::string&			getState				(void) const;
  	const std::pair<int, int>&	getValueSequencePair	(void) const;
  	const std::pair<int, int>&  getInitialize   	 	(void) const;
  	const std::pair<int, int>&  getConfiguration	 	(void) const;
  	const std::pair<int, int>&  getStart	    	 	(void) const;
  	const std::pair<int, int>&  getHalt		   	 		(void) const;
  	const std::pair<int, int>&  getPause		   	 	(void) const;
 	const std::pair<int, int>&  getResume	  	 		(void) const;
  	const int		   			getNumberOfColumns		(void) const;

protected:
  	std::string					componentName_		;
  	std::string					registerName_	 	;
  	std::string					state_				;
  	std::pair<int, int>			valueSequencePair_	;
  	std::pair<int, int>			initializeState_	;
  	std::pair<int, int>			configurationState_	;
  	std::pair<int, int>			startState_			;
  	std::pair<int, int>			haltState_			;
  	std::pair<int, int>			pauseState_			;
  	std::pair<int, int>			resumeState_		;

};
}
#endif /* VIEWREGISTERSEQUENCERINFO_H_ */
