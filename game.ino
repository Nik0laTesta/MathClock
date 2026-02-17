/*
 * DINO GAME TAB
 * 
 * Simple Chrome-style dino runner game
 * - Player jumps over ground obstacles (red) or ducks under air obstacles (purple)
 * - Score increases over time, speed increases gradually
 * - Game ends on collision
 */

// --- GAME CONSTANTS ---
#define JUMP_DURATION_MS     450
#define INITIAL_GAME_SPEED   100   // milliseconds per frame
#define MIN_GAME_SPEED       40    // fastest the game can get
#define SPEED_DECREASE       1     // speed reduction per obstacle cleared
#define OBSTACLE_START_X     14    // right edge of grid
#define PLAYER_X             1     // player position from left
#define PLAYER_GROUND_Y      3     // player Y when on ground
#define PLAYER_AIR_Y         0     // player Y when jumping
#define GROUND_OBSTACLE      0     // obstacle type on ground
#define AIR_OBSTACLE         1     // obstacle type in air
#define COLLISION_FLASH_MS   3000

// --- GAME STATE ---
int8_t playerY    = PLAYER_GROUND_Y;
bool   isJumping  = false;
unsigned long jumpTimer = 0;
int8_t obstacleX  = OBSTACLE_START_X;
int8_t obstacleType = GROUND_OBSTACLE;
int    gameSpeed  = INITIAL_GAME_SPEED;
unsigned long frameTimer = 0;

/**
 * Main game loop - runs when currentMode == GAME
 */
void runDinoGame() {
  FastLED.clear();

  // Handle jump input from IR remote
  if (irJumpTriggered && !isJumping) { 
    isJumping = true; 
    jumpTimer = millis(); 
    irJumpTriggered = false; 
  }
  
  // End jump after duration
  if (isJumping && (millis() - jumpTimer > JUMP_DURATION_MS)) {
    isJumping = false;
  }
  
  // Update player Y position
  playerY = isJumping ? PLAYER_AIR_Y : PLAYER_GROUND_Y;

  // Move obstacle and spawn new ones
  if (millis() - frameTimer > gameSpeed) {
    obstacleX--;
    
    // Spawn new obstacle when old one goes off screen
    if (obstacleX < 0) { 
      obstacleX = OBSTACLE_START_X;
      obstacleType = random(0, 2); // Random ground or air obstacle
      score++;
      
      // Gradually increase difficulty (decrease speed)
      if (gameSpeed > MIN_GAME_SPEED) {
        gameSpeed -= SPEED_DECREASE;
      }
    }
    frameTimer = millis();
  }

  // Collision detection
  if (obstacleX == PLAYER_X) {
    bool collision = false;
    
    if (obstacleType == GROUND_OBSTACLE && !isJumping) {
      collision = true; // Hit ground obstacle while on ground
    } else if (obstacleType == AIR_OBSTACLE && isJumping) {
      collision = true; // Hit air obstacle while jumping
    }
    
    if (collision) {
      handleGameOver();
      return; // Exit game loop after game over
    }
  }

  // Draw game elements
  drawGameScreen();
}

/**
 * Handle game over sequence
 */
void handleGameOver() {
  saveHighScore();
  
  // Flash red border
  FastLED.clear();
  
  // Top and bottom borders
  for (int x = 0; x < GRID_WIDTH; x++) { 
    drawPixelToGrid(x, 0, CRGB::Red, true);            // Top row (top half)
    drawPixelToGrid(x, GRID_HEIGHT - 1, CRGB::Red, true); // Bottom row (top half)
    drawPixelToGrid(x, 0, CRGB::Red, false);           // Top row (bottom half)
    drawPixelToGrid(x, GRID_HEIGHT - 1, CRGB::Red, false); // Bottom row (bottom half)
  }
  
  // Left and right borders
  for (int y = 0; y < GRID_HEIGHT; y++) { 
    drawPixelToGrid(0, y, CRGB::Red, true);               // Left edge (top half)
    drawPixelToGrid(GRID_WIDTH - 1, y, CRGB::Red, true);  // Right edge (top half)
    drawPixelToGrid(0, y, CRGB::Red, false);              // Left edge (bottom half)
    drawPixelToGrid(GRID_WIDTH - 1, y, CRGB::Red, false); // Right edge (bottom half)
  }
  
  safeShow();
  delay(COLLISION_FLASH_MS);
  
  // Reset game state
  score = 0;
  gameSpeed = INITIAL_GAME_SPEED;
  obstacleX = OBSTACLE_START_X;
  obstacleType = GROUND_OBSTACLE;
  playerY = PLAYER_GROUND_Y;
  isJumping = false;
  
  // Keep game active
  lastIRActivity = millis();
}

/**
 * Draw all game elements on screen
 */
void drawGameScreen() {
  // Draw score on top row (5 digits, padded with spaces)
  char scoreStr[6];
  snprintf(scoreStr, sizeof(scoreStr), "%5d", score);
  CRGB scoreColor = (score > highScore) ? CRGB::Green : CRGB::Blue;
  drawStaticRow(scoreStr, true, scoreColor);
  
  // Draw player (vertical bar)
  drawPixelChar('|', PLAYER_X, CRGB::Green, false, playerY);
  
  // Draw obstacle
  CRGB obstacleColor = (obstacleType == GROUND_OBSTACLE) ? CRGB::Red : CRGB::Purple;
  int obstacleY = (obstacleType == GROUND_OBSTACLE) ? GRID_HEIGHT - 1 : 0;
  drawPixelToGrid(obstacleX, obstacleY, obstacleColor, false);
}

/**
 * Save high score to EEPROM only if improved
 * Note: EEPROM has limited write cycles (~100k), so only write when necessary
 */
void saveHighScore() {
  if (score > highScore) {
    highScore = score;
    EEPROM.put(EEPROM_HIGHSCORE_ADDR, highScore);
    Serial.print(F("New high score: "));
    Serial.println(highScore);
  }
}
