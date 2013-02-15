CircleClock
===========

Software for an Arduino-powered clock. Hours and minutes are displayed on two colors rings made from full-RGB flexi-strip, minutes are displayed using a ring of 60 white LEDs similar to the hypnodisc project (https://github.com/edelooff/hypnodisc).

As time progresses, hour circle slowly lights up from the high point (noon/midnight) all the way around. The hour is indicated by the fraction of the ring that is lit. For examples, at three o'clock, a quarter of the outer ring is lit, the remainder of the ring is dark. The minute ring works in a similar way, being halfway filled at half past the hour.

As the rings light up further, their color also shifts. Much like the circular color pickers you see in many graphics applications, the hue shown on each ring is dependant on the time indicated. For example, when it's 9:15, the clock shows a purple minute ring inside an orange hour ring.
