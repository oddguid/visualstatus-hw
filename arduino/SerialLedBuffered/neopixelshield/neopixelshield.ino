#include <Adafruit_NeoPixel.h>
#include <EasyTransfer.h>

//
// NeoPixel shield
// ---------------
//
// Uses the SerialLedBuffered protocol to set the colors on an Adafruit
// NeoPixel shield.
//
// The Adafruit_NeoPixel library is used to set the colors on the shield.
//
// The EasyTransfer library is used for receiving the binary data.
//
// SerialLedBuffered protocol:
// https://github.com/oddguid/visualstatus-protocol/blob/master/SerialLedBuffered.md
//

// -----------------------------------------------------------------------------
// DEFINES
// -----------------------------------------------------------------------------
#define NEO_PIN             6              // data pin of shield
#define NUM_LEDS            40             // shield has 40 LEDs

#define SERIAL_SPEED        115200         // speed for serial port

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

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEO_PIN,
                                            NEO_GRB + NEO_KHZ800);

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
    byte r = receiveBuffer.rgb[index++];
    byte g = receiveBuffer.rgb[index++];
    byte b = receiveBuffer.rgb[index++];

    // update pixel
    strip.setPixelColor(i, r, g, b);
  }

  strip.show();
}

void setup()
{
  // initialize NeoPixel shield, all pixels 'off'
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
