#ifndef _ots_ConfigurationGroupKey_h_
#define _ots_ConfigurationGroupKey_h_

#include <ostream>

namespace ots
{

//ConfigurationGroupKey is the type used for versions association with global configurations
//(whereas ConfigurationVersion is the type used for version associated with a configuration table)
class ConfigurationGroupKey
{

public:

    explicit 				ConfigurationGroupKey 		(unsigned int key=INVALID);
    explicit 				ConfigurationGroupKey 		(char * const &groupString);
    explicit 				ConfigurationGroupKey 		(const std::string &groupString);
    virtual  				~ConfigurationGroupKey		(void);

    unsigned int 			key						(void) const;
    bool					isInvalid				(void) const;
    std::string				toString				(void) const;

    //Operators
    ConfigurationGroupKey& 	operator=				(const unsigned int key);
    bool 					operator==				(unsigned int key) const;
    bool 					operator==				(const ConfigurationGroupKey& key) const;
    bool 					operator!=				(unsigned int key) const;
    bool 					operator!=				(const ConfigurationGroupKey& key) const;
    bool 					operator<				(const ConfigurationGroupKey& key) const;
    bool 					operator>				(const ConfigurationGroupKey& key) const;


    friend std::ostream& 	operator<< 				(std::ostream& out, const ConfigurationGroupKey& key)
    {
    	out << key.toString();
    	return out;
    }

    static ConfigurationGroupKey	getNextKey 			(const ConfigurationGroupKey& key=ConfigurationGroupKey());
    static std::string				getFullGroupString	(const std::string &groupName, const ConfigurationGroupKey& key);
    static void						getGroupNameAndKey 	(const std::string &fullGroupString, std::string &groupName, ConfigurationGroupKey& key);
    static const unsigned int	    getDefaultKey		(void);
    static const unsigned int	    getInvalidKey		(void);

private:
	static const unsigned int INVALID;
	static const unsigned int DEFAULT;
    unsigned int key_;
};
}
#endif
