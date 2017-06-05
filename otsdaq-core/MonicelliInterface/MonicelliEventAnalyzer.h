#ifndef _ots_MonicelliEventAnalyzer_h
#define _ots_MonicelliEventAnalyzer_h

#include "otsdaq-core/MonicelliInterface/MonicelliFileReader.h"
#include "otsdaq-core/MonicelliInterface/Visual3DEvent.h"

namespace monicelli
{
class Event;
class EventHeader;
class Geometry;
}

namespace ots
{

//class Visual3DEvents;

class MonicelliEventAnalyzer
{
public:
    MonicelliEventAnalyzer(void);
    ~MonicelliEventAnalyzer(void);

    void                    load     (std::string fileName);
    const Visual3DEvents& getEvents(void);

private:
    void analyzeEvent(unsigned int event);

    MonicelliFileReader     theReader_;
    monicelli::Event*       theMonicelliEvent_   ;
    monicelli::EventHeader* theMonicelliHeader_  ;
    monicelli::Geometry*    theMonicelliGeometry_;
    Visual3DEvents          theVisualEvents_;
};

}
#endif
