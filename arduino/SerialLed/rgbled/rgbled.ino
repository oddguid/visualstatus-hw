#include <SoftPWM.h>
#include <TimerOne.h>

//
// RGB LED (common anode)
// ----------------------
//
// Uses the SerialLed protocol to set the colors of 3 RGB common anode LEDs.
//
// LED0 uses pins  9 (red)
//                10 (blue)
//                11 (green)
// LED1 uses pins  6 (red)
//                 8 (blue)
//                 7 (green)
// LED2 uses pins  3 (red)
//                 5 (blue)
//                 4 (green)
//
// The common anode of the LEDs is connected to 5V on the arduino.
//
// The SoftPWM library is used to enable PWM on the used pins and since the LEDs
// are common anode, the library is initialized with SOFTPWM_INVERTED.
//
// The TimerOne library is used with a callback function that will set the
// color on the LEDs. The interval for the timer is 10ms which allows for a
// refresh rate of 100Hz.
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
#define NUM_LEDS            3              // number of LEDs
#define DEFAULT_DELAY       500            // 500 ms: 2 toggles/s
#define MIN_DELAY           100            // minimum delay

#define SERIAL_SPEED        115200         // speed for serial port

#define SERIAL_BUFFER_SIZE  32             // buffer size for serial commands
#define COLOR_BUFFER_SIZE   (NUM_LEDS * 3) // buffer size for colors and pins

// -----------------------------------------------------------------------------
// GLOBALS
// -----------------------------------------------------------------------------
char serialBuffer[SERIAL_BUFFER_SIZE];
byte serialBufferIndex;

byte colorBuffer[2][COLOR_BUFFER_SIZE];
byte colorMode;

word toggleDelay;
byte displayMode;

byte pinBuffer[COLOR_BUFFER_SIZE] =
  { 9, 11, 10,    // LED 0, r-b-g
    6,  8,  7,    // LED 1, r-b-g
    3,  5,  4     // LED 2, r-b-g
  };

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

void callback()
{
  if (displayMode == 1)
  {
    // write colors using PWM
    for (int i = 0; i < COLOR_BUFFER_SIZE; ++i)
    {
      // output current color
      SoftPWMSet(pinBuffer[i], int(colorBuffer[colorMode][i]));
    }
  }
  else
  {
    // write no color
    for (int i = 0; i < COLOR_BUFFER_SIZE; ++i)
    {
      SoftPWMSet(pinBuffer[i], int(0));
    }
  }
}

void setup()
{
  // clear serial buffer
  clearSerialBuffer();

  // clear color buffer
  clearColorBuffer();

  // initialize PWM library
  SoftPWMBegin(SOFTPWM_INVERTED);

  for (int i = 0; i < COLOR_BUFFER_SIZE; ++i)
  {
    SoftPWMSet(pinBuffer[i], 0);
  }

  // initialize timer for interrupt
  // 10000 us = 10ms ~ 100Hz
  Timer1.initialize(10000);
  Timer1.attachInterrupt(callback);

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

  // wait until toggle delay has passed
  long waitTime = toggleDelay - long(millis() - startTime);

  if (waitTime > 0)
  {
    delay(waitTime);
  }

  // toggle delay has passed, switch color buffer
  colorMode ^= 1;
}
