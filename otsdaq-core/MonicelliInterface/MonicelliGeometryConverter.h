#ifndef _ots_MonicelliGeometryConverter_h
#define _ots_MonicelliGeometryConverter_h

#include "otsdaq-core/MonicelliInterface/MonicelliFileReader.h"
#include "otsdaq-core/MonicelliInterface/Visual3DGeometry.h"

namespace monicelli
{
class Geometry;
}

namespace ots
{

//class Visual3DEvents;

class MonicelliGeometryConverter
{
public:
    MonicelliGeometryConverter(void);
    ~MonicelliGeometryConverter(void);

    void                      loadGeometry(std::string fileName);
    const Visual3DGeometry& getGeometry (void) const;

private:
    void convertGeometry(void);

    MonicelliFileReader  theReader_;
    monicelli::Geometry* theMonicelliGeometry_;
    Visual3DGeometry     theVisual3DGeometry_;
};

}
#endif
