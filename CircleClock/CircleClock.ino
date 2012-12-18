#include <LPD8806.h>
#include <SPI.h>

// Basic times and counts
const byte
  stripLength = 158,
  minuteBegin = 0,
  minuteDots = 60,
  hourBegin = 62,
  hourDots = 96,
  brightness = 100, // this doesn't work yet for the main colors
  minutesPerHour = 60, // This does not take well to changes
  hoursPerCircle = 12,
  hoursPerDay = 24;
const unsigned int
  intervalTick = 1000,  // 1000
  intervalUpdate = 20, // 20,
  ticksPerMinute = 60, // 60
  ticksPerHour = ticksPerMinute * minutesPerHour;
const unsigned long
  ticksPerCircle = ticksPerHour * hoursPerCircle,
  ticksPerDay = ticksPerHour * hoursPerDay;
bool
  secondHandRising = true;
byte
  secondHandBrightness = 0;
unsigned long
  // Relative timekeeping colors variables
  colorHandMinute = 0,
  colorHandHour = 0,
  secondsPastHour = 0,
  secondsPastMidnight = 0,
  // Timing variables
  currentMillis = 0,
  nextTick = 0,
  nextUpdate = 0;

LPD8806 strip = LPD8806(stripLength);

void setup() {
  strip.begin();
  Serial.begin(57600);
  Serial.println("[circleclock_animation]");
}

void loop() {
  currentMillis = millis();
  if (currentMillis >= nextTick) {
    nextTick += intervalTick;  // One second passed, set timer for next
    ++secondsPastHour %= ticksPerHour;
    ++secondsPastMidnight %= ticksPerDay;
    if (secondsPastHour % ticksPerMinute == 0) {
      secondHandBrightness = 0;
      secondHandRising = true;
    }
    colorHandMinute = colorWheel(secondsPastHour, ticksPerHour);
    colorHandHour = colorWheel(secondsPastMidnight, ticksPerCircle);
    DisplayHours();
  }
  if (currentMillis >= nextUpdate) {
    nextUpdate += intervalUpdate; // Did a color update, set timer for next
    DisplayMinutes();
    strip.show();
  }
}

void DisplayHours() {
  const unsigned int ticksPerDot = ticksPerCircle / hourDots;
  unsigned int secondsPast = secondsPastMidnight % ticksPerCircle;
  bool done = false;
  for (byte i = 0; i < hourDots; ++i) {
    if (secondsPast >= (i + 1) * ticksPerDot) {
      strip.setPixelColor(i + hourBegin, colorHandHour);
    } else if (!done) {
      byte effectiveBrightness = brightness * (secondsPast % ticksPerDot) / ticksPerDot;
      strip.setPixelColor(i + hourBegin, colorBrightness(colorHandHour, effectiveBrightness));
      done = true;
    } else {
      strip.setPixelColor(i + hourBegin, 0);
    }
  }
}

void DisplayMinutes() {
  const byte ticksPerDot = ticksPerHour / minuteDots;
  const byte handSteps = intervalTick / intervalUpdate / 2;
  bool done = false;
  for (byte i = 0; i < minuteDots; ++i) {
    if (secondsPastHour >= (i + 1) * ticksPerDot) {
      strip.setPixelColor(i + minuteBegin, colorHandMinute);
    } else if (!done) {
      if (secondsPastHour % ticksPerDot == (ticksPerDot - 1)) {
        secondHandBrightness = min(++secondHandBrightness, handSteps);
      } else if (secondHandRising) {
        if (++secondHandBrightness == handSteps)
          secondHandRising = false;
      } else {
        if (--secondHandBrightness == 0)
          secondHandRising = true;
      }
      byte effectiveBrightness = brightness * secondHandBrightness / handSteps;
      Serial.println(effectiveBrightness);
      unsigned long secondHandColor = colorBrightness(colorHandMinute, effectiveBrightness);
      strip.setPixelColor(i + minuteBegin, secondHandColor);
      done = true;
    } else {
      strip.setPixelColor(i + minuteBegin, 0);
    }
  }
}
/*
uint32_t hourColor() {
  return colorWheel(secondsPastMidnight, ticksPerCircle);
}

uint32_t minuteColor() {
  return colorWheel(secondsPastHour, ticksPerHour);
}
*/
long colorBrightness(unsigned long color, byte brightness) {
  byte green = ((color >> 16) & 0x7F) * brightness / 100;
  byte red = ((color >> 8) & 0x7F) * brightness / 100;
  byte blue = (color & 0x7F) * brightness / 100;
  return strip.Color(red, green, blue);
}

long colorWheel(unsigned long position, unsigned long scale) {
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
  return strip.Color(r, g, b);
}
