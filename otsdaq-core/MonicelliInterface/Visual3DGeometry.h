#ifndef _ots_Visual3DGeometry_h
#define _ots_Visual3DGeometry_h

#include "otsdaq-core/MonicelliInterface/Visual3DEvent.h"

#include <vector>

namespace ots
{

class Point
{
public:
	double x;
	double y;
	double z;
	Point& operator*(double factor){this->x*=factor;this->y*=factor;this->z*=factor;return *this;}
};

typedef std::vector<Point> Points;

class Visual3DShape
{
public:
	//0->(0,0) 1->(0,numberOfColumns), 2->(numberOfRows,numberOfColumns), 3->(numberOfRows,0)
	//3-------2
	//|       |
	//0-------1
	Points corners;
	int      numberOfRows;
	int      numberOfColumns;
};

typedef std::vector<Visual3DShape> Visual3DShapes;

class Visual3DGeometry
{
public:
    Visual3DGeometry(){;}
    virtual ~Visual3DGeometry(void){;}

    void reset(void){theShapes_.clear();}

    //Setters
    void addShape(const Visual3DShape& shape) {theShapes_.push_back(shape);}
    //Getters
    const Visual3DShapes& getShapes(void) const {return theShapes_;}

private:
    Visual3DShapes theShapes_;
};

}

#endif
