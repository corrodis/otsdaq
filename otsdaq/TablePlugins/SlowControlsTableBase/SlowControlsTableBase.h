#ifndef _ots_SlowControlsTableBase_h_
#define _ots_SlowControlsTableBase_h_

#include "otsdaq/TableCore/TableBase.h"


namespace ots
{
// clang-format off
class SlowControlsTableBase : virtual public TableBase //virtual so future plugins can inherit from multiple table base classes
{
  public:
	SlowControlsTableBase(void);
	SlowControlsTableBase(std::string tableName, std::string* accumulatedExceptions = 0);

	virtual ~SlowControlsTableBase(void);

	virtual bool	slowControlsChannelListHasChanged 	(void) const = 0;
	virtual void	getSlowControlsChannelList			(std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>& channelList) const = 0;
	
	//boardReader{
	//	build vector .. based on table 1,2,3,4,5.. 
	//	pass vector outputPV()
	//}
	//DTC {
	//	build vector from config Tree
	//	pass vector to outputPV()
	//}
	
	//use table name to have different file names! (instead of DEFINES like in DTC)
	
	//is channel binary or not?.. then can handle all the same
	void outputPVListAndSetFlagsForEPICs(const std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>& channelList);

	// Column names
	struct ColChannel
	{
		std::string const colStatus_ 				= "Status";
		std::string const colUnits_ 				= "Units";
		std::string const colChannelDataType_		= "ChannelDataType";
		std::string const colLowLowThreshold_		= "LowLowThreshold";
		std::string const colLowThreshold_ 			= "LowThreshold";
		std::string const colHighThreshold_			= "HighThreshold";
		std::string const colHighHighThreshold_ 	= "HighHighThreshold";
	} channelColNames_;

private:

	#define EPICS_CONFIG_PATH (std::string(__ENV__("USER_DATA")) + "/" + "EPICSConfigurations/")
	#define EPICS_DIRTY_FILE_PATH \
		std::string( \
			getenv("OTSDAQ_EPICS_DATA")? \
				(std::string(getenv("OTSDAQ_EPICS_DATA")) + "/" + "dirtyFlag.txt"): \
				(EPICS_CONFIG_PATH + "/dirtyFlag.txt")  )
};
}  // namespace ots

#endif
