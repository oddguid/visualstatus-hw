#include <LivingColors.h>

//
// Living Colors
// -------------
//
// Uses the SerialLed protocol to set the colors on Philips Living Colors
// lights (generation 1). 
//
// The LivingColors library is used to set the colors on the lights.
//
// Note: this sketch turns the Living Colors lights on, but never turns them
// off. The 'off'-state is faked by setting the color to black. After running
// this sketch the lights should be properly turned off. 
//
// SerialLed protocol:
// https://github.com/oddguid/visualstatus-protocol/blob/master/SerialLed.md
//
// Command format summary:
//
// wXXX-R1R1R1G1G1G1B1B1B1-R2R2R2G2G2G2B2B2B2\n
//   w      -> Write color, can be upper case
//   XXX    -> LED number, 0 <= x <= 255.
//   R1R1R1 -> Red value of first color, 0 <= r <= 255
//   G1G1G1 -> Green value of first color, 0 <= g <= 255
//   B1B1B1 -> Blue value of first color, 0 <= b <= 255
//   R2R2R2 -> Red value of second color, 0 <= r <= 255
//   G2G2G2 -> Green value of second color, 0 <= g <= 255
//   B2B2B2 -> Blue value of second color, 0 <= b <= 255
//
// dTTTTT\n
//   d      -> Set delay between toggles in ms, can be upper case
//             500ms ^= 2 toggles/s
//   TTTTT  -> Delay in milliseconds, 100 <= t <= 65535
//
// c\n
//   c      -> Clear all colors, can be upper case
//
// mX\n
//   m      -> Set display mode, can be upper case
//   X      -> 0 to turn off, 1 to turn on
//
// response is !\n on success, ?\n on error.
//

// -----------------------------------------------------------------------------
// DEFINES
// -----------------------------------------------------------------------------
#define MOSI_PIN            11             // SPI master data out pin
#define MISO_PIN            12             // SPI master data in pin
#define SCK_PIN             13             // SPI clock pin
#define CS_PIN              10             // SPI slave select pin
#define NUM_LEDS            1              // number of lights
#define DEFAULT_DELAY       500            // 500 ms: 2 toggles/s
#define MIN_DELAY           250            // minimum delay

#define SERIAL_SPEED        115200         // speed for serial port

#define SERIAL_BUFFER_SIZE  32             // buffer size for serial commands
#define COLOR_BUFFER_SIZE   (NUM_LEDS * 3) // buffer size for colors

// -----------------------------------------------------------------------------
// GLOBALS
// -----------------------------------------------------------------------------
char serialBuffer[SERIAL_BUFFER_SIZE];
byte serialBufferIndex;

byte colorBuffer[2][COLOR_BUFFER_SIZE];
byte colorMode;

word toggleDelay;
byte displayMode;

LivingColors livingColors(CS_PIN, SCK_PIN, MOSI_PIN, MISO_PIN);

unsigned char lightAddress[9] = 
 { 0xDF, 0xF0, 0xE5, 0x27, 0x49, 0x5D, 0x9C, 0x8B, 0x11 };   // known address
 
// -----------------------------------------------------------------------------
// FUNCTIONS
// -----------------------------------------------------------------------------
byte value(char c)
{
  // convert digit
  byte val = 0;

  if (c >= '0')
  {
    if (c <= '9')
    {
      val = c - '0';
    }
  }

  return val;
}

byte value(char c1, char c2, char c3)
{
  // convert 3 digit number
  return value(c1) * 100 + value(c2) * 10 + value(c3);
}

word value(char c1, char c2, char c3, char c4, char c5)
{
  // convert 5 digit number
  return value(c1) * 10000 + value(c2) * 1000 + value(c3) * 100
       + value(c4) * 10 + value(c5);
}

void clearSerialBuffer()
{
  // clear serial buffer
  for (byte i = 0; i < SERIAL_BUFFER_SIZE; ++i)
  {
    serialBuffer[i] = 0;
  }

  serialBufferIndex = 0;
}

void clearColorBuffer()
{
  // clear both color buffers
  for (byte j = 0; j < 2; ++j)
  {
    for (byte i = 0; i < COLOR_BUFFER_SIZE; ++i)
    {
      colorBuffer[j][i] = 0;
    }
  }
}

void setColorFromSerialBuffer()
{
  // index = LED number
  byte index = value(serialBuffer[1], serialBuffer[2], serialBuffer[3]);

  if (index < NUM_LEDS)
  {
    index *= 3;

    // colors
    byte r1 = value(serialBuffer[ 5], serialBuffer[ 6], serialBuffer[ 7]);
    byte g1 = value(serialBuffer[ 8], serialBuffer[ 9], serialBuffer[10]);
    byte b1 = value(serialBuffer[11], serialBuffer[12], serialBuffer[13]);

    byte r2 = value(serialBuffer[15], serialBuffer[16], serialBuffer[17]);
    byte g2 = value(serialBuffer[18], serialBuffer[19], serialBuffer[20]);
    byte b2 = value(serialBuffer[21], serialBuffer[22], serialBuffer[23]);

    // set buffers
    colorBuffer[0][index    ] = r1;
    colorBuffer[0][index + 1] = g1;
    colorBuffer[0][index + 2] = b1;

    colorBuffer[1][index    ] = r2;
    colorBuffer[1][index + 1] = g2;
    colorBuffer[1][index + 2] = b2;
  }
}

void setDelayFromSerialBuffer()
{
  // convert delay
  toggleDelay = value(serialBuffer[1], serialBuffer[2], serialBuffer[3],
                      serialBuffer[4], serialBuffer[5]);

  if (toggleDelay < MIN_DELAY)
  {
    // cap to minimum
    toggleDelay = MIN_DELAY;
  }
}

void setDisplayModeFromSerialBuffer()
{
  // convert display mode
  displayMode = value(serialBuffer[1]);
}

void processCommand()
{
  char response = '!';

  // parse serial buffer for command
  switch (serialBuffer[0])
  {
  case 'w':
  case 'W':
    // set color
    setColorFromSerialBuffer();
    break;
  case 'd':
  case 'D':
    // set delay
    setDelayFromSerialBuffer();
    break;
  case 'c':
  case 'C':
    // clear color buffer
    clearColorBuffer();
    break;
  case 'm':
  case 'M':
    // set display mode
    setDisplayModeFromSerialBuffer();
    break;
  default:
    response = '?';
    break;
  }

  // write response
  Serial.println(response);

  // clear serial buffer
  clearSerialBuffer();
}

void setup()
{
  // clear serial buffer
  clearSerialBuffer();

  // clear color buffer
  clearColorBuffer();

  // initialize Living Colors lights
  livingColors.init();
  livingColors.clearLamps();
  livingColors.addLamp(lightAddress);

  // turn lights on
  for (byte i = 0; i < NUM_LEDS; ++i)
  {
    livingColors.turnLampOnRGB(i, 0, 0, 0);
  }

  colorMode = 0;
  toggleDelay = DEFAULT_DELAY;
  displayMode = 0;

  // setup serial port
  Serial.begin(SERIAL_SPEED);

  // report ready
  Serial.println("!");
}

void loop()
{
  unsigned long startTime = millis();

  // look for new serial data
  while (Serial.available())
  {
    char c = Serial.read();

    // store char in serial buffer
    serialBuffer[serialBufferIndex] = c;
    ++serialBufferIndex;

    // 0x0A or 0x0D signals end of command
    if ((c == 13) || (c == 10) || (serialBufferIndex >= SERIAL_BUFFER_SIZE))
    {
      // new command, process
      processCommand();
      break;
    }
  }

  if (displayMode == 1)
  {
    // write color from buffer
    byte index = 0;

    for (byte i = 0; i < NUM_LEDS; ++i)
    {
      byte r = colorBuffer[colorMode][index++];
      byte g = colorBuffer[colorMode][index++];
      byte b = colorBuffer[colorMode][index++];

      // update light
      livingColors.setLampColourRGB(i, r, g, b);
    }
  }
  else 
  {
    // fake 'off'-state by setting color to black
    for (byte i = 0; i < NUM_LEDS; ++i)
    {
      livingColors.setLampColourRGB(i, 0, 0, 0);
    }
  }

  // wait until toggle delay has passed
  long waitTime = toggleDelay - long(millis() - startTime);

  if (waitTime > 0)
  {
    delay(waitTime);
  }

  // toggle delay has passed, switch color buffer
  colorMode ^= 1;
}
