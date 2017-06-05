#include "otsdaq-core/DataDecoders/TriggerData.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
TriggerData::TriggerData(void)
{}

//========================================================================================================================
TriggerData::~TriggerData(void)
{}

//========================================================================================================================
bool TriggerData::isTriggerHigh(uint32_t data)
{
  //int type     = data & 0xf;
  //int dataType = (data>>4) & 0xf;
  //if(type==8 && dataType==0xb) return true;
  if((data & 0xf)==8 && (data & 0xf0)==0xb0) return true;
  return false;
}

//========================================================================================================================
bool TriggerData::isTriggerLow(uint32_t data)
{
  //int type     = data & 0xf;
  //int dataType = (data>>4) & 0xf;
  //if(type==8 && dataType==0xa) return true;
  if((data & 0xf)==8 && (data & 0xf0)==0xa0) return true;
  return false;
}

//========================================================================================================================
uint32_t TriggerData::decodeTriggerHigh(uint32_t data)
{
  return (data>>8) & 0xffffff;
}

//========================================================================================================================
uint32_t TriggerData::decodeTriggerLow(uint32_t data)
{
  return (data>>16) & 0xffff;
}

//========================================================================================================================
uint64_t TriggerData::mergeTriggerHighAndLow(uint32_t triggerHigh, uint32_t triggerLow)
{
  //uint64_t trigger = 0;
  //trigger |= ((uint64_t)triggerHigh)<<16;
  //trigger |= (uint64_t)triggerLow;
  return (((uint64_t)triggerHigh)<<16) + triggerLow;
}

//========================================================================================================================
void TriggerData::insertTriggerHigh(uint64_t& trigger, uint32_t dataTriggerHigh)
{
  trigger |= ((uint64_t)decodeTriggerHigh(dataTriggerHigh))<<16;
}

//========================================================================================================================
void TriggerData::insertTriggerLow(uint64_t& trigger, uint32_t dataTriggerLow)
{
  trigger |= (uint64_t)decodeTriggerLow(dataTriggerLow);
}
