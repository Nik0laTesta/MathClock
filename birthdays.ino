/*
 * BIRTHDAY MANAGEMENT TAB
 *
 * Hardcoded family birthdays only — no EEPROM.
 * Add or remove entries in familyBirthdays[] freely;
 * the code reads the list length automatically.
 *
 * At minute 0 of each hour on a birthday:
 *   Top row  : alternates HAPPY / BDAY every 5 seconds
 *   Bottom row: cycles each word of the name then age in years
 *               e.g. "OPA ZEE" born 1960 -> OPA -> ZEE -> 66J -> repeat
 *               Multiple people sharing a day cycle one after the other.
 */

// --- HARDCODED FAMILY BIRTHDAYS ----------------------------------------------
// Format: { "FULL NAME", month, day, year }
// Names split on spaces — each word shown separately on the bottom row.
// Add or remove lines freely; length is calculated automatically.

struct BirthdayFlash {
  const char* name;
  uint8_t     month;
  uint8_t     day;
  uint16_t    year;
};

const BirthdayFlash familyBirthdays[] = {
  { "FAM ONE",    2,   6, 1960 },
  { "FAM TWO",  2,  14, 1991 },
  
};
const uint8_t FAMILY_BDAY_COUNT = sizeof(familyBirthdays) / sizeof(familyBirthdays[0]);

// --- HELPERS -----------------------------------------------------------------

// Count space-separated words in a name string.
static uint8_t countNameWords(const char* name) {
  uint8_t count = 1;
  for (uint8_t i = 0; name[i] != '\0'; i++) {
    if (name[i] == ' ') count++;
  }
  return count;
}

// Copy the nth word (0-indexed) from a space-separated name into out[6].
static void getNthNameWord(const char* name, uint8_t n, char out[6]) {
  uint8_t word = 0;
  uint8_t j    = 0;
  out[0] = '\0';
  for (uint8_t i = 0; name[i] != '\0'; i++) {
    if (name[i] == ' ') {
      if (word == n) { out[j] = '\0'; return; }
      word++;
      j = 0;
    } else if (word == n && j < 5) {
      out[j++] = name[i];
    }
  }
  if (word == n) out[j] = '\0';
}

// --- BIRTHDAY DISPLAY --------------------------------------------------------

/*
 * Called from checkSpecialDays() at minute 0 only.
 * Returns true when a birthday matches today (suppresses normal clock).
 *
 * Top    : HAPPY (even phases) / BDAY (odd phases), 5 s each
 * Bottom : words of name then age, cycling every 5 s
 */
bool checkAndDisplayBirthdays(DateTime now) {

  // Find today's matches — small fixed array, realistic max is 2-3
  uint8_t matchIdx[8];
  uint8_t matchCount = 0;

  for (uint8_t i = 0; i < FAMILY_BDAY_COUNT && matchCount < 8; i++) {
    if (familyBirthdays[i].month == now.month() &&
        familyBirthdays[i].day   == now.day()) {
      matchIdx[matchCount++] = i;
    }
  }

  if (matchCount == 0) return false;

  // Count total bottom-row words: each person contributes name words + 1 age word
  uint8_t totalWords = 0;
  for (uint8_t i = 0; i < matchCount; i++) {
    totalWords += countNameWords(familyBirthdays[matchIdx[i]].name) + 1;
  }

  // Current phase (5 s each): even = HAPPY, odd = BDAY
  uint16_t phaseIdx    = (uint16_t)(millis() / 5000UL);
  uint16_t totalPhases = (uint16_t)totalWords * 2;
  uint16_t phase       = phaseIdx % totalPhases;

  bool    isHappy  = (phase % 2 == 0);
  uint8_t wordIdx  = phase / 2;

  // Walk through matches to find which word wordIdx refers to
  char    bottomWord[6] = "";
  uint8_t wordsSoFar    = 0;

  for (uint8_t i = 0; i < matchCount; i++) {
    const char* name      = familyBirthdays[matchIdx[i]].name;
    uint16_t    birthYear = familyBirthdays[matchIdx[i]].year;
    uint8_t     nameWords = countNameWords(name);
    uint8_t     wordsThis = nameWords + 1;

    if (wordIdx < wordsSoFar + wordsThis) {
      uint8_t localIdx = wordIdx - wordsSoFar;
      if (localIdx < nameWords) {
        getNthNameWord(name, localIdx, bottomWord);
      } else {
        snprintf(bottomWord, sizeof(bottomWord), "%dJ", now.year() - birthYear);
      }
      break;
    }
    wordsSoFar += wordsThis;
  }

  // Draw
  FastLED.clear();
  uint8_t hue = (uint8_t)(phaseIdx * 37);
  drawStaticRow(isHappy ? "HAPPY" : "BDAY", true,  CHSV(hue,       255, 255));
  drawStaticRow(bottomWord,                 false, CHSV(hue + 128, 255, 255));

  return true;
}
