#include <ThreeWire.h>
#include <RtcDS1302.h>

bool operator < (RtcDateTime firstDateTime, RtcDateTime secondDateTime);
bool operator >= (RtcDateTime firstDateTime, RtcDateTime secondDateTime);
bool operator == (RtcDateTime firstDateTime, RtcDateTime secondDateTime);
