#include <LPD8806.h>
#include <SPI.h>

#define DEBUG (true)

// Basic times and counts
const byte
  // Physical aspects of the c
  stripLength = 158,
  minuteBegin = 0,
  minuteDots = 60,
  hourBegin = 62,
  hourDots = 96,
  brightness = 40,
  // Relative brightness levels for the hour gong.
  gongHourFrameDelay = 4,
  gongLevel[] = {1, 2, 3, 4, 5, 6, 7, 9, 11, 14, 17, 20, 25, 30, 35, 45, 55, 70, 85, 100,
                 85, 70, 55, 45, 35, 30, 25, 20, 17, 14, 11, 9, 7, 6, 5, 4, 3, 2, 1, 0},
  gongLevels = sizeof(gongLevel),
  // Relative distribution of time parts
  ticksPerMinute = 60,
  minutesPerHour = 60,
  hoursPerCircle = 12,
  intervalUpdate = 20;
char
  adjustmentStepSize = 5;
const unsigned int
  intervalTick = 1000;
const int
  ticksPerHour = ticksPerMinute * minutesPerHour,
  ticksPerMinuteDot = ticksPerHour / minuteDots;
const long
  ticksPerCircle = (long) ticksPerHour * hoursPerCircle,
  ticksPerHourDot = ticksPerCircle / hourDots;

bool
  secondHandRising = true;
byte
  secondHandBrightness = 0;
int
  ticksPastHour = 0;
long
  ticksPastCircle = 0,
  unixTime = 0;
unsigned long
  minuteRingColor = 0;

LPD8806 strip = LPD8806(stripLength);

void setup() {
  strip.begin();
  if (DEBUG) {
    Serial.begin(9600);
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
  }
}

void loop() {
  static long nextTick = 0;
  static long nextUpdate = 0;
  long currentMillis = millis();
  if (Serial.available())
    processSerialInput();
  if (currentMillis >= nextTick) {
    // One second passed, set timer for next
    nextTick = currentMillis + intervalTick;
    ++unixTime;
    ticksPastHour = unixTime % ticksPerHour;
    ticksPastCircle = unixTime % ticksPerCircle;
    if (ticksPastHour % ticksPerMinute == 0) {
      // A minute has past
      secondHandBrightness = 0;
      secondHandRising = true;
    }
    printTimeSerial();
    minuteRingColor = minuteColor();
    if (ticksPastHour == 0)
      hourGong(ticksPastCircle / ticksPerHour);
    else
      displayHours(hourColor());
  }
  if (currentMillis >= nextUpdate) {
    // Did a color update, set timer for next
    nextUpdate = currentMillis + intervalUpdate;
    displayMinutes(minuteRingColor);
    displayMinuteHandBlink(minuteRingColor);
    strip.show();
  }
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
    case '+':
      Serial.read();
      clockAdjust(Serial.parseInt());
      break;
    case '-':
      Serial.read();
      clockAdjust(-Serial.parseInt());
      break;
    default:
      clockSetTime(Serial.parseInt());
  }
}

void clockAdjust(long difference) {
  unixTime += difference;
  int minuteHandShift = difference % ticksPerHour;
  long hourHandShift = difference % ticksPerCircle;
  if (ticksPastHour + minuteHandShift < 0)
    minuteHandShift += ticksPerHour;
  else if (ticksPastHour + minuteHandShift > ticksPerHour)
    minuteHandShift -= ticksPerHour;
  if (ticksPastCircle + hourHandShift < 0)
    hourHandShift += ticksPerCircle;
  else if (ticksPastCircle + hourHandShift > ticksPerCircle)
    hourHandShift -= ticksPerCircle;
  char minuteDiff = adjustmentStepSize;
  if (minuteHandShift < 0)
    minuteDiff = -minuteDiff;
  char hourDiff = adjustmentStepSize * hoursPerCircle;
  if (hourHandShift < 0)
    hourDiff = -hourDiff;
  ticksPerHourDot / (hourHandShift > 0 ? 6 : -6);
  int minuteSteps = minuteHandShift / minuteDiff;
  int hourSteps = hourHandShift / hourDiff;
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
    delay(5);
  }
  minuteRingColor = minuteColor();
}

void clockSetTime(unsigned long time) {
  clockAdjust(time - unixTime);
}

void displayHours(long color) {
  bool done = false;
  for (byte i = 0; i < hourDots; ++i) {
    if (ticksPastCircle >= (i + 1) * ticksPerHourDot) {
      strip.setPixelColor(i + hourBegin, color);
    } else if (!done) {
      byte effectiveBrightness = (
          brightness * (ticksPastCircle % ticksPerHourDot) / ticksPerHourDot);
      strip.setPixelColor(i + hourBegin, colorBrightness(color, effectiveBrightness));
      done = true;
    } else {
      strip.setPixelColor(i + hourBegin, 0);
    }
  }
}

void displayMinutes(long color) {
  bool done = false;
  for (byte i = 0; i < minuteDots; ++i) {
    if (ticksPastHour >= (i + 1) * ticksPerMinuteDot) {
      strip.setPixelColor(i + minuteBegin, color);
    } else if (!done) {
      byte effectiveBrightness = (
          brightness * (ticksPastHour % ticksPerMinuteDot) / ticksPerMinuteDot);
      strip.setPixelColor(i + minuteBegin, colorBrightness(color, effectiveBrightness));
      done = true;
    } else {
      strip.setPixelColor(i + minuteBegin, 0);
    }
  }
}

void displayMinuteHandBlink(long color) {
  const byte handSteps = intervalTick / intervalUpdate / 2;
  byte handDot = ticksPastHour / ticksPerMinuteDot + minuteBegin;
  if (ticksPastHour % ticksPerMinuteDot == (ticksPerMinuteDot - 1)) {
    secondHandBrightness = min(++secondHandBrightness, handSteps);
  } else if (secondHandRising) {
    if (++secondHandBrightness == handSteps)
      secondHandRising = false;
  } else {
    if (!--secondHandBrightness)
      secondHandRising = true;
  }
  byte effectiveBrightness = brightness * secondHandBrightness / handSteps;
  unsigned long secondHandColor = colorBrightness(color, effectiveBrightness);
  strip.setPixelColor(handDot, secondHandColor);
}

void hourGong(byte strikes) {
  unsigned long color, hColor, mColor;
  if (strikes == 0)
    strikes = hoursPerCircle;
  color = hourColor();
  // Fade minute hand before gonging
  for (byte bright = 95; bright > 0; bright -= 5) {
    mColor = colorBrightness(minuteColor(), bright);
    for (byte pos = minuteDots; pos-- > 0;)
      strip.setPixelColor(minuteBegin + pos, mColor);
    strip.show();
    delay(10);
  }
  unsigned int frames = strikes * gongLevels + gongHourFrameDelay;
  for (int frame = 0; frame < frames; ++frame) {
    // Gong on minutes ring
    if (frame / gongLevels < strikes) {
      mColor = colorBrightness(color, gongLevel[frame % gongLevels]);
      for (byte pos = minuteDots; pos-- > 0;)
        strip.setPixelColor(minuteBegin + pos, mColor);
    }
    // Gong on hours ring, slightly delayed
    if (frame >= gongHourFrameDelay) {
      hColor = colorBrightness(color, gongLevel[(frame - gongHourFrameDelay) % gongLevels]);
      for (byte pos = hourDots; pos-- > 0;)
        strip.setPixelColor(hourBegin + pos, hColor);
    }
    // Make visible and pause between frames
    strip.show();
    delay(15);
  }
}

unsigned long hourColor() {
  return colorBrightness(
      colorWheel(ticksPastCircle, ticksPerCircle), brightness);
}

unsigned long minuteColor() {
  return colorBrightness(
      colorWheel(ticksPastHour, ticksPerHour), brightness);
}

unsigned long colorBrightness(unsigned long color, byte brightness) {
  byte b = ((color >> 16) & 0x7F) * brightness / 100;
  byte r = ((color >> 8) & 0x7F) * brightness / 100;
  byte g = (color & 0x7F) * brightness / 100;
  return strip.Color(r, b, g);
}

unsigned long colorWheel(unsigned long position, unsigned long scale) {
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
  //return strip.Color(r, g, b);
  // Change order of colors because the strip is wired differently
  return strip.Color(r, b, g);
}
