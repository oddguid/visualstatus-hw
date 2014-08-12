#include <SoftPWM.h>
#include <TimerOne.h>
#include <EasyTransfer.h>

//
// RGB LED (common anode)
// ----------------------
//
// Uses the SerialLedBuffered protocol to set the colors of 3 RGB common anode
// LEDs.
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
// The EasyTransfer library is used for receiving the binary data.
//
// SerialLedBuffered protocol:
// https://github.com/oddguid/visualstatus-protocol/blob/master/SerialLedBuffered.md
//

// -----------------------------------------------------------------------------
// DEFINES
// -----------------------------------------------------------------------------
#define NUM_LEDS            3              // number of LEDs

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

byte colorBuffer[COLOR_BUFFER_SIZE];

EasyTransfer easyTransfer;

byte pinBuffer[COLOR_BUFFER_SIZE] =
  { 9, 11, 10,    // LED 0, r-b-g
    6,  8,  7,    // LED 1, r-b-g
    3,  5,  4     // LED 2, r-b-g
  };

// -----------------------------------------------------------------------------
// FUNCTIONS
// -----------------------------------------------------------------------------
void setColorFromBuffer()
{
  // copy to color buffer
  memcpy(colorBuffer, receiveBuffer.rgb, COLOR_BUFFER_SIZE);
}

void callback()
{
  // write colors using PWM
  for (int i = 0; i < COLOR_BUFFER_SIZE; ++i)
  {
    // output current color
    SoftPWMSet(pinBuffer[i], int(colorBuffer[i]));
  }
}

void setup()
{
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
