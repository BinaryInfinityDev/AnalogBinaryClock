#pragma once
#include <FastLED.h>
#include <Time.h>

#define FRACTIONAL_BRIGHTNESS 255
#define MOD(a, b) ((((a) % (b)) + (b)) % (b))
#define POSITION(offset, order, period, pixels, milli) (MOD((offset) + ((milli) * (order)) % (period) * (pixels) / (period), (pixels)))
#define SPECTRUM_CYCLE_PERIOD_SECONDS 8

enum Direction { FORWARD = 1, BACKWARD = -1, CLOCKWISE = 1, COUNTERCLOCKWISE = -1 };
enum HandType { NONE = 0, SMALL_BAR = 1, MEDIUM_BAR = 2, LARGE_BAR = 4, RIPPLE = 11 };

struct Instant {
  time_t currentMinute;
  unsigned long startOfMinuteMillis;
};

class LeapTime {
  time_t _leapMinute;
  int _leapOffset;

 public:
  LeapTime();
  LeapTime(time_t leapMinute, int leapOffset);
  int secondsInMinute(Instant instant) const;
  int secondsInHour(Instant instant) const;
  unsigned long millisSinceStartOfDay(Instant instant) const;
  static unsigned long millisSinceStartOfHour(Instant instant);
};

struct Strand {
  /* Configuration for a single strand in the clock */
  CRGB* strand;
  int numPixels;

  int secondsPeriod;
  int minutesPeriod;
  int hoursPeriod;

  int numPixelFractions = 255;
  int offset = 0;
  Direction order = CLOCKWISE;
  Strand(int numPixels, int secondsPeriod, int minutesPeriod, int hoursPeriod);
  Strand(int numPixels, int secondsPeriod, int minutesPeriod, int hoursPeriod, int offset, Direction order);
};

class AnalogBinaryClock {
 private:
  Strand* _strands;
  int _numStrands;
  HandType _secondHandType = RIPPLE, _minuteHandType = SMALL_BAR, _hourHandType = LARGE_BAR;
  LeapTime _leapTime = LeapTime();

  void updateStrand(Strand strand, Instant instant, unsigned long now, int hueOffset);
  static void updateStrandBar(Strand strand, int width, int position, const CHSV& color);
  static void updateStrandRipple(Strand strand, int width, int duration, unsigned long millis, int position, const CHSV& color);

 public:
  AnalogBinaryClock(Strand* strands, int numStrands);
  AnalogBinaryClock(Strand* strands, int numStrands, HandType secondHandType, HandType minuteHandType, HandType hourHandType);
  void update(Instant instant);
  void setLeapTime(LeapTime leapTime);
};
