#include <stdio.h>    // Used for printf() statements
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <wiringPi.h> // Include WiringPi library!
#include <time.h>

// call this function to start working with devices
int init_devices();

// functions to read analog channels
int read_single_ADC_sensor (int analog_channel); // returs digital value of the channel
int read_all_ADC_sensors (int values[]); // returs vector with 8 values of the 8 channels

// functions to turn on/off leds
int set_led_1 (int Led_Value); // Led_Value = 0 (off), 1 (on)
int set_led_2 (int Led_Value);

// functions to read digital inputs 
int read_button (); // returns 1 (button pressed) or 0 (no pressed)
int read_infrared (); // returns 0 (object detected) or 1 (no detected)

// function to move servomotor 
// void setServoPulse(int pulseWidth);  // move servo continous movement
void moveServo(int angle); // move servo 180 degrees

//function to read accelerometer
int Read_Giroscope_X ();
int Read_Giroscope_Y ();

//function to calculate distance
float getDistance();

// call this function before finishing the program
int close_devices ();

