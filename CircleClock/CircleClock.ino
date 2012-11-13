#include <LPD8806.h>
#include <SPI.h>

// Basic times and counts
#define LED_COUNT (42)
#define MINUTES (LED_COUNT)
#define SECONDS_HOUR (MINUTES * 60)

// Intervals
#define INTERVAL_SECOND (1000)
#define INTERVAL_UPDATE (50)

LPD8806 strip = LPD8806(LED_COUNT);
#define DARK (strip.Color(0, 0, 0))

// Current time variables
byte minute = 24;
byte second = 0;
boolean tick = false;
// Color variables
long minuteColor = 0;
long hourColor = 0;
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
  if (currentMillis - previousSecond > INTERVAL_SECOND) {
    previousSecond = currentMillis;  // One second passed, set timer for next
    second = ++second % 60;
    tick = true;
    if (second == 0) {
      minute = ++minute % MINUTES;
    }
  }
  currentMillis = millis();
  if (currentMillis - previousUpdate > INTERVAL_UPDATE) {
    previousUpdate = currentMillis; // Did a color update, set timer for next
    DisplayMinutes();
  }  

}

void DisplayMinutes() {
  minuteColor = MinuteColor();
  for (byte i = 0; i < LED_COUNT; i++) {
    if (i <= minute) {
      strip.setPixelColor(i, minuteColor);
    } else {
      strip.setPixelColor(i, DARK);
    }
  }
  if (tick) {
    tick = false;
    strip.setPixelColor(minute, DARK);
  }  
  strip.show();
}

long MinuteColor() {
  long secondsPast = second + minute * 60;
  return ColorWheel((secondsPast * 768 / SECONDS_HOUR) % 768);
}

long ColorWheel(int angle) {
  // Returns a color out of 768 possible angle fractions of a color wheel
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
  return(strip.Color(r,g,b));
}

