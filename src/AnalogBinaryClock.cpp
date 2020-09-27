//
// Created by Shawn Kovalchick on 9/14/20.
//

#include AnalogBinaryClock.hpp

LeapTime::LeapTime() : _leapMinute(0), _leapOffset(0) {}

LeapTime::LeapTime(time_t leapMinute, int leapOffset) : _leapMinute(leapMinute), _leapOffset(leapOffset) {}

int LeapTime::secondsInMinute(Instant instant) const {
  return 60 + (instant.currentMinute % 60 == _leapMinute % 60 ? _leapOffset : 0);
}

int LeapTime::secondsInHour(Instant instant) const {
  return 3600 + (instant.currentMinute % 3600 == _leapMinute % 3600 ? _leapOffset : 0);
}

unsigned long LeapTime::millisSinceStartOfDay(Instant instant) const {
  return hour(instant.currentMinute) * secondsInHour(instant) * 1000 + millisSinceStartOfHour(instant);
}

unsigned long LeapTime::millisSinceStartOfHour(Instant instant) {
  return (instant.currentMinute % 3600) * 1000 + millis() - instant.startOfMinuteMillis;
}

Strand::Strand(int numPixels, int secondsPeriod, int minutesPeriod, int hoursPeriod) : strand(new CRGB[numPixels]), numPixels(numPixels), secondsPeriod(secondsPeriod), minutesPeriod(minutesPeriod), hoursPeriod(hoursPeriod) {}

Strand::Strand(int numPixels, int secondsPeriod, int minutesPeriod, int hoursPeriod, int offset, Direction order)
    : strand(new CRGB[numPixels]), numPixels(numPixels), secondsPeriod(secondsPeriod), minutesPeriod(minutesPeriod), hoursPeriod(hoursPeriod), offset(offset), order(order) {}

AnalogBinaryClock::AnalogBinaryClock(Strand* strands, int numStrands) : _strands(strands), _numStrands(numStrands) {}

AnalogBinaryClock::AnalogBinaryClock(Strand* strands, int numStrands, HandType secondHandType, HandType minuteHandType, HandType hourHandType)
    : _strands(strands), _numStrands(numStrands), _secondHandType(secondHandType), _minuteHandType(minuteHandType), _hourHandType(hourHandType) {}

void AnalogBinaryClock::update(Instant instant) {
  unsigned long now = millis();
  for (int i = 0; i < _numStrands; i++) {
    updateStrand(_strands[i], instant, now, i * 360 / _numStrands);
  }
  FastLED.show();
  for (int i = 0; i < _numStrands; i++) {
    for (int j = 0; j < _strands[i].numPixels; j++) {
      _strands[i].strand[j] = CRGB::Black;
    }
  }
}

void AnalogBinaryClock::setLeapTime(LeapTime leapTime) {
  _leapTime = leapTime;
}

void AnalogBinaryClock::updateStrand(Strand strand, Instant instant, unsigned long now, int hueOffset) {
  // Determine color offset (PERIOD_IN_SECONDS * 1024 milliseconds / 256 colors in range)
  uint8_t cycleOffset = (millis() / (SPECTRUM_CYCLE_PERIOD_SECONDS * 4)) % 256;

  unsigned long millisSinceStartOfDay = _leapTime.millisSinceStartOfDay(instant);
  unsigned long millisSinceStartOfHour = _leapTime.millisSinceStartOfHour(instant);
  unsigned long millisSinceStartOfMinute = (now - instant.startOfMinuteMillis) % 1000;

  int hourPosition = POSITION(strand.offset + (1 - _hourHandType) * strand.numPixelFractions / 2, strand.order, strand.hoursPeriod * _leapTime.secondsInHour(instant), strand.numPixels * strand.numPixelFractions, millisSinceStartOfDay);
  int minutePosition =
      POSITION(strand.offset + (1 - _minuteHandType) * strand.numPixelFractions / 2, strand.order, strand.minutesPeriod * _leapTime.secondsInMinute(instant), strand.numPixels * strand.numPixelFractions, millisSinceStartOfHour);
  int secondPosition = POSITION(strand.offset + (1 - _secondHandType) * strand.numPixelFractions / 2, strand.order, strand.secondsPeriod, strand.numPixels * strand.numPixelFractions, millisSinceStartOfMinute);

  CHSV hourColor = CHSV((hueOffset + cycleOffset) % 256, 255, 255);
  CHSV minuteColor = CHSV((hueOffset + cycleOffset + 85) % 256, 255, 255);
  CHSV secondColor = CHSV((hueOffset + cycleOffset + 170) % 256, 255, 255);

  if (strand.hoursPeriod > 0) {
    switch (_hourHandType) {
      case NONE:
        break;
      case RIPPLE:
        updateStrandRipple(strand, RIPPLE, 500, millisSinceStartOfMinute, minutePosition, hourColor);
        break;
      default:
        updateStrandBar(strand, _hourHandType, hourPosition, hourColor);
    }
  }
  if (strand.minutesPeriod > 0) {
    switch (_minuteHandType) {
      case NONE:
        break;
      case RIPPLE:
        updateStrandRipple(strand, RIPPLE, 500, millisSinceStartOfMinute, hourPosition, minuteColor);
        break;
      default:
        updateStrandBar(strand, _minuteHandType, minutePosition, minuteColor);
    }
  }
  if (strand.secondsPeriod > 0) {
    switch (_secondHandType) {
      case NONE:
        break;
      case RIPPLE:
        updateStrandRipple(strand, RIPPLE, 500, millisSinceStartOfMinute, minutePosition, secondColor);
        break;
      default:
        updateStrandBar(strand, _minuteHandType, secondPosition, secondColor);
    }
  }
}

void AnalogBinaryClock::updateStrandBar(Strand strand, int width, int position, const CHSV& color) {
  int wholePosition = position / strand.numPixelFractions;
  int fractionalPosition = position % strand.numPixelFractions;
  CHSV handRender[width + 1];
  for (auto& i : handRender) {
    i = color;
  }

  handRender[0].v = strand.numPixelFractions - fractionalPosition;
  handRender[width].v = fractionalPosition;
  for (int i = 0; i <= width; i++) {
    strand.strand[(wholePosition + i * strand.order) % strand.numPixels].fadeToBlackBy(handRender[i].v);
    strand.strand[(wholePosition + i * strand.order) % strand.numPixels] += handRender[i];
  }
}

void AnalogBinaryClock::updateStrandRipple(Strand strand, int width, int duration, unsigned long millis, int position, const CHSV& color) {
  CHSV secondHandRender[width + 1];
  for (auto& i : secondHandRender) {
    i = color;
  }
  unsigned long secondHandHalfWidth = (width - 1) * strand.numPixelFractions / 2;

  unsigned long secondHandDiameter = secondHandHalfWidth * millis / duration;
  unsigned long lowerIndex = (secondHandHalfWidth - secondHandDiameter);
  unsigned long upperIndex = (secondHandHalfWidth + secondHandDiameter);
  secondHandRender[lowerIndex / strand.numPixelFractions].value += FRACTIONAL_BRIGHTNESS - lowerIndex % strand.numPixelFractions;
  secondHandRender[lowerIndex / strand.numPixelFractions + 1].value += lowerIndex % strand.numPixelFractions;
  secondHandRender[upperIndex / strand.numPixelFractions].value += FRACTIONAL_BRIGHTNESS - upperIndex % strand.numPixelFractions;
  secondHandRender[upperIndex / strand.numPixelFractions + 1].value += upperIndex % strand.numPixelFractions;
  for (int i = 0; i < width + 1; i++) {
    strand.strand[MOD(position / FRACTIONAL_BRIGHTNESS + i * strand.order, strand.numPixels)] += secondHandRender[i];
  }
}
