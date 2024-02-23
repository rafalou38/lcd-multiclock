
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "chars.h"

#define BTN_MODE 12
#define BTN_PLAY 11
#define BTN_STOP 10

#define BTN_up(x) digitalRead(x) != states[x] && digitalRead(x) == false
#define fit(x) (16 / 2) - (x.length() / 2)

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
  lcd.init();
  lcd.createChar(0, bar_full);
  lcd.createChar(1, bar_1);
  lcd.createChar(2, bar_2);
  lcd.createChar(3, bar_3);
  lcd.createChar(4, bar_4);
  lcd.createChar(5, bar_5);
  lcd.createChar(6, bar_open);
  lcd.createChar(7, bar_close);

  Serial.begin(9600);

  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
}

bool states[13] = {false};

int currentMode = 0;

bool mode_released = false;
bool play_released = false;
bool stop_released = false;

void loop()
{
  mode_released = BTN_up(BTN_MODE);
  states[BTN_MODE] = digitalRead(BTN_MODE);

  play_released = BTN_up(BTN_PLAY);
  states[BTN_PLAY] = digitalRead(BTN_PLAY);

  stop_released = BTN_up(BTN_STOP);
  states[BTN_STOP] = digitalRead(BTN_STOP);

  if (mode_released)
  {
    Serial.println("Changement de mode");

    currentMode = (currentMode + 1) % 2;
    lcd.clear();
  }

  lcd.backlight();
  switch (currentMode)
  {
  case 0:
    chrono();
    break;

  case 1:
    minuteur();
    break;

  default:
    break;
  }
}

unsigned long chronoStart = 0;
unsigned long pausedValue = 0;
unsigned long chronoSecsPrev = 0;
enum AppState
{
  unset,
  stopped,
  running,
  paused
};
AppState chronoState = unset;

String pad(int val)
{
  if (val < 10)
  {
    return "0" + String(val);
  }
  else
  {
    return String(val);
  }
}

void displayTime(unsigned long time)
{
  // Serial.println("Display time: " + String((long)time));
  unsigned long secs = time / 1000;
  // if (secs != chronoSecsPrev || secs == 0)
  // {
  lcd.setCursor((16 / 2) - (8 / 2), 0);

  lcd.print(pad((secs / (60 * 60)))); // hours
  lcd.print(":");
  lcd.print(pad((secs / 60) % (60))); // minutes
  lcd.print(":");
  lcd.print(pad(secs % 60)); // secs

  chronoSecsPrev = secs;
  // }
}
void chrono()
{
  String text;
  switch (chronoState)
  {
  case unset:
    lcd.clear();
    displayTime(0);

    chronoState = stopped;
    break;

  case stopped:
    text = "CHRONO";
    lcd.setCursor(fit(text), 1);
    lcd.print(text);
    displayTime(0);
    if (play_released)
    {
      lcd.clear();
      chronoStart = millis();
      chronoState = running;
    }
    break;

  case running:
    text = "(-_-)";
    lcd.setCursor(fit(text), 1);
    lcd.print(text);
    displayTime(millis() - chronoStart);
    if (play_released)
    {
      lcd.clear();

      pausedValue = millis() - chronoStart;
      displayTime(pausedValue);

      chronoState = paused;
    }
    break;

  case paused:
    text = "(o_o)";
    lcd.setCursor(fit(text), 1);
    lcd.print(text);

    if (play_released)
    {
      chronoState = running;
      chronoStart = millis() - pausedValue;
    }
    break;
  }

  if (stop_released)
  {
    chronoState = stopped;
    displayTime(0);
  }
}

unsigned long minuteurTotal = 0;
unsigned long minuteurStart = 0;
AppState minuteurState = stopped;
void minuteur()
{
  String text;
  String progressBar;

#define amount(x) ((x) < 5 * 1000 * 60UL ? 1000 * 60UL : (x) < 30 * 60 * 1000UL ? 5 * 60 * 1000UL \
                                                                                : 15 * 60 * 1000UL)

  switch (minuteurState)
  {
  case stopped:
    text = "MINUTEUR";
    lcd.setCursor(fit(text), 1);
    lcd.print(text);
    displayTime(0);
    if (play_released)
    {
      minuteurState = running;
      minuteurStart = millis();
      minuteurTotal = amount(minuteurTotal);
    }
    break;

  case running:
    if (play_released)
    {
      minuteurTotal += amount(minuteurTotal);
    }
    else if (stop_released)
    {
      if (minuteurTotal > amount(minuteurTotal))
        minuteurTotal = max(0, minuteurTotal - amount(minuteurTotal));
      else
      {
        minuteurState = stopped;
        minuteurTotal = 0;
      }
    }

    if ((millis() - minuteurStart) > minuteurTotal)
    {
      lcd.clear();
      displayTime(0);
      minuteurState = stopped;
      return;
    }
    else
    {
      displayTime(minuteurTotal - (millis() - minuteurStart));
    }
    
    progressBar = "";
    lcd.setCursor(0, 1);
    lcd.write(byte(6));
    float ratio = (millis() - minuteurStart) / (float)minuteurTotal;
    for (size_t i = 0; i < floor(ratio * 14); i++)
    {
      lcd.write(byte(0));
    }

    float remaining = ratio * 14 - floor(ratio * 14);
    // Serial.println(remaining);
    // if (remaining < 0.25 / 5.0f)
    //   lcd.print(" ");
    if (remaining < 1 / 5.0f)
      lcd.write(byte(1));
    else if (remaining < 2 / 5.0f)
      lcd.write(byte(2));
    else if (remaining < 3 / 5.0f)
      lcd.write(byte(3));
    else if (remaining < 4 / 5.0f)
      lcd.write(byte(4));
    else if (remaining < 5 / 5.0f)
      lcd.write(byte(5));

    for (size_t i = 0; i < floor((1 - ratio) * 14); i++)
    {
      lcd.print(" ");
    }
    lcd.write(byte(7));

    break;

  default:
    break;
  }
}