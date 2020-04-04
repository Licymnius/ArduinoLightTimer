#include "GyverTM1637.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>

enum Mode { NORMAL, MENUCLOCK, MENUPROGRAM, MENUSENSOR, SETCLOCK, SELECTPROGRAM, SETPROGRAM, SETSENSOR };
enum Buttons { NONE, BUTTONMENU, BUTTONOK, BUTTONLIGHT, BUTTONPLUS, BUTTONMINUS };

//Time of ligth turned on by switch during sleep time
//900sec = 15min
const short switchLightTime = 900;

//Arduino modules pins parameters
//RealTimeClock DS1302
const byte RtcData = 2;
const byte RtcClock = 3;
const byte RtcReset = 4;

//Display module TM1637
const byte DisplayData = 6;
const byte DisplayClock = 5;

//LightSensor
const byte AnalogLight = A1;

const byte RelayPin = 8;

//Buttons analogue input
const byte ButtonsPin = A0;

//Buttons input voltage
const short ButtonsNotPressedStart = 950;
const short ButtonsNotPressedEnd = 1030;
const short BUTTONMENUStart = 0;
const short BUTTONMENUEnd = 20;
const short BUTTONOKStart = 580;
const short BUTTONOKEnd = 620;
const short BUTTONLIGHTStart = 640;
const short BUTTONLIGHTEnd = 679;
const short BUTTONPLUSStart = 685;
const short BUTTONPLUSEnd = 709;
const short BUTTONMINUSStart = 712;
const short BUTTONMINUSEnd = 740;

Buttons button;

//Whether seconds point is on
bool pointOn;

bool setMinutes;
bool offMode;
byte buttonTimer;
bool lightOn;
bool sleepMode;
short switchCountdown;

RtcDateTime setDateTime;
RtcDateTime setOnDateTime;
RtcDateTime setOffDateTime;
byte lightSensorThreshold;

ThreeWire myWire(RtcData, RtcClock, RtcReset);
RtcDS1302<ThreeWire> RealTimeClock(myWire);
GyverTM1637 displayModule(DisplayClock, DisplayData);
Mode deviceMode;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  RealTimeClock.Begin();
  if (RealTimeClock.GetIsWriteProtected())
    RealTimeClock.SetIsWriteProtected(false);

  if (!RealTimeClock.GetIsRunning())
    RealTimeClock.SetIsRunning(true);

  displayModule.clear();
  displayModule.brightness(7);
  
  setOnDateTime = RtcDateTime(1, 1, 1, RealTimeClock.GetMemory(0), RealTimeClock.GetMemory(1), 0);
  setOffDateTime = RtcDateTime(1, 1, 1, RealTimeClock.GetMemory(2), RealTimeClock.GetMemory(3), 0);
  lightSensorThreshold = RealTimeClock.GetMemory(4);
}

//Check the button is pressed
void CheckButtons() {
  int actualKeyValue = analogRead(ButtonsPin);  

  if (actualKeyValue >= ButtonsNotPressedStart && actualKeyValue <= ButtonsNotPressedEnd) {
    button = NONE;
    buttonTimer = 0;
    return;
  }

  if (button != NONE) {
    if (buttonTimer < 100)
      buttonTimer++;

    return;
  }

  if (actualKeyValue >= BUTTONMENUStart && actualKeyValue <= BUTTONMENUEnd) {
    button = BUTTONMENU;
    return;
  }
  else if (actualKeyValue >= BUTTONOKStart && actualKeyValue <= BUTTONOKEnd) {
    button = BUTTONOK;
    return;
  }
  else if (actualKeyValue >= BUTTONMINUSStart && actualKeyValue <= BUTTONMINUSEnd) {
    button = BUTTONMINUS;
    return;
  }
  else if (actualKeyValue >= BUTTONPLUSStart && actualKeyValue <= BUTTONPLUSEnd) {
    button = BUTTONPLUS;
    return;
  }
  else if (actualKeyValue >= BUTTONLIGHTStart && actualKeyValue <= BUTTONLIGHTEnd) {
    button = BUTTONLIGHT;
    return;
  }
}

//Buttons Pressed Hooks
void ProcessButtons() {
  switch (button)
  {
    case BUTTONMENU:
      if (switchCountdown != 0)
        return;
      
      switch (deviceMode) {
        case NORMAL:
          deviceMode = MENUCLOCK;
          displayModule.point(false);
          displayModule.displayByte(_4, _A, _C, 0x7e);
          break;
        case MENUCLOCK:
          deviceMode = MENUPROGRAM;
          displayModule.point(false);
          displayModule.displayByte(_N, _P, _O, 0x31);
          break;
        case MENUPROGRAM:
          deviceMode = MENUSENSOR;
          displayModule.displayByte(_C, _E, _H, _C);
          break;
        case MENUSENSOR:
          deviceMode = NORMAL;
          LedUpdate();
          break;
        case SETCLOCK:
          RealTimeClock.SetDateTime(RtcDateTime(1, 1, 1, setDateTime.Hour(), setDateTime.Minute(), 0));          
          deviceMode = NORMAL;
          LedUpdate();
          break;
        case SETPROGRAM:
          if (offMode) {
            RealTimeClock.SetMemory((uint8_t)2, setOffDateTime.Hour());
            RealTimeClock.SetMemory((uint8_t)3, setOffDateTime.Minute());
          }
          else {
            RealTimeClock.SetMemory((uint8_t)0, setOnDateTime.Hour());
            RealTimeClock.SetMemory((uint8_t)1, setOnDateTime.Minute());
          }
          deviceMode = NORMAL;
          break;
        case SETSENSOR:
          RealTimeClock.SetMemory((uint8_t)4, lightSensorThreshold);
          deviceMode = NORMAL;
          break;
      }
      break;
    case BUTTONOK:
      switch (deviceMode) {
        case MENUPROGRAM:
          deviceMode = SELECTPROGRAM;
          displayModule.displayByte(_O, _n, _empty, _empty);
          break;
        case SETCLOCK:
        case SETPROGRAM:
          setMinutes = !setMinutes;
          break;
        case SELECTPROGRAM:
          deviceMode = SETPROGRAM;
          displayModule.point(true);

          if (offMode)
            displayModule.displayClock(setOffDateTime.Hour(), setOffDateTime.Minute());
          else
            displayModule.displayClock(setOnDateTime.Hour(), setOnDateTime.Minute());

          break;
        case MENUCLOCK:
          deviceMode = SETCLOCK;
          displayModule.point(true);
          setDateTime = RealTimeClock.GetDateTime();          
          displayModule.displayClock(setDateTime.Hour(), setDateTime.Minute());
          break;
        case MENUSENSOR:
          deviceMode = SETSENSOR;
          break;
      }
      break;
    case BUTTONLIGHT:      
      if (sleepMode && deviceMode == NORMAL) {
        if (switchCountdown != 0 && switchCountdown != switchLightTime && switchCountdown != switchLightTime * 2) {
          switchCountdown = 0;
          return;
        }
         
        switchCountdown = buttonTimer > 30 ? switchLightTime * 2 : switchLightTime;        
      }      
      break;
    case BUTTONPLUS:
      SetTime(false);
      break;
    case BUTTONMINUS:
      SetTime(true);
      break;
  }
}

void SetTime(bool minus)
{
  switch (deviceMode) {
    case SELECTPROGRAM:
      offMode = !offMode;
      if (offMode)
        displayModule.displayByte(_O, _F, _F, _empty);
      else
        displayModule.displayByte(_O, _n, _empty, _empty);

      LedUpdate();
      break;
    case SETCLOCK:
    case SETPROGRAM:
      RtcDateTime *dateTime;
      dateTime = deviceMode == SETCLOCK ? &setDateTime : (offMode ? &setOffDateTime : &setOnDateTime);
      *dateTime += (setMinutes ? (!minus && dateTime->Minute() == 59 || minus && dateTime->Minute() == 00 ? -3540 : 60) : (!minus && dateTime->Hour() == 23 || minus && dateTime->Hour() == 00 ? -82800 : 3600)) * (minus ? -1 : 1);
      LedUpdate();
      break;
    case SETSENSOR:
      if (lightSensorThreshold > 0 && minus)
        lightSensorThreshold--;
      else if (lightSensorThreshold < 255 && !minus)
        lightSensorThreshold++;

      LedUpdate();
      break;
  }
}

//Update Led Indicator Data
void LedUpdate() {
  pointOn = !pointOn;
  RtcDateTime dateTime = RealTimeClock.GetDateTime();

  if (switchCountdown > 0) {    
    displayModule.displayClock(switchCountdown / 60, switchCountdown % 60);
    return;
  }

  switch (deviceMode) {
    case NORMAL:
      displayModule.point(pointOn);      
      displayModule.displayClock(dateTime.Hour(), dateTime.Minute());
      break;
    case SETSENSOR:
      displayModule.displayInt(lightSensorThreshold);
      break;
    case SETCLOCK:
    case SETPROGRAM:
      //Выбираем что показывать время включения или время отключения или настройку текущего времени
      RtcDateTime displayDateTime = deviceMode == SETCLOCK ? setDateTime : (dateTime = offMode ? setOffDateTime : setOnDateTime);

      //Digits blinking
      if (button != NONE || pointOn)
        displayModule.displayClock(displayDateTime.Hour(), displayDateTime.Minute());
      else {
        if (setMinutes) {
          byte firstDigit = displayDateTime.Hour() / 10;
          byte data[4] = {firstDigit == 0 ? (byte)10 : firstDigit, (byte)(displayDateTime.Hour() - firstDigit * 10), 10, 10};
          displayModule.display(data);
        }
        else {
          byte firstDigit = displayDateTime.Minute() / 10;
          byte data[4] = {10, 10, firstDigit, (byte)(displayDateTime.Minute() - firstDigit * 10)};
          displayModule.display(data);
        }
      }
      break;
  }
}

void CheckLightTimer() {
  RtcDateTime currentDateTime = RealTimeClock.GetDateTime();

  if (switchCountdown > 0) {
    if (!lightOn) {
      displayModule.point(true);
      lightOn = true;
      displayModule.brightness(3);
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(RelayPin, HIGH);
    }

    switchCountdown--;
    return;
  }

  sleepMode = (currentDateTime.Hour() > setOffDateTime.Hour() || currentDateTime.Hour() == setOffDateTime.Hour() && currentDateTime.Minute() >= setOffDateTime.Minute()) &&  
      (currentDateTime.Hour() < setOnDateTime.Hour() || currentDateTime.Hour() == setOnDateTime.Hour() && currentDateTime.Minute() <= setOnDateTime.Minute());
    
  //Turning the light on
  if (!sleepMode && !lightOn && lightSensorThreshold * 4 < analogRead(AnalogLight)) {
    lightOn = true;
    displayModule.brightness(3);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(RelayPin, HIGH);
  }

  //Turning the light off
  if (lightOn && (lightSensorThreshold * 4 > analogRead(AnalogLight) || sleepMode)) {
    lightOn = false;
    displayModule.brightness(sleepMode ? 1 : 7);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(RelayPin, LOW);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 10; i++) {
    //100ms period
    CheckButtons();

    if (buttonTimer == 0 || buttonTimer > 5)
      ProcessButtons();    

    if (deviceMode == SETCLOCK && i == 5)
      LedUpdate();

    delay(100);
  }

  //1s period
  LedUpdate();
  CheckLightTimer();  
}
