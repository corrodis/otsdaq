#ifndef _ots_ConfigurationVersion_h_
#define _ots_ConfigurationVersion_h_

#include <ostream>

namespace ots
{

//ConfigurationVersion is the type used for version associated with a configuration table
//(whereas ConfigurationGroupKey is the type used for versions association with global configurations)
//
// Designed so that version type could be changed easily, e.g. to string
class ConfigurationVersion
{
public:
	static const unsigned int INVALID;
	static const unsigned int DEFAULT;
	static const unsigned int SCRATCH;

    explicit 				ConfigurationVersion 	(unsigned int version=INVALID);
    explicit 				ConfigurationVersion 	(char* const &versionStr);
    explicit 				ConfigurationVersion 	(const std::string &versionStr);
    virtual  				~ConfigurationVersion	(void);

    unsigned int 			version					(void) const;
    bool					isTemporaryVersion		(void) const;
    bool					isScratchVersion		(void) const;
    bool					isMockupVersion			(void) const;
    bool					isInvalid				(void) const;
    std::string				toString				(void) const;

    //Operators
    ConfigurationVersion& 	operator=				(const unsigned int version);
    bool 					operator==				(unsigned int version) const;
    bool 					operator==				(const ConfigurationVersion& version) const;
    bool 					operator!=				(unsigned int version) const;
    bool 					operator!=				(const ConfigurationVersion& version) const;
    bool 					operator<				(const ConfigurationVersion& version) const;
    bool 					operator>				(const ConfigurationVersion& version) const;
    bool 					operator<=				(const ConfigurationVersion& version) const { return !operator>(version);}
    bool 					operator>=				(const ConfigurationVersion& version) const { return !operator<(version);}

    friend std::ostream& 	operator<< 				(std::ostream& out, const ConfigurationVersion& version)
    {
    	if(version.isScratchVersion())
    		out << "ScratchVersion";
    	else if(version.isMockupVersion())
    		out << "Mock-up";
    	else if(version.isInvalid())
    		out << "InvalidVersion";
    	else
    		out << version.toString();
    	return out;
    }

    static ConfigurationVersion	getNextVersion (const ConfigurationVersion& version=ConfigurationVersion());
    static ConfigurationVersion	getNextTemporaryVersion (const ConfigurationVersion& version=ConfigurationVersion());

private:

	enum{ NUM_OF_TEMP_VERSIONS = 10000 };

    unsigned int version_;
    std::string  versionString_;

};
}
#endif
