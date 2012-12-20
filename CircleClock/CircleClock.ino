#include <LPD8806.h>
#include <SPI.h>

// Basic times and counts
const byte
  // Physical aspects of the c
  stripLength = 158,
  minuteBegin = 0,
  minuteDots = 60,
  hourBegin = 62,
  hourDots = 96,
  brightness = 100, // this doesn't work yet for the main colors
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
      secondHandBrightness = 0;
      secondHandRising = true;
    }
    printTimeSerial();
    minuteRingColor = minuteColor();
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
  Serial.println("Processing serial input");
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
      minuteRingColor = minuteColor();
      displayMinutes(minuteRingColor);
    }
    if (hourSteps) {
      --hourSteps;
      ticksPastCircle += hourDiff;
      displayHours(hourColor());
    }
    strip.show();
    delay(5);
  }
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

unsigned long hourColor() {
  return colorWheel(ticksPastCircle, ticksPerCircle);
}

unsigned long minuteColor() {
  return colorWheel(ticksPastHour, ticksPerHour);
}

unsigned long colorBrightness(unsigned long color, byte brightness) {
  byte green = ((color >> 16) & 0x7F) * brightness / 100;
  byte red = ((color >> 8) & 0x7F) * brightness / 100;
  byte blue = (color & 0x7F) * brightness / 100;
  return strip.Color(red, green, blue);
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
  return strip.Color(r, g, b);
}
