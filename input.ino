/*
 * INPUT HANDLING TAB  (v20)
 *
 * IR is handled by a dedicated co-processor Nano which pulls each
 * dedicated pin LOW for 150ms to signal a button press (active LOW).
 * All input pins use INPUT_PULLUP so they sit at a known HIGH when idle.
 *
 * PIN MAPPING (Main Nano receives from IR Nano):
 *   D3   ←  RETURN  (back / cancel)
 *   D4   ←  UP
 *   D5   ←  DOWN
 *   D7   ←  LEFT
 *   D8   ←  RIGHT
 *   D9   ←  OK
 *   D10  ←  GAME1 (launch DINO)
 *   D11  ←  GAME2 (launch DODGE)
 *   D12  ←  GAME3 (launch SNAKE)
 *   D13  ←  OPTIONS (open settings)
 *
 * ACTIVE LOW: pin reads LOW = button pressed, HIGH = idle.
 *
 * CONTROL SCHEME:
 *
 *   CLOCK          GAME1→DINO  GAME2→DODGE  GAME3→SNAKE  OPTIONS→settings
 *   GAME_SELECT    UP/DOWN cycle  OK launch  RETURN cancel
 *   DINO           any button→jump  RETURN→quit
 *   DODGE          LEFT/RIGHT move  RETURN→quit
 *   SNAKE          UP/DOWN/LEFT/RIGHT steer  RETURN→quit
 *   SETTING        UP/DOWN scroll  OK select  RETURN→CLOCK
 *   SET_TIME       UP/DOWN increment  OK next/save  RETURN→SETTING
 *   SET_DATE       UP/DOWN increment  OK next/save  RETURN→SETTING
 *
 * Physical button (A2):
 *   Short press < 1s   → scroll / cycle in menus (no action in CLOCK)
 *   Hold 3s            → open settings from CLOCK
 *   Hold 5s in settings → confirm
 */

// ─── INPUT PIN DEFINITIONS ───────────────────────────────────────────────────

#define PIN_RETURN  3
#define PIN_UP      4
#define PIN_DOWN    5
#define PIN_LEFT    7
#define PIN_RIGHT   8
#define PIN_OK      9
#define PIN_GAME1   10
#define PIN_GAME2   11
#define PIN_GAME3   12
#define PIN_OPTIONS 13

// ─── PIN SETUP ───────────────────────────────────────────────────────────────

void setupInputPins() {
  pinMode(PIN_RETURN,  INPUT_PULLUP);
  pinMode(PIN_UP,      INPUT_PULLUP);
  pinMode(PIN_DOWN,    INPUT_PULLUP);
  pinMode(PIN_LEFT,    INPUT_PULLUP);
  pinMode(PIN_RIGHT,   INPUT_PULLUP);
  pinMode(PIN_OK,      INPUT_PULLUP);
  pinMode(PIN_GAME1,   INPUT_PULLUP);
  pinMode(PIN_GAME2,   INPUT_PULLUP);
  pinMode(PIN_GAME3,   INPUT_PULLUP);
  pinMode(PIN_OPTIONS, INPUT_PULLUP);
}

// ─── INPUT STATE ─────────────────────────────────────────────────────────────

static bool lastReturn  = false;
static bool lastUp      = false;
static bool lastDown    = false;
static bool lastLeft    = false;
static bool lastRight   = false;
static bool lastOk      = false;
static bool lastGame1   = false;
static bool lastGame2   = false;
static bool lastGame3   = false;
static bool lastOptions = false;

// ─── MAIN IR INPUT HANDLER ───────────────────────────────────────────────────

void handleIRInput() {

  // Active LOW: LOW = pressed, HIGH = idle
  bool pinReturn  = digitalRead(PIN_RETURN)  == LOW;
  bool pinUp      = digitalRead(PIN_UP)      == LOW;
  bool pinDown    = digitalRead(PIN_DOWN)    == LOW;
  bool pinLeft    = digitalRead(PIN_LEFT)    == LOW;
  bool pinRight   = digitalRead(PIN_RIGHT)   == LOW;
  bool pinOk      = digitalRead(PIN_OK)      == LOW;
  bool pinGame1   = digitalRead(PIN_GAME1)   == LOW;
  bool pinGame2   = digitalRead(PIN_GAME2)   == LOW;
  bool pinGame3   = digitalRead(PIN_GAME3)   == LOW;
  bool pinOptions = digitalRead(PIN_OPTIONS) == LOW;

  // Rising edges (LOW transition = button just pressed)
  bool edgeReturn  = pinReturn  && !lastReturn;
  bool edgeUp      = pinUp      && !lastUp;
  bool edgeDown    = pinDown    && !lastDown;
  bool edgeLeft    = pinLeft    && !lastLeft;
  bool edgeRight   = pinRight   && !lastRight;
  bool edgeOk      = pinOk      && !lastOk;
  bool edgeGame1   = pinGame1   && !lastGame1;
  bool edgeGame2   = pinGame2   && !lastGame2;
  bool edgeGame3   = pinGame3   && !lastGame3;
  bool edgeOptions = pinOptions && !lastOptions;

  // Activity timers
  if (pinReturn || pinUp || pinDown || pinLeft || pinRight || pinOk ||
      pinGame1  || pinGame2 || pinGame3 || pinOptions) {
    lastIRActivity     = millis();
    lastGameIRActivity = millis();
  }

  // ════════════════════════════════════════════════════════════════════════
  // MODE DISPATCH
  // ════════════════════════════════════════════════════════════════════════

  if (currentMode == CLOCK) {
    if      (edgeGame1)   launchGame(0);
    else if (edgeGame2)   launchGame(1);
    else if (edgeGame3)   launchGame(2);
    else if (edgeOptions) {
      currentMode  = SETTING;
      currentMenu  = MENU_DIFFICULTY;
      lastActivity = millis();
      Serial.println(F("Settings opened (OPTIONS)"));
    }
  }

  else if (currentMode == GAME_SELECT) {
    if      (edgeReturn) { currentMode = CLOCK; lastMin = -1; }
    else if (edgeUp)     gameSelectChoice = (gameSelectChoice + 2) % 3;
    else if (edgeDown)   gameSelectChoice = (gameSelectChoice + 1) % 3;
    else if (edgeOk)     launchSelectedGame();

    if (millis() - lastIRActivity > 10000) { currentMode = CLOCK; lastMin = -1; }
  }

  else if (currentMode == GAME) {
    if (edgeReturn) {
      currentMode = CLOCK; lastMin = -1;
      Serial.println(F("Dino quit"));
    }
    else if (edgeUp || edgeDown || edgeLeft || edgeRight || edgeOk) {
      irJumpTriggered = true;
    }
  }

  else if (currentMode == DODGE_GAME) {
    if (edgeReturn) {
      currentMode = CLOCK; lastMin = -1;
      Serial.println(F("Dodge quit"));
    }
    else {
      if (edgeLeft)  dodgeMoveLeft  = true;
      if (edgeRight) dodgeMoveRight = true;
    }
  }

  else if (currentMode == SNAKE_GAME) {
    if (edgeReturn) {
      currentMode = CLOCK; lastMin = -1;
      Serial.println(F("Snake quit"));
    }
    else {
      if (edgeUp)    snakeTurnUp    = true;
      if (edgeDown)  snakeTurnDown  = true;
      if (edgeLeft)  snakeTurnLeft  = true;
      if (edgeRight) snakeTurnRight = true;
    }
  }

  else if (currentMode == SETTING) {
    if (edgeReturn) {
      currentMode = CLOCK;
      EEPROM.write(EEPROM_DIFFICULTY_ADDR, difficulty);
      lastMin = -1;
      Serial.println(F("Settings closed (RETURN)"));
    }
    else {
      if (edgeDown) { currentMenu = (SettingMenu)((currentMenu + 1) % 4); lastActivity = millis(); }
      if (edgeUp)   { currentMenu = (SettingMenu)((currentMenu + 3) % 4); lastActivity = millis(); }
      if (edgeOk)   { selectMenuItem(); lastActivity = millis(); }
    }
  }

  else if (currentMode == SET_TIME) {
    if (edgeReturn) { currentMode = SETTING; lastActivity = millis(); }
    else {
      if (edgeUp || edgeDown) { incrementTimeField(); lastActivity = millis(); }
      if (edgeOk) {
        if (settingStep < 1) settingStep++;
        else { saveTime(); currentMode = CLOCK; lastMin = -1; }
        lastActivity = millis();
      }
    }
  }

  else if (currentMode == SET_DATE) {
    if (edgeReturn) { currentMode = SETTING; lastActivity = millis(); }
    else {
      if (edgeUp || edgeDown) { incrementDateField(); lastActivity = millis(); }
      if (edgeOk) {
        if (settingStep < 2) settingStep++;
        else { saveDate(); currentMode = CLOCK; lastMin = -1; }
        lastActivity = millis();
      }
    }
  }

  // ── Save state for next tick ─────────────────────────────────────────────
  lastReturn  = pinReturn;
  lastUp      = pinUp;
  lastDown    = pinDown;
  lastLeft    = pinLeft;
  lastRight   = pinRight;
  lastOk      = pinOk;
  lastGame1   = pinGame1;
  lastGame2   = pinGame2;
  lastGame3   = pinGame3;
  lastOptions = pinOptions;
}

// ─── GAME LAUNCH ─────────────────────────────────────────────────────────────

void launchGame(uint8_t choice) {
  if (choice == 0) {
    currentMode        = GAME;
    score              = 0;
    gameSpeed          = INITIAL_GAME_SPEED;
    obstacleX          = OBSTACLE_START_X;
    playerY            = PLAYER_GROUND_Y;
    isJumping          = false;
    lastIRActivity     = millis();
    lastGameIRActivity = millis();
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    safeShow(); delay(200);
    Serial.println(F("DINO launched"));
  }
  else if (choice == 1) {
    currentMode        = DODGE_GAME;
    lastGameIRActivity = millis();
    resetDodgeGame();
    fill_solid(leds, NUM_LEDS, CRGB::OrangeRed);
    safeShow(); delay(200);
    Serial.println(F("DODGE launched"));
  }
  else {
    currentMode        = SNAKE_GAME;
    lastGameIRActivity = millis();
    resetSnake();
    fill_solid(leds, NUM_LEDS, CRGB::Cyan);
    safeShow(); delay(200);
    Serial.println(F("SNAKE launched"));
  }
}

void launchSelectedGame() {
  launchGame(gameSelectChoice);
}

// ─── PHYSICAL BUTTON ─────────────────────────────────────────────────────────

void handleButton() {
  bool btnPressed = !digitalRead(BTN_PIN);

  if (btnPressed && !btnActive) {
    btnTimer = millis(); btnActive = true; btnMediumFired = false;
  }

  if (btnActive && btnPressed) {
    if (!btnMediumFired && millis() - btnTimer >= BUTTON_MEDIUM_PRESS_MS) {
      btnMediumFired = true;
      handleMediumHold();
    }
  }

  if (btnActive && !btnPressed) {
    unsigned long dur = millis() - btnTimer;
    btnActive = false;

    if (btnMediumFired) {
      if (dur >= BUTTON_LONG_PRESS_MS &&
          (currentMode == SETTING || currentMode == SET_TIME ||
           currentMode == SET_DATE)) {
        handleLongPress();
      }
    }
    else if (dur < BUTTON_SHORT_PRESS_MS) {
      handleShortPress();
    }
  }
}

void handleShortPress() {
  lastActivity = millis();
  switch (currentMode) {
    case CLOCK:        /* remote-only – do nothing */                       break;
    case SETTING:      currentMenu = (SettingMenu)((currentMenu + 1) % 4); break;
    case SET_TIME:     incrementTimeField();                                 break;
    case SET_DATE:     incrementDateField();                                 break;
    case GAME_SELECT:  gameSelectChoice = (gameSelectChoice + 1) % 3;       break;
    default: break;
  }
}

void handleMediumHold() {
  if (currentMode == CLOCK) {
    currentMode  = SETTING;
    currentMenu  = MENU_DIFFICULTY;
    lastActivity = millis();
    Serial.println(F("Settings opened (button hold)"));
  }
}

void handleLongPress() {
  lastActivity = millis();
  switch (currentMode) {
    case SETTING:      selectMenuItem(); break;
    case SET_TIME:
      if (settingStep == 0) settingStep = 1;
      else { saveTime(); currentMode = CLOCK; lastMin = -1; }
      break;
    case SET_DATE:
      if (settingStep < 2) settingStep++;
      else { saveDate(); currentMode = CLOCK; lastMin = -1; }
      break;
    case GAME_SELECT:  launchSelectedGame();   break;
  }
}

// ─── CLOCK DISPLAY HELPERS ───────────────────────────────────────────────────

void runMorningSequence(DateTime now) {
  FastLED.clear();
  uint8_t hue = beat8(20);
  const char* top = (now.second() / 10) % 2 == 0 ? "MRGN" : "<PAPA";
  const char* names[] = {"test1", "£Test2", "also", "BAH"};
  drawStaticRow(top,                       true,  CHSV(hue,       255, 255));
  drawStaticRow(names[(now.second()/7)%4], false, CHSV(hue + 128, 255, 255));
}

void checkSpecialDays(DateTime now) {
  // Nothing special outside minute 0 and minute 30
  if (now.minute() != 0 && now.minute() != 30) return;
  // Birthdays and other special events only at the top of the hour
  if (checkAndDisplayBirthdays(now)) return;

  const char* evt = nullptr;
  if      (now.month() == 3  && now.day() == 15) evt = "NAME";
  else if (now.month() == 1  && now.day() == 1)  evt = "YEAR";
  else if (now.month() == 4  && now.day() == 6)  evt = "ESTR";
  else if (now.month() == 7  && now.day() == 21) evt = "BELG";
  else if (now.month() == 12 && now.day() == 25) evt = "XMAS";

  if (evt != nullptr) {
    FastLED.clear();
    drawStaticRow("HAPPY", true,  CRGB::Gold);
    drawStaticRow(evt,     false, CRGB::Gold);
  }
}

void updateClock(DateTime now) {
  if (now.minute() != lastMin) {
    FastLED.clear();
    drawStaticRow(getEquation(now.hour(),   difficulty), true,  CRGB::White);
    drawStaticRow(getEquation(now.minute(), difficulty), false, CRGB::White);
    leds[150] = leds[151] = leds[152] = leds[153] = CRGB::White;
    lastMin = now.minute();
  }
}

// ─── SETTINGS MENU ───────────────────────────────────────────────────────────

void runSettingsMenu() {
  FastLED.clear();
  switch (currentMenu) {
    case MENU_DIFFICULTY: {
      const CRGB dc[] = { CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Orange, CRGB::Red };
      char buf[2]; snprintf(buf, sizeof(buf), "%d", difficulty);
      drawStaticRow("LVL", true,  CRGB::White);
      drawStaticRow(buf,   false, dc[difficulty - 1]);
      break;
    }
    case MENU_TIME:     drawStaticRow("TIME", true, CRGB::Cyan);    drawStaticRow("SET", false, CRGB::Cyan);    break;
    case MENU_DATE:     drawStaticRow("DATE", true, CRGB::Magenta); drawStaticRow("SET", false, CRGB::Magenta); break;
    case MENU_EXIT:     drawStaticRow("EXIT", true, CRGB::Red);     drawStaticRow("SAVE",false, CRGB::Red);     break;
  }
  if (millis() - lastActivity > SETTINGS_TIMEOUT_MS) {
    currentMode = CLOCK; EEPROM.write(EEPROM_DIFFICULTY_ADDR, difficulty); lastMin = -1;
  }
}

void selectMenuItem() {
  DateTime now = rtc.now();
  switch (currentMenu) {
    case MENU_DIFFICULTY:
      difficulty = (difficulty % 5) + 1;
      EEPROM.write(EEPROM_DIFFICULTY_ADDR, difficulty);
      break;
    case MENU_TIME:
      currentMode = SET_TIME; settingStep = 0;
      tempHour = now.hour(); tempMinute = now.minute();
      break;
    case MENU_DATE:
      currentMode = SET_DATE; settingStep = 0;
      tempYear = now.year(); tempMonth = now.month(); tempDay = now.day();
      break;
    case MENU_EXIT:
      currentMode = CLOCK; EEPROM.write(EEPROM_DIFFICULTY_ADDR, difficulty); lastMin = -1;
      break;
  }
}

// ─── TIME SETTING ─────────────────────────────────────────────────────────────

void runTimeSettingMode() {
  FastLED.clear();
  char h[3], m[3];
  snprintf(h, sizeof(h), "%02d", tempHour);
  snprintf(m, sizeof(m), "%02d", tempMinute);
  drawStaticRow(h, true,  settingStep == 0 ? CRGB::Green : CRGB::White);
  drawStaticRow(m, false, settingStep == 1 ? CRGB::Green : CRGB::White);
  if ((millis() / 500) % 2 == 0)
    leds[150] = leds[151] = leds[152] = leds[153] = CRGB::White;
}

void incrementTimeField() {
  if (settingStep == 0) tempHour   = (tempHour   + 1) % 24;
  else                  tempMinute = (tempMinute + 1) % 60;
}

void saveTime() {
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), tempHour, tempMinute, 0));
}

// ─── DATE SETTING ─────────────────────────────────────────────────────────────

void runDateSettingMode() {
  FastLED.clear();
  char d[6];
  if      (settingStep == 0) snprintf(d, sizeof(d), "%d",   tempYear);
  else if (settingStep == 1) snprintf(d, sizeof(d), "%02d", tempMonth);
  else                       snprintf(d, sizeof(d), "%02d", tempDay);
  drawStaticRow("DATE", true,  CRGB::Cyan);
  drawStaticRow(d,      false, CRGB::Green);
}

void incrementDateField() {
  if      (settingStep == 0) { tempYear++;  if (tempYear  > 2099) tempYear  = 2000; }
  else if (settingStep == 1) { tempMonth++; if (tempMonth > 12)   tempMonth = 1;    }
  else                       { tempDay++;   if (tempDay   > 31)   tempDay   = 1;    }
}

void saveDate() {
  DateTime now = rtc.now();
  rtc.adjust(DateTime(tempYear, tempMonth, tempDay, now.hour(), now.minute(), now.second()));
}


