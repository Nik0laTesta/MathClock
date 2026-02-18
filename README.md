# LED Matrix Math Clock

A custom Arduino-based clock that displays the time as math equations on a 15×5 WS2812B LED matrix, with built-in games, birthday celebrations, and IR remote control.
Everything was 3D printed and designed by me.
The exact components list, materials, code and models will soon be available on cults3D.com
On there I will also explain some more about why you should use certain materials because I learned the hard way some materials just dont play nice together. 
The foto's are still a work in progress. Apart from a few minor tweaks the code seems good. 
I am currently still waiting for some plexiglass dark panel samples to test over the leds and then we can stick it all together.

## Features

### Clock Display
- Time shown as dynamically generated math equations
- 5 difficulty levels (simple addition → complex multi-operation equations)
- Automatic brightness adjustment via LDR sensor
- Special event displays (birthdays, holidays, morning sequence)

### Games
Three built-in games controlled via IR remote:
- **Dino Game** - Classic jump-and-dodge runner
- **Dodge Game** - Avoid falling blocks
- **Snake** - Navigate and grow

### Birthday Celebrations
- Hardcoded family birthday list
- Automatic display at minute 0 and 30 of each hour on birthdays
- Alternating "HAPPY" / "BDAY" with name words and age
- Rainbow color cycling

### Settings Menu
- Adjustable difficulty (1-5)
- Set time and date
- Quick access via IR remote

## Hardware

- **Microcontrollers**: Arduino Nano (main) + Arduino Nano (IR co-processor)
- **Display**: 154 WS2812B LEDs arranged as 15×5 grid
- **RTC**: DS3231 real-time clock module
- **Input**: IR receiver (handled by co-processor)
- **Sensors**: LDR (light sensor) on A0, push button on A2
- **Data pin**: D6

### Wiring
- LED matrix data → D6
- RTC → I2C (A4/A5)
- LDR → A0
- Button → A2
- IR co-processor pins → D3, D4, D5, D7, D8, D9, D10, D11, D12, D13

## IR Remote Layout

The IR co-processor Nano translates IR signals into digital pulses on dedicated pins:

| Pin | Function |
|-----|----------|
| D10 | GAME1 (launch Dino) |
| D11 | GAME2 (launch Dodge) |
| D12 | GAME3 (launch Snake) |
| D13 | OPTIONS (open settings) |
| D4  | UP |
| D5  | DOWN |
| D7  | LEFT |
| D8  | RIGHT |
| D9  | OK |
| D3  | RETURN (back/cancel) |

## Software Structure

### Files
- `math_clock_v21.ino` - Main sketch with setup, loop, and mode management
- `input.ino` - IR and button input handling
- `display.ino` - LED drawing functions and 3×5 font
- `math.ino` - Math equation generator (5 difficulty levels)
- `birthdays.ino` - Birthday display logic
- `game.ino` - Dino game
- `dodge_game.ino` - Dodge/falling blocks game
- `snake_game.ino` - Snake game

### Dependencies
- [FastLED](https://github.com/FastLED/FastLED)
- [RTClib](https://github.com/adafruit/RTClib)
- Wire (I2C)
- EEPROM
