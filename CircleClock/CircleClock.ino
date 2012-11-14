#include <LPD8806.h>
#include <SPI.h>

// Basic times and counts
#define LED_COUNT (42)
#define MINUTES (LED_COUNT)
#define SECONDS_PER_MINUTE (60)
#define SECONDS_PER_HOUR (MINUTES * SECONDS_PER_MINUTE)

#define BRIGHTNESS (50)

// Intervals
#define INTERVAL_SECOND (1000)
#define INTERVAL_UPDATE (20)
#define MINUTE_HAND_STEPS (INTERVAL_SECOND / INTERVAL_UPDATE / 2)

LPD8806 strip = LPD8806(LED_COUNT);
#define DARK (strip.Color(0, 0, 0))

// Current time variables
byte minute = 15; // start clock at 15 minutes past
byte second = 0;
byte minute_hand_brightness = 0;
boolean minute_hand_rising = true;

// Color variables
long colorMinutes = 0;
long colorHours = 0;

// Timing variables
long currentMillis = 0;
long previousSecond = 0;
long previousUpdate = 0;

void setup() {
  strip.begin();
  Serial.begin(9600);
  Serial.println("[circleclock_animation]");
}

void loop() {
  if (currentMillis - previousSecond >= INTERVAL_SECOND) {
    previousSecond = currentMillis;  // One second passed, set timer for next
    second = ++second % SECONDS_PER_MINUTE;
    if (second == 0) {
      minute = ++minute % MINUTES;
      minute_hand_brightness = 0;
      minute_hand_rising = true;
      Serial.println("New minute");
    }
  }
  currentMillis = millis();
  if (currentMillis - previousUpdate >= INTERVAL_UPDATE) {
    previousUpdate = currentMillis; // Did a color update, set timer for next
    DisplayMinutes();
  }  
}

void DisplayMinutes() {
  //TODO: Get the correct color as rgb struct WITHOUT the brightness in there
  // This way the brightness can be factored in later, removing the need for two
  // separate color functions (Minute and Hand)
  colorMinutes = MinuteColor();
  for (byte i = 0; i < LED_COUNT; i++) {
    if (i <= minute) {
      strip.setPixelColor(i, colorMinutes);
    } else {
      strip.setPixelColor(i, DARK);
    }
  }
  if (second == (SECONDS_PER_MINUTE - 1)) {
    minute_hand_brightness = min(++minute_hand_brightness, MINUTE_HAND_STEPS);
  } else if (minute_hand_rising) {
    if (++minute_hand_brightness == MINUTE_HAND_STEPS) {
      minute_hand_rising = false;
    }
  } else {
    if (--minute_hand_brightness == 0) {
      minute_hand_rising = true;
    }
  }
  strip.setPixelColor(minute, HandColor());
  strip.show();
}

long MinuteColor() {
  long secondsPast = second + minute * SECONDS_PER_MINUTE;
  return ColorWheel((secondsPast * 768 / SECONDS_PER_HOUR) % 768, BRIGHTNESS);
}

long HandColor() {
  long secondsPast = second + minute * SECONDS_PER_MINUTE;
  byte effectiveBrightness = minute_hand_brightness * BRIGHTNESS / MINUTE_HAND_STEPS;
  return ColorWheel((secondsPast * 768 / SECONDS_PER_HOUR) % 768, effectiveBrightness);
}

long ColorWheel(int angle, byte brightness) {
  // Returns a color out of 768 possible angle fractions of a color wheel
  int r, g, b;
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
  r = r * brightness / 100;
  g = g * brightness / 100;
  b = b * brightness / 100;
  return(strip.Color(r,g,b));
}

