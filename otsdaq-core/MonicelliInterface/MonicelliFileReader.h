#ifndef _ots_MonicelliFileReader_h
#define _ots_MonicelliFileReader_h

#include <string>
#include <TApplication.h>

class TFile;
class TTree;
class TBranch;
namespace monicelli
{
class Event;
class EventHeader;
class Geometry;
}

namespace ots
{

class MonicelliFileReader
{
public:
    MonicelliFileReader();
    ~MonicelliFileReader();

    bool         openGeoFile          (std::string fileName);
    bool         openEventsFile       (std::string fileName);
    void         closeGeoFile         (void);
    void         closeEventsFile      (void);
    unsigned int getNumberOfEvents    (void);
    std::string  getFileName          (void);
    void         readEvent            (unsigned int event);
    monicelli::Event*       getEventPointer      (void);
    monicelli::EventHeader* getEventHeaderPointer(void);
    monicelli::Geometry*    getGeometryPointer   (void);

private:
	int          argv;
	char**       argc;
    TApplication theApp_           ;
    TFile*       theGeoFile_       ;
    TFile*       theEventsFile_    ;
    TTree*       inputGeometryTree_;
    TTree*       inputEventTree_   ;
    TTree*       inputEventHeader_ ;

    monicelli::Event*       theEvent_         ;
    monicelli::EventHeader* theEventHeader_   ;
    monicelli::Geometry*    theGeometry_      ;
    TBranch*     theEventBranch_   ;
    TTree*       theEventTree_     ;
} ;

}
#endif
