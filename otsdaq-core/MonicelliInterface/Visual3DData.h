#ifndef _ots_Visual3DData_h
#define _ots_Visual3DData_h

#include "otsdaq-core/MonicelliInterface/Visual3DEvent.h"

#include <map>
#include <string>
#include <vector>

namespace ots
{


class Visual3DData
{
public:
    Visual3DData(){;}
    virtual ~Visual3DData(void){;}

private:
    map<int,Visual3DEvents> theData_;
};

}

#endif
