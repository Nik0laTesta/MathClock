/*
 * IR CO-PROCESSOR  –  math_clock_ir_v20
 * Second Arduino Nano: receives Panasonic IR, pulses dedicated pins to main Nano.
 *
 * WIRING (IR Nano → Main Nano):
 *   IR Nano D3   →  Main Nano D3    (RETURN – back / cancel)
 *   IR Nano D4   →  Main Nano D4    (UP)
 *   IR Nano D5   →  Main Nano D5    (DOWN)
 *   IR Nano D6   →  Main Nano D7    (LEFT)
 *   IR Nano D7   →  Main Nano D8    (RIGHT)
 *   IR Nano D8   →  Main Nano D9    (OK)
 *   IR Nano D9   →  Main Nano D10   (GAME1 – DINO)
 *   IR Nano D10  →  Main Nano D11   (GAME2 – DODGE)
 *   IR Nano D11  →  Main Nano D12   (GAME3 – SNAKE)
 *   IR Nano D12  →  Main Nano D13   (OPTIONS – open settings)
 *   Shared GND
 *
 * Main Nano uses INPUT_PULLUP on all input pins.
 * IR Nano pulls pin LOW to signal (active LOW).
 *
 * PANASONIC BUTTON MAP:
 *   0x85  UP        0x86  DOWN     0x87  LEFT    0x88  RIGHT
 *   0x82  OK/Enter
 *   0x10  Button 1  (DINO)
 *   0x11  Button 2  (DODGE)
 *   0x12  Button 3  (SNAKE)
 *   0x80  OPTIONS   (open settings)
 *   0x81  RETURN    (back / cancel)
 */

#include <IRremote.hpp>

// --- INPUT ---
#define IR_RECEIVE_PIN  2

// --- OUTPUTS (to Main Nano) – active LOW ---
#define OUT_RETURN  3
#define OUT_UP      4
#define OUT_DOWN    5
#define OUT_LEFT    6
#define OUT_RIGHT   7
#define OUT_OK      8
#define OUT_GAME1   9    // DINO
#define OUT_GAME2   10   // DODGE
#define OUT_GAME3   11   // SNAKE
#define OUT_OPTIONS 12   // open settings

#define PULSE_MS 150

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  // Initialise all outputs HIGH (inactive – matches INPUT_PULLUP on main nano)
  int outPins[] = { OUT_RETURN, OUT_UP, OUT_DOWN, OUT_LEFT, OUT_RIGHT,
                    OUT_OK, OUT_GAME1, OUT_GAME2, OUT_GAME3, OUT_OPTIONS };
  for (int i = 0; i < 10; i++) {
    pinMode(outPins[i], OUTPUT);
    digitalWrite(outPins[i], HIGH);   // idle HIGH
  }

  Serial.println(F("IR Co-Processor v20 active"));
  Serial.println(F("Listening for Panasonic codes..."));
}

// ---------------------------------------------------------------------------

// Pull pin LOW for PULSE_MS then return HIGH (active LOW signalling)
void sendPulse(int pin, const char* label) {
  Serial.print(F("Action: ")); Serial.println(label);
  digitalWrite(pin, LOW);
  delay(PULSE_MS);
  digitalWrite(pin, HIGH);
}

// ---------------------------------------------------------------------------

void loop() {
  if (!IrReceiver.decode()) return;

  uint8_t cmd = IrReceiver.decodedIRData.command;

  Serial.print(F("IR Received: 0x"));
  Serial.print(cmd, HEX);
  Serial.print(F(" -> "));

  switch (cmd) {
    case 0x85: sendPulse(OUT_UP,      "UP");            break;
    case 0x86: sendPulse(OUT_DOWN,    "DOWN");          break;
    case 0x87: sendPulse(OUT_LEFT,    "LEFT");          break;
    case 0x88: sendPulse(OUT_RIGHT,   "RIGHT");         break;
    case 0x82: sendPulse(OUT_OK,      "OK");            break;
    case 0x10: sendPulse(OUT_GAME1,   "GAME1 – DINO");  break;
    case 0x11: sendPulse(OUT_GAME2,   "GAME2 – DODGE"); break;
    case 0x12: sendPulse(OUT_GAME3,   "GAME3 – SNAKE"); break;
    case 0x80: sendPulse(OUT_OPTIONS, "OPTIONS");       break;
    case 0x81: sendPulse(OUT_RETURN,  "RETURN");        break;
    default:
      Serial.println(F("Unknown code"));
      break;
  }

  IrReceiver.resume();
}
