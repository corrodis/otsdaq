/*
 * RegisterBase.h
 *
 *  Created on: Jul 29, 2015
 *      Author: parilla
 */

#ifndef _ots_RegisterBase_h_
#define _ots_RegisterBase_h_
#include "Component.h"
#include "ConfigurationBase.h"
#include "otsdaq-core/ConfigurationDataFormats/RegisterView.h"

#include <list>
#include <map>
#include <string>

namespace ots
{
class RegisterView;

class RegisterBase : public ots::ConfigurationBase
{
  public:
	RegisterBase (std::string configurationName, std::string typeName);
	virtual ~RegisterBase ();
	void init (void);

	const RegisterView& getRegisterView (void) const;
	RegisterView*       getRegisterViewPointer (void);
	RegisterView*       getMockupRegisterViewPointer (void);

	RegisterView* getTemporaryRegisterView (int temporaryVersion);
	std::string   getTypeName (void);

	std::string   componentTypeName_;
	RegisterView* activeRegisterView_;
	RegisterView  mockupRegisterView_;
	//Version and data associated to make it work like a cache.
	//It will be very likely just 1 version
	std::map<int, RegisterView>      registerViews_;
	std::map<std::string, Component> theComponentList_;
};

}  // namespace ots

#endif /* REGISTERBASE_H_ */
