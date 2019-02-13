/*
 * RegisterConfiguration.h
 *
 *  Created on: Jul 24, 2015
 *      Author: parilla
 */

#ifndef ots_RegisterConfiguration_h
#define ots_RegisterConfiguration_h

#include "ConfigurationBase.h"

namespace ots {

class RegisterConfiguration: public ots::ConfigurationBase
{
public:
    RegisterConfiguration (std::string staticConfigurationName);
    virtual 		  ~RegisterConfiguration ();

    void 		  init (void);
protected:

    enum{ComponentName,
         RegisterName,
         RegisterBaseAddress,
         RegisterSize,
         RegisterAccess
        };
};

}

#endif /* REGISTERCONFIGURATION_H_ */
