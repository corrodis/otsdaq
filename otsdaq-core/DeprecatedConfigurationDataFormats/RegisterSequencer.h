/*
 * RegisterSequencer.h
 *
 *  Created on: Jul 24, 2015
 *      Author: parilla
 */

#ifndef REGISTERSEQUENCER_H_
#define REGISTERSEQUENCER_H_

#include "ROCDACs.h"
#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
class RegisterSequencer : public ots::TableBase
{
  public:
	RegisterSequencer(std::string staticTableName);
	virtual ~RegisterSequencer();

	void init(void);

	// getter

  protected:
	enum
	{
		ComponentName,
		RegisterName,
		RegisterValue,
		SequencerNumber,
		State
	};
};

}  // namespace ots

#endif /* REGISTERCONFIGURATION_H_ */
