/*
 * LED Matrix Clock with Math Equations, Dino Game, Dodge Game & Snake
 * Version 20 - IR handled by dedicated co-processor Nano
 *
 * Hardware:
 * - 154 WS2812B LEDs arranged as 15x5 grid (split into top/bottom halves)
 * - DS3231 RTC module
 * - IR co-processor Nano (pulses D2/D3/D4/D5/D7 for UP/DOWN/LEFT/RIGHT/OK)
 * - Push button on A2
 * - LDR (light sensor) on A0
 * - LED data pin 6
 *
 * Changes from v19:
 * - IRremote library completely removed from main sketch
 * - IR input now read from 5 digital pins pulsed by co-processor Nano
 * - Game launch via button combos (1=DINO, 2=DODGE, 3=SNAKE) on remote
 * - safeShow() no longer needs to wait for IR idle (no IR on this Nano)
 * - setupInputPins() called in setup()
 * - GAME_SELECT mode kept for physical button compatibility
 */

#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>
#include <EEPROM.h>

// --- PIN DEFINITIONS ---
#define LED_PIN        6
#define BTN_PIN        A2
#define LDR_PIN        A0
// IR input pins D2/D3/D4/D5/D7 are defined in input.ino

// --- LED CONFIGURATION ---
#define NUM_LEDS       154
#define COLOR_ORDER    GRB
#define CHIPSET        WS2812B
#define BRIGHTNESS_MIN 10
#define BRIGHTNESS_MAX 130

// --- GRID DIMENSIONS ---
#define GRID_WIDTH     15
#define GRID_HEIGHT    5
#define CHAR_WIDTH     3
#define TOP_HALF_START 0
#define BOT_HALF_START 75

// --- TIMING CONSTANTS ---
#define BUTTON_SHORT_PRESS_MS  1000
#define BUTTON_MEDIUM_PRESS_MS 3000
#define BUTTON_LONG_PRESS_MS   5000
#define GAME_TIMEOUT_MS        15000
#define SETTINGS_TIMEOUT_MS    30000
#define LOADING_BAR_COLS       15
#define BIRTHDAY_DISPLAY_MS    10000

// --- EEPROM ADDRESSES ---
#define EEPROM_DIFFICULTY_ADDR       0
#define EEPROM_HIGHSCORE_ADDR        1   // 4 bytes – dino high score
// EEPROM_DODGE_HIGHSCORE_ADDR      456  // defined in dodge_game.ino

// --- GLOBAL OBJECTS ---
CRGB       leds[NUM_LEDS];
RTC_DS3231 rtc;

// --- STATE MANAGEMENT ---
enum Mode {
  CLOCK,
  SETTING,
  GAME,          // dino game
  DODGE_GAME,    // dodge / falling blocks game
  GAME_SELECT,   // game selection overlay (physical button)
  SET_TIME,
  SET_DATE,
  SNAKE_GAME
};
Mode currentMode = CLOCK;

enum SettingMenu { MENU_DIFFICULTY, MENU_TIME, MENU_DATE, MENU_EXIT };
SettingMenu currentMenu = MENU_DIFFICULTY;

// Game select state  (0 = DINO, 1 = DODGE, 2 = SNAKE)
int8_t   gameSelectChoice = 0;

int8_t   settingStep  = 0;
int8_t   tempHour = 0, tempMinute = 0;
int      tempYear = 2025;
int8_t   tempMonth = 1, tempDay = 1;
int8_t   difficulty = 1;
int      score      = 0;
int      highScore  = 0;
int8_t   lastMin    = -1;

// --- BUTTON STATE ---
unsigned long btnTimer       = 0;
bool          btnActive      = false;
bool          btnMediumFired = false;

// --- IR / INPUT STATE ---
// (No IRremote library – pins read directly from co-processor)
unsigned long lastIRActivity     = 0;
unsigned long lastGameIRActivity = 0;

bool irJumpTriggered = false;

// Snake input flags
bool snakeTurnUp    = false;
bool snakeTurnDown  = false;
bool snakeTurnLeft  = false;
bool snakeTurnRight = false;

// --- SETTINGS MENU STATE ---
unsigned long lastActivity = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS_MAX);

  pinMode(BTN_PIN, INPUT_PULLUP);

  // Set up the 5 digital input pins from the IR co-processor
  setupInputPins();

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println(F("RTC not found!"));
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    safeShow();
    delay(2000);
  }

  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power – resetting to compile time"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Load difficulty
  difficulty = EEPROM.read(EEPROM_DIFFICULTY_ADDR);
  if (difficulty < 1 || difficulty > 5) {
    difficulty = 1;
    EEPROM.write(EEPROM_DIFFICULTY_ADDR, difficulty);
  }

  // Load dino high score
  EEPROM.get(EEPROM_HIGHSCORE_ADDR, highScore);
  if (highScore < 0 || highScore > 9999) highScore = 0;

  // Load dodge high score (function lives in dodge_game.ino)
  loadDodgeHighScore();

  // Load snake high score (function lives in snake_game.ino)
  loadSnakeHighScore();

  // Initialise activity timers so game timeouts don't fire immediately
  lastIRActivity     = millis();
  lastGameIRActivity = millis();

  Serial.println(F("LED Clock v20 ready"));
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  handleButton();
  handleIRInput();
  adjustBrightness();

  DateTime now = rtc.now();

  switch (currentMode) {

    case GAME:
      if (millis() - lastGameIRActivity > GAME_TIMEOUT_MS) {
        currentMode = CLOCK;
        lastMin = -1;
      }
      runDinoGame();
      break;

    case DODGE_GAME:
      runDodgeGame();
      break;

    case SNAKE_GAME:
      runSnakeGame();
      break;

    case GAME_SELECT:
      runGameSelectMenu();
      break;

    case SETTING:
      runSettingsMenu();
      break;

    case SET_TIME:
      runTimeSettingMode();
      break;

    case SET_DATE:
      runDateSettingMode();
      break;

    default: // CLOCK
      if (now.hour() == 7 && now.minute() == 30) {
        runMorningSequence(now);
      } else {
        updateClock(now);
        checkSpecialDays(now);
      }
      break;
  }

  safeShow();
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Game selection menu – shown when physical button cycles to game select.
 * UP / DOWN to cycle, OK to launch.
 */
void runGameSelectMenu() {
  FastLED.clear();

  const char* gameNames[]  = {"DINO", "FALL", "SNKE"};
  const CRGB  gameColors[] = {CRGB::Green, CRGB::OrangeRed, CRGB::Cyan};

  drawStaticRow(gameNames[gameSelectChoice], true, gameColors[gameSelectChoice]);

  int nextChoice = (gameSelectChoice + 1) % 3;
  drawStaticRow(gameNames[nextChoice], false, CRGB(50, 50, 50));

  // Auto-cancel after 10 seconds of no input
  if (millis() - lastIRActivity > 10000) {
    currentMode = CLOCK;
    lastMin     = -1;
  }
}
