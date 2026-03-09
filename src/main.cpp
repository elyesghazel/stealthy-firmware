#include <Arduino.h>
#include <SPI.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// SPI pins
#define EPD_MOSI 4
#define EPD_SCK  5
#define EPD_CS   6
#define EPD_DC   7
#define EPD_RST  15
#define EPD_BUSY 16

// buttons
#define BTN_UP 10
#define BTN_DOWN 11
#define BTN_SELECT 12
#define BTN_BACK 13

#define displayRotation 3

// display
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

enum ButtonEvent {
  BTN_NONE,
  BTN_UP_EVENT,
  BTN_DOWN_EVENT,
  BTN_SELECT_EVENT,
  BTN_BACK_EVENT
};

enum Screen {
  SCREEN_MENU,
  SCREEN_INFO,
  SCREEN_SETTINGS
};

Screen currentScreen = SCREEN_MENU;
Screen lastScreen = SCREEN_MENU;

int menuIndex = 0;
bool screenDirty = true;
bool firstDraw = true;

void initButtons()
{
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
}

ButtonEvent readButtons()
{
  static uint32_t lastPress = 0;

  if (millis() - lastPress < 150) return BTN_NONE;

  if (digitalRead(BTN_UP) == LOW) {
    lastPress = millis();
    return BTN_UP_EVENT;
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    lastPress = millis();
    return BTN_DOWN_EVENT;
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    lastPress = millis();
    return BTN_SELECT_EVENT;
  }

  if (digitalRead(BTN_BACK) == LOW) {
    lastPress = millis();
    return BTN_BACK_EVENT;
  }

  return BTN_NONE;
}

void handleButtons()
{
  ButtonEvent btn = readButtons();

  if (btn == BTN_NONE) return;

  switch(currentScreen)
  {
    case SCREEN_MENU:

      if(btn == BTN_UP_EVENT) {
        menuIndex--;
        screenDirty = true;
      }

      if(btn == BTN_DOWN_EVENT) {
        menuIndex++;
        screenDirty = true;
      }

      if(btn == BTN_SELECT_EVENT)
      {
        if(menuIndex == 0) currentScreen = SCREEN_INFO;
        if(menuIndex == 1) currentScreen = SCREEN_SETTINGS;

        screenDirty = true;
      }

      break;

    case SCREEN_INFO:

      if(btn == BTN_BACK_EVENT) {
        currentScreen = SCREEN_MENU;
        screenDirty = true;
      }

      break;

    case SCREEN_SETTINGS:

      if(btn == BTN_BACK_EVENT) {
        currentScreen = SCREEN_MENU;
        screenDirty = true;
      }

      break;
  }

  if(menuIndex < 0) menuIndex = 1;
  if(menuIndex > 1) menuIndex = 0;
}

void drawMenuFull()
{
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(10,40);
    if(menuIndex == 0) display.print("> ");
    else display.print("  ");
    display.println("Info");

    display.setCursor(10,80);
    if(menuIndex == 1) display.print("> ");
    else display.print("  ");
    display.println("Settings");

  } while(display.nextPage());
}

void drawMenuPartial()
{
  display.setPartialWindow(0, 20, 250, 100);

  display.firstPage();
  do {

    display.fillRect(0,20,250,100,GxEPD_WHITE);

    display.setCursor(10,40);
    if(menuIndex == 0) display.print("> ");
    else display.print("  ");
    display.println("Info");

    display.setCursor(10,80);
    if(menuIndex == 1) display.print("> ");
    else display.print("  ");
    display.println("Settings");

  } while(display.nextPage());
}


void drawMenu()
{
  if(firstDraw)
  {
    drawMenuFull();
    firstDraw = false;
  }
  else
  {
    drawMenuPartial();
  }
}

void drawInfo()
{
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(10,40);
    display.println("Info Screen");

    display.setCursor(10,80);
    display.println("Press BACK");

  } while (display.nextPage());
}

void drawSettings()
{
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(10,40);
    display.println("Settings");

    display.setCursor(10,80);
    display.println("Press BACK");

  } while (display.nextPage());
}

void drawScreen()
{
  if(!screenDirty) return;

  screenDirty = false;

  switch(currentScreen)
  {
    case SCREEN_MENU:
      drawMenu();
      break;

    case SCREEN_INFO:
      drawInfo();
      break;

    case SCREEN_SETTINGS:
      drawSettings();
      break;
  }
}

void setup()
{
  Serial.begin(115200);

  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

  initButtons();

  display.init(115200);
  display.setRotation(displayRotation);

  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  screenDirty = true;
}

void loop()
{
  handleButtons();
  drawScreen();
}