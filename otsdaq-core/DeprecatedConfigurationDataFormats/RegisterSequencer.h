/*
 * RegisterSequencer.h
 *
 *  Created on: Jul 24, 2015
 *      Author: parilla
 */

#ifndef REGISTERSEQUENCER_H_
#define REGISTERSEQUENCER_H_

#include "ConfigurationBase.h"
#include "ROCDACs.h"

namespace ots {

class RegisterSequencer: public ots::ConfigurationBase
{
public:
    RegisterSequencer (std::string staticConfigurationName);
    virtual 		  ~RegisterSequencer ();

    void 		  init (void);

    //getter

protected:

    enum{ComponentName,
         RegisterName,
		 RegisterValue,
		 SequencerNumber,
		 State
        };



};

}

#endif /* REGISTERCONFIGURATION_H_ */
