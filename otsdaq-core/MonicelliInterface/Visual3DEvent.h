#ifndef _ots_Visual3DEvent_h
#define _ots_Visual3DEvent_h

#include <vector>

namespace ots
{

class VisualHit
{
public:
	double x;
	double y;
	double z;
	double charge;
	VisualHit& operator*(double factor){this->x*=factor;this->y*=factor;this->z*=factor;return *this;}
};

class VisualTrack
{
public:
	double slopeX;
	double interceptX;
	double slopeY;
	double interceptY;
};

typedef std::vector<VisualHit>   VisualHits;
typedef std::vector<VisualTrack> VisualTracks;

class Visual3DEvent
{
public:

	Visual3DEvent(){;}
    ~Visual3DEvent(void){;}

    void addHit  (VisualHit&   hit)  {hits_  .push_back(hit);}
    void addTrack(VisualTrack& track){tracks_.push_back(track);}

    const VisualHits&   getHits  (void) const {return hits_;}
    const VisualTracks& getTracks(void) const {return tracks_;}
private:
    VisualHits   hits_;
    VisualTracks tracks_;
};

typedef std::vector<Visual3DEvent> Visual3DEvents;

}

#endif
