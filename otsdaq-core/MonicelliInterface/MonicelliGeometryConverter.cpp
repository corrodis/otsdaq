#include "otsdaq-core/MonicelliInterface/MonicelliGeometryConverter.h"
#include "otsdaq-core/MonicelliInterface/MonicelliFileReader.h"
#include "otsdaq-core/MonicelliInterface/Visual3DGeometry.h"

#include "otsdaq-core/MonicelliInterface/Geometry.h"
#include "otsdaq-core/MonicelliInterface/Detector.h"

using namespace ots;


//========================================================================================================================
MonicelliGeometryConverter::MonicelliGeometryConverter(void)
{}

//========================================================================================================================
MonicelliGeometryConverter::~MonicelliGeometryConverter(void)
{}

//========================================================================================================================
void MonicelliGeometryConverter::loadGeometry(std::string fileName)
{
	theVisual3DGeometry_.reset();
    if(!theReader_.openGeoFile(fileName))
        return;

    theMonicelliGeometry_ = theReader_.getGeometryPointer() ;

    convertGeometry();
    theReader_.closeGeoFile();
}

//========================================================================================================================
const Visual3DGeometry& MonicelliGeometryConverter::getGeometry(void) const
{
    return theVisual3DGeometry_;
}

//========================================================================================================================
void MonicelliGeometryConverter::convertGeometry(void)
{
    for(monicelli::Geometry::iterator it=theMonicelliGeometry_->begin(); it!=theMonicelliGeometry_->end(); it++)
    {
    	Visual3DShape tmpShape;
    	Point         tmpPoint;
    	tmpPoint.x = 0;
    	tmpPoint.y = 0;
    	tmpPoint.z = 0;
        it->second->fromLocalToGlobal(&tmpPoint.x,&tmpPoint.y,&tmpPoint.z);
        tmpShape.corners.push_back(tmpPoint*10);//To become um
    	tmpPoint.x = it->second->getDetectorLengthX();
    	tmpPoint.y = 0;
    	tmpPoint.z = 0;
        it->second->fromLocalToGlobal(&tmpPoint.x,&tmpPoint.y,&tmpPoint.z);
        tmpShape.corners.push_back(tmpPoint*10);//To become um
    	tmpPoint.x = it->second->getDetectorLengthX();
    	tmpPoint.y = it->second->getDetectorLengthY();
    	tmpPoint.z = 0;
        it->second->fromLocalToGlobal(&tmpPoint.x,&tmpPoint.y,&tmpPoint.z);
        tmpShape.corners.push_back(tmpPoint*10);//To become um
    	tmpPoint.x = 0;
    	tmpPoint.y = it->second->getDetectorLengthY();
    	tmpPoint.z = 0;
        it->second->fromLocalToGlobal(&tmpPoint.x,&tmpPoint.y,&tmpPoint.z);
        tmpShape.corners.push_back(tmpPoint*10);//To become um
        tmpShape.numberOfRows    = it->second->getNumberOfRows();
        tmpShape.numberOfColumns = it->second->getNumberOfCols();
        theVisual3DGeometry_.addShape(tmpShape);
    }

}
