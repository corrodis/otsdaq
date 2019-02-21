#ifndef _ots_FEConfiguration_h_
#define _ots_FEConfiguration_h_

#include <map>
#include <string>
#include <vector>

#include "otsdaq-coreTableCore/TableBase.h"

namespace ots
{
class FEConfiguration : public TableBase
{
  public:
	FEConfiguration(void);
	virtual ~FEConfiguration(void);

	// Methods
	void init(ConfigurationManager* configManager);
	// Getters
	//	std::vector<std::string>  getListOfFEIDs       (void) const;
	//	std::vector<std::string>  getListOfFEIDs       (const std::string& supervisorType,
	// unsigned int supervisorInstance)                                const;
	//	//FIXME This is wrong because there can be same name interfaces on different
	// supervisors!!!!!!! I am doing it for the DQM :( 	const std::string
	// getFEInterfaceType   (const std::string& frontEndID) const; 	const std::string
	// getFEInterfaceType   (const std::string& supervisorType, unsigned int
	// supervisorInstance, const std::string& frontEndID) const;

	// Getters
	//	std::vector<unsigned int> getListOfFEWRs       (void) const;
	//	std::vector<unsigned int> getListOfFEWRs       (unsigned int supervisorInstance)
	// const; 	const std::string&        getFEWRInterfaceName (unsigned int id)
	// const;
	//
	//	std::vector<unsigned int> getListOfFEWs        (void) const;
	//	std::vector<unsigned int> getListOfFEWs        (unsigned int supervisorInstance)
	// const; 	const std::string&        getFEWInterfaceName  (unsigned int id)
	// const;
	//
	//	std::vector<unsigned int> getListOfFERs        (void) const;
	//	std::vector<unsigned int> getListOfFERs        (unsigned int supervisorInstance)
	// const; 	const std::string&        getFERInterfaceName  (unsigned int id)
	// const;

  private:
	enum
	{
		SupervisorType,
		SupervisorInstance,
		FrontEndId,
		FrontEndType
	};
	std::string composeUniqueName(std::string  supervisorName,
	                              unsigned int supervisorInstance) const
	{
		return supervisorName + std::to_string(supervisorInstance);
	}
	std::map<std::string, std::map<std::string, unsigned int>> typeNameToRow_;
};
}  // namespace ots
#endif
