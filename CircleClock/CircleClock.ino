#include <LPD8806.h>
#include <SPI.h>
#include "color.h"

#define DEBUG (false)

const byte
  // Physical aspects of the clock
  stripLength = 158,
  minuteBegin = 0,
  minuteDots = 60,
  hourBegin = 62,
  hourDots = 96,
  initialBrightness = 100,
  // Gong timing, ring delay and relative brightness levels
  gongFrameTime = 20,
  gongHourFrameDelay = 5,
  gongLevel[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 17, 19, 23, 28, 34, 40, 48, 58, 70, 84, 100,
                 84, 70, 55, 48, 40, 34, 28, 23, 19, 17, 15, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 1},
  gongLevels = sizeof(gongLevel),
  // Relative distribution of time parts
  ticksPerMinute = 60,
  minutesPerHour = 60,
  hoursPerCircle = 12,
  intervalUpdate = 20;
const char
  // Speed and stepsize for clock adjustments
  adjustmentStepDelay = 5,
  adjustmentStepSize = 5;
const int
  intervalTick = 1000,
  ticksPerHour = ticksPerMinute * minutesPerHour,
  ticksPerMinuteDot = ticksPerHour / minuteDots;
const long
  ticksPerCircle = (long) ticksPerHour * hoursPerCircle,
  ticksPerHourDot = ticksPerCircle / hourDots;
bool
  secondHandRising = true;
byte
  brightness = initialBrightness,
  secondHandBrightness = 0;
int
  ticksPastHour = 0;
long
  ticksPastCircle = 0,
  unixTime = 0;
rgbdata_t minuteRingColor;

LPD8806 strip = LPD8806(stripLength);

void setup() {
  strip.begin();
  Serial.begin(9600);
  #if DEBUG
    Serial.println("[circleclock_animation]");
    Serial.print("ticksPerMinute: ");
    Serial.println(ticksPerMinute);
    Serial.print("minutesPerHour: ");
    Serial.println(minutesPerHour);
    Serial.print("ticksPerHour: ");
    Serial.println(ticksPerHour);
    Serial.print("hoursPerCircle: ");
    Serial.println(hoursPerCircle);
    Serial.print("ticksPerCircle: ");
    Serial.println(ticksPerCircle);
    Serial.print("gongLevels: ");
    Serial.println(gongLevels);
  #endif
}

void loop() {
  static long
    nextTick = 0,
    nextUpdate = 0;
  long currentMillis = millis();
  if (Serial.available())
    processSerialInput();
  if (currentMillis >= nextTick) {
    // One second passed, set timer for next
    nextTick = currentMillis + intervalTick;
    eachTick();
  }
  if (currentMillis >= nextUpdate) {
    nextUpdate = currentMillis + intervalUpdate;
    eachUpdate();
  }
}

void eachUpdate(void) {
  // Updates the color and brightness of the seconds and minute ring
  displayMinutes(minuteRingColor);
  displayMinuteHandBlink(minuteRingColor);
  strip.show();
}

void eachTick(void) {
  // Updates that are run each second.
  // Increments tracked time and offsets, updates hour hand color and position
  ++unixTime;
  ticksPastHour = unixTime % ticksPerHour;
  ticksPastCircle = unixTime % ticksPerCircle;
  minuteRingColor = minuteColor();
  printTimeSerial();
  if (ticksPastHour % ticksPerMinute == 0)
    eachMinute();
  displayHours(hourColor());
}

void eachMinute(void) {
  // After each minute, resets the blinking hand brightness and direction.
  secondHandBrightness = 0;
  secondHandRising = true;
  if (ticksPastHour == 0)
    eachHour();
}

void eachHour(void) {
  // Each hour, gong the number of hours that have passed and synchronize the time.
  hourGong(ticksPastCircle / ticksPerHour);
}

void printTimeSerial(void) {
  Serial.print(ticksPastCircle / ticksPerHour);
  Serial.print(":");
  Serial.print(ticksPastHour / ticksPerMinute);
  Serial.print(":");
  Serial.println(ticksPastHour % ticksPerMinute);
}

void processSerialInput(void) {
  switch (Serial.peek()) {
    case 10: // Newline
      Serial.read();
      break;
    case 'b':
      Serial.read();
      adjustBrightness(Serial.parseInt());
      break;
    case '+':
      Serial.read();
      adjustClock(Serial.parseInt());
      break;
    case '-':
      adjustClock(Serial.parseInt());
      break;
    default:
      setClock(Serial.parseInt());
  }
}

void adjustBrightness(char newBrightness) {
  brightness = newBrightness;
  #if DEBUG
    Serial.print("Brightness set to: ");
    Serial.println(brightness, DEC);
  #endif
}

void adjustClock(long difference) {
  char minuteDiff, hourDiff;
  int minuteHandShift, minuteSteps, hourSteps;
  long hourHandShift;
  unixTime += difference;
  minuteHandShift = difference % ticksPerHour;
  hourHandShift = difference % ticksPerCircle;
  if (ticksPastHour + minuteHandShift < 0)
    minuteHandShift += ticksPerHour;
  else if (ticksPastHour + minuteHandShift > ticksPerHour)
    minuteHandShift -= ticksPerHour;
  if (ticksPastCircle + hourHandShift < 0)
    hourHandShift += ticksPerCircle;
  else if (ticksPastCircle + hourHandShift > ticksPerCircle)
    hourHandShift -= ticksPerCircle;
  minuteDiff = adjustmentStepSize * (minuteHandShift < 0 ? -1 : 1);
  hourDiff = adjustmentStepSize * hoursPerCircle * (hourHandShift < 0 ? -1 : 1);
  minuteSteps = minuteHandShift / minuteDiff;
  hourSteps = hourHandShift / hourDiff;
  while (minuteSteps || hourSteps) {
    if (minuteSteps) {
      --minuteSteps;
      ticksPastHour += minuteDiff;
      displayMinutes(minuteColor());
    }
    if (hourSteps) {
      --hourSteps;
      ticksPastCircle += hourDiff;
      displayHours(hourColor());
    }
    strip.show();
    delay(adjustmentStepDelay);
  }
  minuteRingColor = minuteColor();
}

void setClock(unsigned long time) {
  adjustClock(time - unixTime);
}

void displayHours(rgbdata_t color) {
  bool done = false;
  long sColor = lpdColor(color);
  for (byte i = 0; i < hourDots; ++i) {
    if (ticksPastCircle >= (i + 1) * ticksPerHourDot) {
      strip.setPixelColor(i + hourBegin, sColor);
    } else if (!done) {
      byte relativeBrightness = (
          100 * (ticksPastCircle % ticksPerHourDot) / ticksPerHourDot);
      strip.setPixelColor(i + hourBegin, lpdColor(attenuateColor(color, relativeBrightness)));
      done = true;
    } else {
      strip.setPixelColor(i + hourBegin, 0);
    }
  }
}

void displayMinutes(rgbdata_t color) {
  bool done = false;
  long sColor = lpdColor(color);
  for (byte i = 0; i < minuteDots; ++i) {
    if (ticksPastHour >= (i + 1) * ticksPerMinuteDot) {
      strip.setPixelColor(i + minuteBegin, sColor);
    } else if (!done) {
      byte relativeBrightness = (
          100 * (ticksPastHour % ticksPerMinuteDot) / ticksPerMinuteDot);
      strip.setPixelColor(i + minuteBegin, lpdColor(attenuateColor(color, relativeBrightness)));
      done = true;
    } else {
      strip.setPixelColor(i + minuteBegin, 0);
    }
  }
}

void displayMinuteHandBlink(rgbdata_t color) {
  const byte handSteps = intervalTick / intervalUpdate / 2;
  byte handDot, relativeBrightness;
  handDot = ticksPastHour / ticksPerMinuteDot + minuteBegin;
  if (ticksPastHour % ticksPerMinuteDot == (ticksPerMinuteDot - 1)) {
    secondHandBrightness = min(++secondHandBrightness, handSteps);
  } else if (secondHandRising) {
    if (++secondHandBrightness == handSteps)
      secondHandRising = false;
  } else {
    if (!--secondHandBrightness)
      secondHandRising = true;
  }
  relativeBrightness = 100 * secondHandBrightness / handSteps;
  strip.setPixelColor(handDot, lpdColor(attenuateColor(color, relativeBrightness)));
}

void hourGong(byte strikes) {
  unsigned int frames;
  unsigned long hColor, mColor;
  rgbdata_t color;
  if (strikes == 0)
    strikes = hoursPerCircle;
  color = hourColor();
  // Fade minute ring before gonging
  for (byte bright = 95; bright > 0; bright -= 5) {
    mColor = lpdColor(attenuateColor(minuteColor(), bright));
    displayHours(attenuateColor(color, bright));
    for (byte pos = minuteDots; pos-- > 0;)
      strip.setPixelColor(minuteBegin + pos, mColor);
    strip.show();
    delay(gongFrameTime);
  }
  // TODO: Fade out hour ring before gonging, fade it back in when done.
  // Preferably have the first gong rise blend into the hour ring
  // and have the last gong fade leave the current hour in place.
  frames = strikes * gongLevels + gongHourFrameDelay;
  for (unsigned int frame = 0; frame < frames; ++frame) {
    // Gong on minutes ring
    if (frame / gongLevels < strikes) {
      mColor = lpdColor(attenuateColor(
          color, gongLevel[frame % gongLevels]));
      for (byte pos = minuteDots; pos-- > 0;)
        strip.setPixelColor(minuteBegin + pos, mColor);
    }
    // Gong on hours ring, slightly delayed
    if (frame >= gongHourFrameDelay) {
      hColor = lpdColor(attenuateColor(
          color, gongLevel[(frame - gongHourFrameDelay) % gongLevels]));
      for (byte pos = hourDots; pos-- > 0;)
        strip.setPixelColor(hourBegin + pos, hColor);
    }
    // Make visible and pause between frames
    strip.show();
    delay(gongFrameTime);
  }
}

rgbdata_t hourColor() {
  // Returns the color for the hour ring (global brightness is applied)
  return attenuateColor(colorWheel(ticksPastCircle, ticksPerCircle), brightness);
}

rgbdata_t minuteColor() {
  // Returns the color for the minute ring (global brightness is applied)
  return attenuateColor(colorWheel(ticksPastHour, ticksPerHour), brightness);
}

rgbdata_t attenuateColor(rgbdata_t color, byte brightness) {
  // Changes the brightness for a particular color, each channel linearly
  color.r = color.r * brightness / 100;
  color.g = color.g * brightness / 100;
  color.b = color.b * brightness / 100;
  return color;
}

rgbdata_t colorWheel(unsigned long position, unsigned long scale) {
  // Returns a color out of 768 possible angle fractions of a color wheel
  unsigned int angle = position * 768 / scale % 768;
  byte r, g, b;
  switch (angle / 128) {
    case 0:  // Green up
      r = 127;
      g = angle % 128;
      b = 0;
      break;
    case 1:  // Red down
      r = 127 - angle % 128;
      g = 127;
      b = 0;
      break;
    case 2:  // Blue up
      r = 0;
      g = 127;
      b = angle % 128;
      break;
    case 3: // Green down
      r = 0;
      g = 127 - angle % 128;
      b = 127;
      break;
    case 4:  // Red up
      r = angle % 128;
      g = 0;
      b = 127;
      break;
    case 5:  // Blue down
      r = 127;
      g = 0;
      b = 127 - angle % 128;
      break;
  }
  // Strip is wired differently from library expectations, RBG order is correct.
  return rgbdata_t {r, g, b};
}

unsigned long lpdColor(rgbdata_t color) {
  return strip.Color(color.r, color.g, color.b);
}
