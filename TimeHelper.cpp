#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "TimeHelper.h"

bool operator < (RtcDateTime firstDateTime, RtcDateTime secondDateTime)
{
  return firstDateTime.Hour() < secondDateTime.Hour() || firstDateTime.Hour() == secondDateTime.Hour() && firstDateTime.Minute() < secondDateTime.Minute();
}

bool operator >= (RtcDateTime firstDateTime, RtcDateTime secondDateTime)
{
  return firstDateTime == secondDateTime || firstDateTime.Hour() > secondDateTime.Hour() || firstDateTime.Hour() == secondDateTime.Hour() && firstDateTime.Minute() > secondDateTime.Minute();
}

bool operator == (RtcDateTime firstDateTime, RtcDateTime secondDateTime)
{
  return firstDateTime.Hour() == secondDateTime.Hour() && firstDateTime.Minute() == secondDateTime.Minute();
}
