#include "GyverTM1637.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>

enum Mode { NORMAL, MENUCLOCK, MENUPROGRAM, SETCLOCK, SELECTPROGRAM, SETPROGRAM };
enum Buttons { NONE, BUTTON1, BUTTON2, BUTTON3, BUTTON4, BUTTON5 };

//Arduino modules pins parameters
//RealTimeClock DS1302
const byte RtcData = 2;
const byte RtcClock = 3;
const byte RtcReset = 4;

//Display module TM1637
const byte DisplayData = 6;
const byte DisplayClock = 5;

//Buttons analogue input
const byte ButtonsPin = A0;

//Buttons input voltage
const short ButtonsNotPressedStart = 950;
const short ButtonsNotPressedEnd = 1030;
const short Button1Start = 0;
const short Button1End = 20;
const short Button2Start = 610;
const short Button2End = 630;
const short Button3Start = 670;
const short Button3End = 690;
const short Button4Start = 700;
const short Button4End = 712;
const short Button5Start = 715;
const short Button5End = 740;

Buttons button;

//Whether seconds point is on
bool pointOn;

//display brightness
uint8_t brightness = 7;

bool setMinutes;
bool offMode;
byte buttonTimer;

RtcDateTime setDateTime;
RtcDateTime setOnDateTime;
RtcDateTime setOffDateTime;

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

  Serial.begin(9600);  
  setOnDateTime = RtcDateTime(0, 0, 0, RealTimeClock.GetMemory(0), RealTimeClock.GetMemory(1), 0);
  setOffDateTime = RtcDateTime(0, 0, 0, RealTimeClock.GetMemory(2), RealTimeClock.GetMemory(3), 0);
}

//Check the button is pressed
void CheckButtons() {
  int actualKeyValue = analogRead(ButtonsPin);

  if (actualKeyValue >= ButtonsNotPressedStart && actualKeyValue <= ButtonsNotPressedEnd) {
    button = NONE;
    buttonTimer = 0;
    return;
  }

  if (button != NONE)    {
    if (buttonTimer < 10)
      buttonTimer++;

    return;
  }

  if (actualKeyValue >= Button1Start && actualKeyValue <= Button1End) {
    button = BUTTON1;
    return;
  }
  else if (actualKeyValue >= Button2Start && actualKeyValue <= Button2End) {
    button = BUTTON2;
    return;
  }
  else if (actualKeyValue >= Button5Start && actualKeyValue <= Button5End) {
    button = BUTTON5;
    return;
  }
  else if (actualKeyValue >= Button4Start && actualKeyValue <= Button4End) {
    button = BUTTON4;
    return;
  }
  else if (actualKeyValue >= Button3Start && actualKeyValue <= Button3End) {
    button = BUTTON3;
    return;
  }
}

//Buttons Pressed Hooks
void ProcessButtons() {
  switch (button)
  {
    case BUTTON1:
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
        case SETCLOCK:
          RealTimeClock.SetDateTime(setDateTime);
          deviceMode = NORMAL;
          LedUpdate();
          break;
        case MENUPROGRAM:
          deviceMode = NORMAL;
          LedUpdate();
          break;
        case SETPROGRAM:
          if (offMode){
            RealTimeClock.SetMemory((uint8_t)2, setOffDateTime.Hour());
            RealTimeClock.SetMemory((uint8_t)3, setOffDateTime.Minute());
            }            
          else {
            RealTimeClock.SetMemory((uint8_t)0, setOnDateTime.Hour());
            RealTimeClock.SetMemory((uint8_t)1, setOnDateTime.Minute());
          }
          deviceMode = NORMAL;
          break;
      }
      break;
    case BUTTON2:
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
      }
      break;
    case BUTTON4:
      SetTime(false);
      LedUpdate();
      break;
    case BUTTON5:
      SetTime(true);
      LedUpdate();

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
      break;
    case SETCLOCK:
    case SETPROGRAM:
      RtcDateTime *dateTime;
      dateTime = deviceMode == SETCLOCK ? &setDateTime : (offMode ? &setOffDateTime : &setOnDateTime);
      *dateTime += (setMinutes ? (!minus && dateTime->Minute() == 59 || minus && dateTime->Minute() == 00 ? -3540 : 60) : (!minus && dateTime->Hour() == 23 || minus && dateTime->Hour() == 00 ? -82800 : 3600)) * (minus ? -1 : 1);
      Serial.println(setOnDateTime.TotalSeconds());
      break;
  }
}

//Update Led Indicator Data
void LedUpdate() {
  pointOn = !pointOn;
  RtcDateTime dateTime = RealTimeClock.GetDateTime();

  switch (deviceMode) {
    case NORMAL:
      displayModule.point(pointOn);
      displayModule.brightness(brightness);
      displayModule.displayClock(dateTime.Hour(), dateTime.Minute());
      break;
    case SETCLOCK:
    case SETPROGRAM:
      //Выбираем что показывать время включения или время отключения или настройку текущего времени
      RtcDateTime displayDateTime = deviceMode == SETCLOCK ? setDateTime : (dateTime = offMode ? setOffDateTime : setOnDateTime);

      //Отстутствие мигание цифры
      if (button != NONE || pointOn)
        displayModule.displayClock(displayDateTime.Hour(), displayDateTime.Minute());
      else {
        //Мигиние цифры
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

//Check timers Fired
void CheckTimers() {
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < 10; i++ ) {
    //100ms
    CheckButtons();

    if (buttonTimer == 0 || buttonTimer > 5)
      ProcessButtons();

    if (button != NONE)
      Serial.println(button);

    if (deviceMode == SETCLOCK && i == 5)
      LedUpdate();

    delay(100);
  }

  //1s
  LedUpdate();
}

void loop() {
  // put your main code here, to run repeatedly:
  CheckTimers();
}
