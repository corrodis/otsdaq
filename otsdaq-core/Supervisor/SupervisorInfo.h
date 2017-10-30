#ifndef _ots_SupervisorInfo_h_
#define _ots_SupervisorInfo_h_

#include <string>

namespace ots
{

class SupervisorInfo
{
public:
    SupervisorInfo (void) : status_("Unknown"), URL_("")
    {;}
    ~SupervisorInfo(void)
    {;}

    //Getters
    std::string getStatus(void) const
    {
        return status_;
    }

    //Setters
    void setStatus(std::string status)
    {
        status_=status;
    }

private:
    std::string status_;
    std::string URL_;
    int         port_;
    std::string fullURL_;
};

}

#endif
