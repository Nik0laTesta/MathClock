/*
 * SNAKE GAME TAB
 *
 * Classic Snake on the 15x5 LED grid (both halves combined = 15 wide, 10 tall).
 *
 * The grid used by snake spans BOTH the top and bottom halves of the display,
 * giving a 15 x 10 play field. We use a helper snakeDrawPixel() that routes
 * to the correct half automatically.
 *
 * Controls (Samsung remote):
 *   UP    → turn up
 *   DOWN  → turn down
 *   LEFT  → turn left
 *   RIGHT → turn right
 *
 * Rules:
 *   - Snake starts 3 segments long in the centre, moving right
 *   - Eating a food pixel grows the snake by 1 and scores +1
 *   - Hitting a wall or yourself ends the game
 *   - Speed increases every 5 foods eaten
 *   - High score saved to EEPROM
 *
 * On death:
 *   - Snake flashes red
 *   - Score shown on top half in BLUE, or GREEN if new high score
 *   - "SNKE" label on bottom half
 *   - After 3 seconds, resets and keeps playing
 */

// --- SNAKE EEPROM ---
// Placed after dodge high score (456–459)
#define EEPROM_SNAKE_HIGHSCORE_ADDR  460   // 4 bytes (long)

// --- SNAKE GRID ---
// Combines top half (rows 0-4) and bottom half (rows 5-9) into one 15x10 field
#define SNAKE_COLS        15
#define SNAKE_ROWS        10
#define SNAKE_MAX_LENGTH  24   // 24 segments max, saves RAM (hard to fill anyway)

// --- SNAKE TIMING ---
#define SNAKE_INITIAL_SPEED  300   // ms per move
#define SNAKE_MIN_SPEED       80   // fastest
#define SNAKE_SPEED_STEP       3   // ms faster per food eaten (gradual)
#define SNAKE_DEATH_FLASH    3000  // ms to show death screen

// --- SNAKE DIRECTIONS ---
#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

// --- SNAKE STATE ---
struct SnakeSegment {
  int8_t x;
  int8_t y;
};

SnakeSegment snakeBody[SNAKE_MAX_LENGTH];
int8_t       snakeLength   = 0;
int8_t       snakeDir      = DIR_RIGHT;
int8_t       snakePendingDir = DIR_RIGHT;
int8_t       foodX         = 0;
int8_t       foodY         = 0;
int          snakeSpeed    = SNAKE_INITIAL_SPEED;
int          snakeScore    = 0;
int          snakeHighScore = 0;
unsigned long snakeMoveTimer = 0;
// Input flags set by IR handler (declared in math_clock_v15.ino)

// --- SNAKE TIMEOUT ---
#define SNAKE_TIMEOUT_MS 20000

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Load snake high score from EEPROM
 */
void loadSnakeHighScore() {
  EEPROM.get(EEPROM_SNAKE_HIGHSCORE_ADDR, snakeHighScore);
  if (snakeHighScore < 0 || snakeHighScore > 99999) snakeHighScore = 0;
  Serial.print(F("Snake High Score: "));
  Serial.println(snakeHighScore);
}

/**
 * Save snake high score to EEPROM only if improved
 */
void saveSnakeHighScore() {
  if (snakeScore > snakeHighScore) {
    snakeHighScore = snakeScore;
    EEPROM.put(EEPROM_SNAKE_HIGHSCORE_ADDR, snakeHighScore);
    Serial.print(F("New snake high score: "));
    Serial.println(snakeHighScore);
  }
}

/**
 * Place food at a random cell not occupied by the snake
 */
void spawnFood() {
  // Collect free cells and pick one at random
  // Simple retry approach: try random positions until we find a free one
  int8_t fx, fy;
  int    tries = 0;
  do {
    fx = random(0, SNAKE_COLS);
    fy = random(0, SNAKE_ROWS);
    tries++;
    bool occupied = false;
    for (int i = 0; i < snakeLength; i++) {
      if (snakeBody[i].x == fx && snakeBody[i].y == fy) {
        occupied = true;
        break;
      }
    }
    if (!occupied) break;
  } while (tries < 200);

  foodX = fx;
  foodY = fy;
}

/**
 * Reset snake to starting state
 */
void resetSnake() {
  snakeLength    = 3;
  snakeDir       = DIR_RIGHT;
  snakePendingDir = DIR_RIGHT;
  snakeSpeed     = SNAKE_INITIAL_SPEED;
  snakeScore     = 0;
  snakeMoveTimer = millis();
  lastGameIRActivity = millis();

  // Start in the centre-left area
  snakeBody[0] = {7, 5};   // head
  snakeBody[1] = {6, 5};
  snakeBody[2] = {5, 5};   // tail

  // Clear direction flags
  snakeTurnUp = snakeTurnDown = snakeTurnLeft = snakeTurnRight = false;

  spawnFood();
}

/**
 * Draw a pixel on the combined 15x10 snake grid.
 * Rows 0-4  → top half
 * Rows 5-9  → bottom half (row 5 = row 0 of bottom half, etc.)
 */
void snakeDrawPixel(int x, int y, CRGB col) {
  if (x < 0 || x >= SNAKE_COLS || y < 0 || y >= SNAKE_ROWS) return;
  if (y < 5) {
    drawPixelToGrid(x, y,     col, true);   // top half
  } else {
    drawPixelToGrid(x, y - 5, col, false);  // bottom half
  }
}

/**
 * Main snake game loop
 */
void runSnakeGame() {
  FastLED.clear();

  // Auto-exit if no input for a while
  if (millis() - lastGameIRActivity > SNAKE_TIMEOUT_MS) {
    currentMode = CLOCK;
    lastMin     = -1;
    return;
  }

  // ── Apply buffered direction input ──────────────────────────────────────
  // Prevent 180-degree reversal
  if (snakeTurnUp    && snakeDir != DIR_DOWN)  snakePendingDir = DIR_UP;
  if (snakeTurnDown  && snakeDir != DIR_UP)    snakePendingDir = DIR_DOWN;
  if (snakeTurnLeft  && snakeDir != DIR_RIGHT) snakePendingDir = DIR_LEFT;
  if (snakeTurnRight && snakeDir != DIR_LEFT)  snakePendingDir = DIR_RIGHT;
  snakeTurnUp = snakeTurnDown = snakeTurnLeft = snakeTurnRight = false;

  // ── Move snake on timer ─────────────────────────────────────────────────
  if (millis() - snakeMoveTimer >= (unsigned long)snakeSpeed) {
    snakeMoveTimer = millis();
    snakeDir = snakePendingDir;

    // Calculate new head position
    int8_t newX = snakeBody[0].x;
    int8_t newY = snakeBody[0].y;
    if (snakeDir == DIR_UP)    newY--;
    if (snakeDir == DIR_DOWN)  newY++;
    if (snakeDir == DIR_LEFT)  newX--;
    if (snakeDir == DIR_RIGHT) newX++;

    // ── Wall collision ────────────────────────────────────────────────────
    if (newX < 0 || newX >= SNAKE_COLS || newY < 0 || newY >= SNAKE_ROWS) {
      handleSnakeDeath();
      return;
    }

    // ── Self collision ────────────────────────────────────────────────────
    // Skip last segment (it will move away this tick)
    for (int i = 0; i < snakeLength - 1; i++) {
      if (snakeBody[i].x == newX && snakeBody[i].y == newY) {
        handleSnakeDeath();
        return;
      }
    }

    // ── Did we eat food? ──────────────────────────────────────────────────
    bool ate = (newX == foodX && newY == foodY);

    // Shift body: move everything one step back (drop tail unless eating)
    if (ate) {
      // Grow: make room for new head without dropping tail
      if (snakeLength < SNAKE_MAX_LENGTH) snakeLength++;
    }
    // Shift all segments back one position
    for (int i = snakeLength - 1; i > 0; i--) {
      snakeBody[i] = snakeBody[i - 1];
    }
    // Place new head
    snakeBody[0] = {newX, newY};

    if (ate) {
      snakeScore++;
      // Speed up every 5 foods
      if (snakeScore % 5 == 0 && snakeSpeed > SNAKE_MIN_SPEED) {
        snakeSpeed -= SNAKE_SPEED_STEP * 5;
        if (snakeSpeed < SNAKE_MIN_SPEED) snakeSpeed = SNAKE_MIN_SPEED;
      }
      spawnFood();
    }
  }

  // ── Draw food ───────────────────────────────────────────────────────────
  // Blink food to make it visible (fast blink)
  if ((millis() / 200) % 2 == 0) {
    snakeDrawPixel(foodX, foodY, CRGB::Red);
  }

  // ── Draw snake ──────────────────────────────────────────────────────────
  // Head is bright green, body is darker green
  for (int i = snakeLength - 1; i >= 0; i--) {
    CRGB col = (i == 0) ? CRGB::Green : CRGB(0, 80, 0);
    snakeDrawPixel(snakeBody[i].x, snakeBody[i].y, col);
  }
}

/**
 * Handle snake death
 */
void handleSnakeDeath() {
  saveSnakeHighScore();

  // Flash snake red
  for (int flash = 0; flash < 3; flash++) {
    FastLED.clear();
    for (int i = 0; i < snakeLength; i++) {
      snakeDrawPixel(snakeBody[i].x, snakeBody[i].y, CRGB::Red);
    }
    safeShow();
    delay(200);
    FastLED.clear();
    safeShow();
    delay(150);
  }

  // Show score screen
  bool isNewRecord = (snakeScore >= snakeHighScore && snakeScore > 0);
  CRGB scoreColor  = isNewRecord ? CRGB::Green : CRGB::Blue;

  FastLED.clear();
  char scoreBuf[6];
  snprintf(scoreBuf, sizeof(scoreBuf), "%5d", snakeScore);
  drawStaticRow(scoreBuf, true,  scoreColor);
  drawStaticRow("SNKE",   false, CRGB::White);
  safeShow();

  delay(SNAKE_DEATH_FLASH);

  // Reset and keep playing
  resetSnake();
  lastGameIRActivity = millis();
}
