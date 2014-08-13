#include <LPD8806.h>
#include <SPI.h>
#include <EasyTransfer.h>

//
// RGB LED strip
// -------------
//
// Uses the SerialLedBuffered protocol to set the colors on an LPD8806 RGB LED
// strip.
//
// Because the LPD8806 chip supports 7-bit per color channel, the input color
// values are scaled from the range [0, 255] to the range [0, 127] by dividing
// the color value by 2.
//
// The LPD8806 library and SPI library are used to set the colors on the strip.
//
// The EasyTransfer library is used for receiving the binary data.
//
// SerialLedBuffered protocol:
// https://github.com/oddguid/visualstatus-protocol/blob/master/SerialLedBuffered.md
//

// -----------------------------------------------------------------------------
// DEFINES
// -----------------------------------------------------------------------------
#define DATA_PIN            2              // data pin of LED strip
#define CLOCK_PIN           3              // clock pin of LED strip
#define NUM_LEDS            32             // 1 meter has 32 RGB LEDs

#define SERIAL_SPEED        115200         // speed for serial port

#define STRIP_BRG           0              // set to 0 if LED strip is common
                                           // GRB, set to 1 for BRG strip

#define COLOR_BUFFER_SIZE   (NUM_LEDS * 3) // buffer size for colors

// -----------------------------------------------------------------------------
// GLOBALS
// -----------------------------------------------------------------------------

// transport structure
struct RECEIVE_DATA_STRUCTURE
{
  byte rgb[COLOR_BUFFER_SIZE];             // RGB triplets
};

RECEIVE_DATA_STRUCTURE receiveBuffer;

LPD8806 strip = LPD8806(NUM_LEDS, DATA_PIN, CLOCK_PIN);

EasyTransfer easyTransfer;

// -----------------------------------------------------------------------------
// FUNCTIONS
// -----------------------------------------------------------------------------
void setColorFromBuffer()
{
  // write color from buffer
  byte index = 0;

  for (byte i = 0; i < NUM_LEDS; ++i)
  {
    byte r = receiveBuffer.rgb[index++] >> 1;
    byte g = receiveBuffer.rgb[index++] >> 1;
    byte b = receiveBuffer.rgb[index++] >> 1;

    // update pixel
#if STRIP_BRG > 0
    // LPD8806 library expects GRB LED strips: swap green and
    // blue for BRG LED strips
    strip.setPixelColor(i, r, b, g);
#else
    strip.setPixelColor(i, r, g, b);
#endif
  }

  strip.show();
}

void setup()
{
  // initialize LPD8806 strip, all pixels 'off'
  strip.begin();
  strip.show();

  // setup serial port
  Serial.begin(SERIAL_SPEED);

  // setup EasyTransfer
  easyTransfer.begin(details(receiveBuffer), &Serial);

  // report ready
  Serial.println("!");
}

void loop()
{
  if (easyTransfer.receiveData())
  {
    // update colors
    setColorFromBuffer();

    // write response
    Serial.println("!");
  }
}
