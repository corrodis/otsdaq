/*
 * RegisterConfiguration.h
 *
 *  Created on: Jul 24, 2015
 *      Author: parilla
 */

#ifndef ots_RegisterConfiguration_h
#define ots_RegisterConfiguration_h

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class RegisterConfiguration : public ots::TableBase
{
  public:
	RegisterConfiguration(std::string staticConfigurationName);
	virtual ~RegisterConfiguration();

	void init(void);

  protected:
	enum
	{
		ComponentName,
		RegisterName,
		RegisterBaseAddress,
		RegisterSize,
		RegisterAccess
	};
};

}  // namespace ots

#endif /* REGISTERCONFIGURATION_H_ */
