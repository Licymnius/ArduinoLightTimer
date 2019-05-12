#include "GyverTM1637.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>

enum Mode { NORMAL, MENUCLOCK, MENUPROGRAM, SETCLOCK, SELECTPROGRAM, SETPROGRAM };

//Параметры подключения модулей
const byte RtcData = 2;
const byte RtcClock = 3;
const byte RtcReset = 4;
const byte DisplayData = 6;
const byte DisplayClock = 5;
const byte ButtonsPin = A0;
const short Button1Start = 0;
const short Button1End = 20;
const short Button2Start = 610;
const short Button2End = 630;
const short Button3Start = 670;
const short Button3End = 690;
const short Button4Start = 700;
const short Button4End = 715;
const short Button5Start = 722;
const short Button5End = 740;
bool pointOn;
uint8_t brightness = 7;
byte buttonLock;
bool setMinutes;
bool offMode;

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
  Serial.println(RealTimeClock.GetMemory(0));
  setOnDateTime = RtcDateTime((uint32_t)RealTimeClock.GetMemory(0));
}

//Buttons Pressed Hooks
void CheckButtons() {
  if (buttonLock != 0)
    return;

  int actualKeyValue = analogRead(ButtonsPin);
  //Нажатие кнопки меню
  if (actualKeyValue >= Button1Start && actualKeyValue <= Button1End) {
    //Кнопка 1 нажата
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
        if(offMode)
          RealTimeClock.SetMemory((uint8_t)1, setOffDateTime.TotalSeconds());
        else
          RealTimeClock.SetMemory((uint8_t)0, setOnDateTime.TotalSeconds());

        Serial.println(RealTimeClock.GetMemory(0));
        break;
    }
    Serial.println("Button 1 Pressed");
  } else if (actualKeyValue >= Button2Start && actualKeyValue <= Button2End) {
    //Кнопка 2 нажата
    switch (deviceMode) {
      case MENUCLOCK:
        deviceMode = SETCLOCK;
        displayModule.point(true);
        setDateTime = RealTimeClock.GetDateTime();
        displayModule.displayClock(setDateTime.Hour(), setDateTime.Minute());
        break;
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
    }
    Serial.println("Button 2 Pressed");
  } else if (actualKeyValue >= Button3Start && actualKeyValue <= Button3End) {
    //Кнопка 3 нажата
    Serial.println("Button 3 Pressed");
  } else if (actualKeyValue >= Button4Start && actualKeyValue <= Button4End) {
    //Кнопка 4 нажата
    SetTime(false);
    Serial.println("Button 4 Pressed");
  } else if (actualKeyValue >= Button5Start && actualKeyValue <= Button5End) {
    //Кнопка 5 нажата
    SetTime(true);
    Serial.println("Button 5 Pressed");
  }

  //Подавление дребезга кнопок
  if (actualKeyValue >= Button1Start && actualKeyValue <= Button5End)
    buttonLock = 2;
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
      if (buttonLock || pointOn)
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
    if (buttonLock == 0)
      CheckButtons();
    else
      buttonLock--;

    delay(100);

    if (deviceMode == SETCLOCK && i == 5)
      LedUpdate();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  LedUpdate();
  CheckTimers();
}
