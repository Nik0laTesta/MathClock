/*
 * MATH EQUATIONS TAB
 * 
 * Generates random math equations that equal a target number
 * Difficulty levels control equation complexity
 */

/**
 * Generate a math equation string that equals the target value
 * Tries multiple random approaches until finding one that fits in 5 characters
 * 
 * Difficulty levels:
 * 1: Simple addition only (a+b)
 * 2: Addition and subtraction with low numbers (a+b, a-b)
 * 3: Addition and subtraction with high numbers (a+b, a-b with larger values)
 * 4: High numbers with multiplication mixed in (a+b, a-b, axb)
 * 5: Maximum complexity - any operations in any order following proper order of operations
 *    (axb+c, a/b-c, a+bxc, etc.)
 * 
 * @param target The number the equation should equal (0-59 for time)
 * @param lvl Difficulty level (1-5)
 * @return const char* to static buffer with equation (max 5 chars)
 */
// Result buffer - static so it persists after return, no heap allocation
static char _eqBuf[6];
static char _tmp[12];   // scratch buffer to test length before committing
// Write to _tmp first; only copy to _eqBuf if the result actually fits
#define TRY(fmt, ...) snprintf(_tmp, sizeof(_tmp), fmt, ##__VA_ARGS__); \
  if (strlen(_tmp) <= 5) { strcpy(_eqBuf, _tmp); }

const char* getEquation(int target, int lvl) {
  // Special case for zero
  if (target == 0) {
    return "0+0";  // string literal, no heap
  }
  
  // Try up to 150 times to generate valid equation
  for (int attempt = 0; attempt < 150; attempt++) {
    int a, b, c;
    _eqBuf[0] = '\0';  // clear result
    
    switch (lvl) {
      case 1: // Level 1: Simple addition only
        {
          // Split target into two random addends
          a = random(0, target + 1);
          b = target - a;
          TRY("%d+%d", a, b);
        }
        break;
        
      case 2: // Level 2: Addition and subtraction with low numbers (max ~50)
        {
          if (random(0, 2) == 0) {
            // Addition: a + b = target
            a = random(0, min(target + 1, 50));
            b = target - a;
            if (b >= 0 && b <= 50) {
              TRY("%d+%d", a, b);
            }
          } else {
            // Subtraction: a - b = target
            a = random(target, min(target + 50, 99));
            b = a - target;
            TRY("%d-%d", a, b);
          }
        }
        break;
        
      case 3: // Level 3: Addition and subtraction with HIGH numbers, plus division
        {
          int opChoice = random(0, 3);
          
          if (opChoice == 0) {
            // Addition: a + b = target (max 99 to fit in 5 chars: "99+59")
            a = random(0, min(100, target + 1));
            b = target - a;
            if (b >= 0 && b <= 99) {
              TRY("%d+%d", a, b);
            }
          } else if (opChoice == 1) {
            // Subtraction: a - b = target (max 99 to fit: "99-59")
            a = random(target, 100);
            b = a - target;
            if (b <= 99) {
              TRY("%d-%d", a, b);
            }
          } else {
            // Division: a / b = target (allows 3-digit numbers: "333/9")
            b = random(2, 9);
            a = target * b;
            if (a <= 999) {
              TRY("%d/%d", a, b);
            }
          }
        }
        break;
        
      case 4: // Level 4: High numbers + multiplication
        {
          int opChoice = random(0, 3);
          
          if (opChoice == 0 && target >= 2) {
            // Multiplication: a x b = target
            for (int factor = 9; factor >= 2; factor--) {
              if (target % factor == 0) {
                int quotient = target / factor;
                TRY("%dx%d", factor, quotient);
                break;
              }
            }
          } else if (opChoice == 1) {
            // Addition with high numbers (max 99: "99+59")
            a = random(0, min(100, target + 1));
            b = target - a;
            if (b >= 0 && b <= 99) {
              TRY("%d+%d", a, b);
            }
          } else {
            // Subtraction with high numbers (max 99: "99-59")
            a = random(target, 100);
            b = a - target;
            if (b <= 99) {
              TRY("%d-%d", a, b);
            }
          }
        }
        break;
        
      case 5: // Level 5: Maximum complexity with proper order of operations
        {
          int pattern = random(0, 6);
          
          if (pattern == 0 && target >= 2) {
            // Pattern: axb+c = target → axb = target-c (order: multiply first, then add)
            c = random(1, target);
            int product = target - c;
            if (product > 0) {
              for (int factor = 9; factor >= 2; factor--) {
                if (product % factor == 0) {
                  int quotient = product / factor;
                  if (quotient <= 9) {
                    TRY("%dx%d+%d", factor, quotient, c);
                    break;
                  }
                }
              }
            }
          } 
          else if (pattern == 1 && target >= 2) {
            // Pattern: axb-c = target → axb = target+c (order: multiply first, then subtract)
            c = random(1, 20);
            int product = target + c;
            for (int factor = 9; factor >= 2; factor--) {
              if (product % factor == 0) {
                int quotient = product / factor;
                if (quotient <= 9) {
                  TRY("%dx%d-%d", factor, quotient, c);
                  break;
                }
              }
            }
          }
          else if (pattern == 2 && target >= 2) {
            // Pattern: a/b+c = target → a/b = target-c (order: divide first, then add)
            c = random(1, target);
            int quotient = target - c;
            if (quotient > 0) {
              b = random(2, 9);
              a = quotient * b;
              if (a <= 99) {
                TRY("%d/%d+%d", a, b, c);
              }
            }
          }
          else if (pattern == 3 && target >= 2) {
            // Pattern: a/b-c = target → a/b = target+c (order: divide first, then subtract)
            c = random(1, 30);
            int quotient = target + c;
            b = random(2, 9);
            a = quotient * b;
            if (a <= 99) {
              TRY("%d/%d-%d", a, b, c);
            }
          }
          else if (pattern == 4 && target >= 2) {
            // Pattern: a+bxc = target → bxc = target-a (order: multiply first, then add)
            a = random(1, target);
            int product = target - a;
            if (product > 0) {
              for (int factor = 9; factor >= 2; factor--) {
                if (product % factor == 0) {
                  int quotient = product / factor;
                  if (quotient <= 9) {
                    TRY("%d+%dx%d", a, factor, quotient);
                    break;
                  }
                }
              }
            }
          }
          else if (pattern == 5 && target >= 2) {
            // Pattern: a-bxc = target → bxc = a-target (order: multiply first, then subtract)
            int product = random(2, 50);
            a = target + product;
            if (a <= 99) {
              for (int factor = 9; factor >= 2; factor--) {
                if (product % factor == 0) {
                  int quotient = product / factor;
                  if (quotient <= 9) {
                    TRY("%d-%dx%d", a, factor, quotient);
                    break;
                  }
                }
              }
            }
          }
        }
        break;
        
      default:
        // Invalid level - use simple addition
        a = random(0, target + 1);
        TRY("%d+%d", a, target - a);
        break;
    }
    
    // Check if result fits in 5 characters and is valid
    if (_eqBuf[0] != '\0' && strlen(_eqBuf) <= 5) {
      return _eqBuf;
    }
  }
  
  // Fallback if no valid equation found
  int a = random(0, min(target + 1, 9));
  int b = target - a;
  TRY("%d+%d", a, b);
  if (strlen(_eqBuf) > 5) {
    TRY("%d", target);
  }
  return _eqBuf;
}

/**
 * Get font array index for a character
 * Maps characters to their position in the font array
 * 
 * @param c Character to look up
 * @return Index in font array (0-41) or -1 if not found
 */
int getCharIdx(char c) {
  // Numbers 0-9
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  
  // Math operators
  if (c == '+') return 10;
  if (c == '-') return 11;
  if (c == 'x') return 12;
  if (c == '/') return 13;
  
  // Alphabet A-Z (indices 14-39)
  if (c >= 'A' && c <= 'Z') {
    return 14 + (c - 'A');
  }
  
  // Special characters
  if (c == '<') return 40;
  if (c == '=') return 41;
  
  // Space or unknown character
  return -1;
}
