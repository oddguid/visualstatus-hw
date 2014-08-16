#include <EasyTransfer.h>
#include <LivingColors.h>

//
// Living Colors
// -------------
//
// Uses the SerialLedBuffered protocol to set the colors on Philips Living Colors
// lights (generation 1). 
//
// The LivingColors library is used to set the colors on the lights.
//
// The EasyTransfer library is used for receiving the binary data.
//
// Note: this sketch turns the Living Colors lights on, but never turns them
// off. After running this sketch the lights should be properly turned off. 
//
// SerialLedBuffered protocol:
// https://github.com/oddguid/visualstatus-protocol/blob/master/SerialLedBuffered.md
//

// -----------------------------------------------------------------------------
// DEFINES
// -----------------------------------------------------------------------------
#define MOSI_PIN            11             // SPI master data out pin
#define MISO_PIN            12             // SPI master data in pin
#define SCK_PIN             13             // SPI clock pin
#define CS_PIN              10             // SPI slave select pin
#define NUM_LEDS            1              // number of lights

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

EasyTransfer easyTransfer;

LivingColors livingColors(CS_PIN, SCK_PIN, MOSI_PIN, MISO_PIN);

unsigned char lightAddress[9] = 
 { 0xDF, 0xF0, 0xE5, 0x27, 0x49, 0x5D, 0x9C, 0x8B, 0x11 };   // known address
 
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

    // update light
    livingColors.setLampColourRGB(i, r, g, b);
  }
}

void setup()
{
  // initialize Living Colors lights
  livingColors.init();
  livingColors.clearLamps();
  livingColors.addLamp(lightAddress);

  // turn lights on
  for (byte i = 0; i < NUM_LEDS; ++i)
  {
    livingColors.turnLampOnRGB(i, 0, 0, 0);
  }

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
