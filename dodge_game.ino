/*
 * DODGE GAME TAB
 *
 * Falling blocks dodge game on the full 15x10 LED grid
 * (top half rows 0-4, bottom half rows 5-9 combined)
 *
 * - Player is a 2-pixel tall column sitting on the bottom row (row 9)
 * - Blocks are 1–3 columns wide and fall from row 0 down to row 9
 * - 1 or 2 blocks can be falling simultaneously (2nd unlocks at score 5+)
 * - LEFT / RIGHT on remote moves the player
 * - Score increases each time a block passes safely
 * - Speed increases gradually
 * - Score only shown on death: blue normally, green if new high score
 * - High score saved to EEPROM at address 456
 */

// --- DODGE EEPROM ---
#define EEPROM_DODGE_HIGHSCORE_ADDR  456   // 4 bytes (long)

// --- DODGE GRID ---
#define DODGE_COLS          15
#define DODGE_ROWS          10   // rows 0-9 top-to-bottom

// --- DODGE CONSTANTS ---
#define DODGE_PLAYER_Y       9   // bottom row of the 10-row field
#define DODGE_PLAYER_START_X 7   // start in the middle
#define DODGE_INITIAL_SPEED  160 // ms per block step
#define DODGE_MIN_SPEED       35 // fastest
#define DODGE_SPEED_DECREASE   2 // ms faster per block survived
#define DODGE_DEATH_FLASH    2500
#define DODGE_TIMEOUT_MS    15000
#define DODGE_MAX_BLOCKS      2  // max simultaneous falling blocks

// --- BLOCK STRUCT ---
struct DodgeBlock {
  int8_t x;       // leftmost column
  int8_t y;       // current row
  int8_t width;   // 1–3 columns
  bool   active;
};

// --- DODGE STATE ---
int8_t     dodgePlayerX   = DODGE_PLAYER_START_X;
int        dodgeSpeed     = DODGE_INITIAL_SPEED;
int        dodgeScore     = 0;
int        dodgeHighScore = 0;
DodgeBlock dodgeBlocks[DODGE_MAX_BLOCKS];

unsigned long dodgeFrameTimer = 0;

bool dodgeMoveLeft  = false;
bool dodgeMoveRight = false;

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Draw a pixel on the combined 15x10 dodge grid.
 * Rows 0-4 → top half,  rows 5-9 → bottom half.
 */
void dodgeDrawPixel(int x, int y, CRGB col) {
  if (x < 0 || x >= DODGE_COLS || y < 0 || y >= DODGE_ROWS) return;
  if (y < 5) {
    drawPixelToGrid(x, y,     col, true);
  } else {
    drawPixelToGrid(x, y - 5, col, false);
  }
}

// ─────────────────────────────────────────────────────────────────────────────

void loadDodgeHighScore() {
  EEPROM.get(EEPROM_DODGE_HIGHSCORE_ADDR, dodgeHighScore);
  if (dodgeHighScore < 0 || dodgeHighScore > 9999) dodgeHighScore = 0;
  Serial.print(F("Dodge High Score: "));
  Serial.println(dodgeHighScore);
}

void saveDodgeHighScore() {
  if (dodgeScore > dodgeHighScore) {
    dodgeHighScore = dodgeScore;
    EEPROM.put(EEPROM_DODGE_HIGHSCORE_ADDR, dodgeHighScore);
    Serial.print(F("New dodge high score: "));
    Serial.println(dodgeHighScore);
  }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Spawn a new block into the given slot.
 * Random width 1–3, random X position. Player should move out of the way!
 */
void spawnBlock(int slot) {
  int8_t w    = random(1, 4);           // 1, 2, or 3 wide
  int8_t maxX = DODGE_COLS - w;         // keep block within grid
  dodgeBlocks[slot].x      = random(0, maxX + 1);
  dodgeBlocks[slot].y      = 0;
  dodgeBlocks[slot].width  = w;
  dodgeBlocks[slot].active = true;
}

// ─────────────────────────────────────────────────────────────────────────────

void resetDodgeGame() {
  dodgePlayerX   = DODGE_PLAYER_START_X;
  dodgeSpeed     = DODGE_INITIAL_SPEED;
  dodgeScore     = 0;
  dodgeMoveLeft  = false;
  dodgeMoveRight = false;

  for (int i = 0; i < DODGE_MAX_BLOCKS; i++) dodgeBlocks[i].active = false;
  spawnBlock(0);   // start with one block

  dodgeFrameTimer    = millis();
  lastGameIRActivity = millis();
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Returns true if a block covers the player column.
 */
bool blockHitsPlayer(int8_t bx, int8_t bw) {
  return (dodgePlayerX >= bx && dodgePlayerX < bx + bw);
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Main dodge game loop
 */
void runDodgeGame() {
  FastLED.clear();

  // Auto-exit after timeout
  if (millis() - lastGameIRActivity > DODGE_TIMEOUT_MS) {
    currentMode = CLOCK;
    lastMin     = -1;
    return;
  }

  // ── Player movement ───────────────────────────────────────────────────────
  if (dodgeMoveLeft) {
    if (dodgePlayerX > 0) dodgePlayerX--;
    dodgeMoveLeft      = false;
    lastGameIRActivity = millis();
  }
  if (dodgeMoveRight) {
    if (dodgePlayerX < DODGE_COLS - 1) dodgePlayerX++;
    dodgeMoveRight     = false;
    lastGameIRActivity = millis();
  }

  // ── Advance blocks on frame tick ──────────────────────────────────────────
  if (millis() - dodgeFrameTimer > (unsigned long)dodgeSpeed) {
    dodgeFrameTimer = millis();

    int activeCount = 0;
    for (int i = 0; i < DODGE_MAX_BLOCKS; i++) {
      if (dodgeBlocks[i].active) activeCount++;
    }

    for (int i = 0; i < DODGE_MAX_BLOCKS; i++) {
      if (!dodgeBlocks[i].active) continue;

      dodgeBlocks[i].y++;

      if (dodgeBlocks[i].y > DODGE_PLAYER_Y) {
        // Block passed the bottom safely
        dodgeBlocks[i].active = false;
        dodgeScore++;
        if (dodgeSpeed > DODGE_MIN_SPEED) dodgeSpeed -= DODGE_SPEED_DECREASE;
        activeCount--;
      }
    }

    // How many blocks should be active?
    // Score 0–4:  always 1 block
    // Score 5–14: 50% chance of a second block
    // Score 15+:  always 2 blocks
    int targetBlocks = 1;
    if      (dodgeScore >= 15) targetBlocks = 2;
    else if (dodgeScore >=  5) targetBlocks = (random(0, 2) == 0) ? 2 : 1;

    // Fill empty slots up to target
    for (int i = 0; i < DODGE_MAX_BLOCKS && activeCount < targetBlocks; i++) {
      if (!dodgeBlocks[i].active) {
        spawnBlock(i);
        activeCount++;
      }
    }
  }

  // ── Collision detection ───────────────────────────────────────────────────
  for (int i = 0; i < DODGE_MAX_BLOCKS; i++) {
    if (!dodgeBlocks[i].active) continue;
    if (dodgeBlocks[i].y >= DODGE_PLAYER_Y - 1 &&
        blockHitsPlayer(dodgeBlocks[i].x, dodgeBlocks[i].width)) {
      handleDodgeGameOver();
      return;
    }
  }

  // ── Draw ──────────────────────────────────────────────────────────────────
  // Player: 2 pixels tall, cyan
  dodgeDrawPixel(dodgePlayerX, DODGE_PLAYER_Y,     CRGB::Cyan);
  dodgeDrawPixel(dodgePlayerX, DODGE_PLAYER_Y - 1, CRGB::Cyan);

  // Falling blocks: orange-red, drawn across their full width
  for (int i = 0; i < DODGE_MAX_BLOCKS; i++) {
    if (!dodgeBlocks[i].active) continue;
    for (int col = 0; col < dodgeBlocks[i].width; col++) {
      dodgeDrawPixel(dodgeBlocks[i].x + col, dodgeBlocks[i].y, CRGB::OrangeRed);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────

void handleDodgeGameOver() {
  saveDodgeHighScore();

  // Brief red flash at collision point
  FastLED.clear();
  dodgeDrawPixel(dodgePlayerX, DODGE_PLAYER_Y,     CRGB::Red);
  dodgeDrawPixel(dodgePlayerX, DODGE_PLAYER_Y - 1, CRGB::Red);
  for (int i = 0; i < DODGE_MAX_BLOCKS; i++) {
    if (!dodgeBlocks[i].active) continue;
    for (int col = 0; col < dodgeBlocks[i].width; col++) {
      dodgeDrawPixel(dodgeBlocks[i].x + col, dodgeBlocks[i].y, CRGB::Red);
    }
  }
  safeShow();
  delay(300);

  // Score screen
  FastLED.clear();
  bool isNewRecord = (dodgeScore >= dodgeHighScore);
  CRGB scoreColor  = isNewRecord ? CRGB::Green : CRGB::Blue;

  char scoreBuf[6];
  snprintf(scoreBuf, sizeof(scoreBuf), "%5d", dodgeScore);
  drawStaticRow(scoreBuf, true,  scoreColor);
  drawStaticRow("FALL",   false, CRGB::White);
  safeShow();
  delay(DODGE_DEATH_FLASH);

  resetDodgeGame();
  lastGameIRActivity = millis();
}
