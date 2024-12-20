#include <stdio.h>    // Used for printf() statements
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include <softPwm.h>
#include <wiringPi.h> // Include WiringPi library!
#include <sys/time.h>
#include <math.h>

#include <time.h>

// Pin number declarations. We're using the Broadcom chip pin numbers.

#define PIN_trigger  6  //GPIO17 - out - trigger 
#define PIN_echo     5  //GPIO27 - in - echo

#define PIN_infrared   25  //GPIO_P1_22 - in - infrared  

#define PIN_free    0  //GPIO_P1_11 - in

#define PIN_button 17  //GPIO_P1_15 - in - pulsador

#define PIN_led_1    23  //GPIO_P1_16 - out - luz 1
#define PIN_led_2    24  //GPIO_P1_18 - out - luz 2

#define SERVO_PIN    13 //GPIO13

#define INTERVAL 300000       // Interval in microseconds
#define MAX_TIME 11660        // Max time for echo in microseconds
#define SOUND_SPEED 34300.0   // Speed of sound in cm/s

#define M_PI 3.14159265358979323846 

int fd;

const int pwmValue = 75; 

float ultimaDistancia = 0.0;
struct timespec ultimoTiempo;

static pthread_mutex_t mutex_S; // mutex for sporadic task

struct timeval start, finish, start_time, finish_time, sr_in_time, sr_out_time;
long elapsed_time, shared_resource_time, exec_time, startTime, endTime;
float distance, speed;
int echo = 0;
bool symptom = false;

void init_devices ()
{
  system("gpio load spi");

  wiringPiSetup(); // Initialize wiringPi -- using Broadcom? pin numbers

  if (wiringPiSPISetup (0, 1000000) < 0)// Configuring conexion to 0.5 MHz
  {
     printf("error ocurrido \n");
     fprintf (stderr, "Unable to open SPI device 0: %s\n", strerror (errno)) ;
     exit (1) ;
  }

  if (wiringPiSetupGpio() == -1) {
     printf("Error: wiringPi setup failed\n");
     return 1;
  }
	
  pinMode(PIN_infrared, INPUT);
  pinMode(PIN_echo, INPUT);     // echo ultrasound 
  pinMode(PIN_trigger, OUTPUT); // trigger ultrasound 
  pinMode(PIN_led_1, OUTPUT);
  pinMode(PIN_led_2, OUTPUT);
  pinMode(PIN_button, INPUT);
  pinMode(SERVO_PIN, PWM_OUTPUT);

  pinMode(PIN_free, INPUT);  // free GPIO an input

  // SERVOMOTOR CONFIGURATION 
  softPwmCreate(SERVO_PIN, 0, 200); // Create PWM in range 0..200

  // Setting the GPIO pins mode  
  digitalWrite(PIN_trigger, LOW);
  
  // Configure accelerometer in I2C    
  fd = wiringPiI2CSetup (0x68);
  wiringPiI2CWriteReg8 (fd,0x6B,0x00);//disable sleep mode
  wiringPiI2CWriteReg8 (fd,0x1C,0x10);
  wiringPiI2CWriteReg8 (fd,0x1B,0x08); 

  printf("devices: pinouts configured \n");
}

// ANALOG SENSORS - ADC  ///////////////////////////

int read_single_ADC_sensor (int analog_channel) {
  int ADC=-1;
  if((analog_channel>=0)&&(analog_channel<=7))
  {
    int ce = 0;
    unsigned char ByteSPI[7];

    // loading data
    ByteSPI[0] = 0b01;//The last bit is the start signal 
    ByteSPI[1]=(0x80)|(analog_channel<<4);//4 bits to configure the mode
    ByteSPI[2]=0;// 8 bit to write the result of analog reading 
    wiringPiSPIDataRW (ce, ByteSPI, 3);// we send the order
    usleep(20);// waiting 20 microseconds 

    ADC=((ByteSPI[1]&0x03)<<8)|ByteSPI[2];// we take the data 
  }
  return (ADC);
}

int read_all_ADC_sensors (int values[])
{
  int i,analog;

  for(i=0;i<8;i++){
      analog= read_single_ADC_sensor(i);
      values [i] = analog;
      delay(100);
  }
  return (0);
}

// DIGITAL DEVICES  ///////////////////////////

int set_led_1 (int Led_Value)
{
   digitalWrite(PIN_led_1, Led_Value);     
}

int set_led_2 (int Led_Value)
{
   digitalWrite(PIN_led_2, Led_Value); 
}
       
int read_button ()
{ 
 int valor;
 valor = digitalRead(PIN_button);
 return (valor);    
}

int read_infrared ()
{
 int valor;
 valor = digitalRead(PIN_infrared);
 return (valor); 
}

// ACCELEROMETER ///////////////////////////

double get_x_rotation(double x, double y, double z);
double get_y_rotation(double x, double y, double z);
double dist(double a, double b);
int read_word_2c(int addr);

void Inicializar_acelerometro ()
{
    fd = wiringPiI2CSetup (0x68);
    wiringPiI2CWriteReg8 (fd,0x6B,0x00); //disable sleep mode
    wiringPiI2CWriteReg8 (fd,0x1C,0x10);
    wiringPiI2CWriteReg8 (fd,0x1B,0x08);
}


double dist(double a, double b)
{
    return sqrt((a*a) + (b*b));
}

int read_word_2c(int addr)
{
    int val;
    val = wiringPiI2CReadReg8(fd, addr);
    val = val << 8;
    val += wiringPiI2CReadReg8(fd, addr+1);
    if (val >= 0x8000)
      val = -(65536 - val);

    return val;
}

double get_y_rotation(double x, double y, double z)
{
  double radians;
  radians = atan2(x, dist(y, z));
  return -(radians * (180.0 / M_PI));
}

double get_x_rotation(double x, double y, double z)
{
  double radians;
  radians = atan2(y, dist(x, z));
  return (radians * (180.0 / M_PI));
}


int Read_Giroscope_X ()
{

int acclX, acclY, acclZ;
double acclX_scaled, acclY_scaled, acclZ_scaled;
  acclX = read_word_2c(0x3B);
  acclY = read_word_2c(0x3D);
  acclZ = read_word_2c(0x3F);

  acclX_scaled = acclX / 16384.0;
  acclY_scaled = acclY / 16384.0;
  acclZ_scaled = acclZ / 16384.0;

  return get_x_rotation(acclX_scaled, acclY_scaled, acclZ_scaled);

}

int Read_Giroscope_Y ()
{

int acclX, acclY, acclZ;
double acclX_scaled, acclY_scaled, acclZ_scaled;
  acclX = read_word_2c(0x3B);
  acclY = read_word_2c(0x3D);
  acclZ = read_word_2c(0x3F);

  acclX_scaled = acclX / 16384.0;
  acclY_scaled = acclY / 16384.0;
  acclZ_scaled = acclZ / 16384.0;

  return get_y_rotation(acclX_scaled, acclY_scaled, acclZ_scaled);

}

// ULTRASOUND ///////////////////////////

long getMicrotime();

long getMicrotime()
{
   struct timeval currentTime;
   gettimeofday(&currentTime, NULL);
   return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

float getDistance()
{
   digitalWrite(PIN_trigger, HIGH); 
   delayMicroseconds(10);
   digitalWrite(PIN_trigger, LOW); 
   
   // printf ("... Ultrasound waiting for echo ...\n"); 
   while(digitalRead(PIN_echo) == LOW);
   startTime = getMicrotime();
   while(digitalRead(PIN_echo) == HIGH);
   endTime = getMicrotime();

   distance = (endTime - startTime) * 0.034 / 2;

   return distance;
}

// SERVOMOTOR ///////////////////////////

// Move servo continous movement
void setServoPulse(int pulseWidth) {
    pwmWrite(SERVO_PIN, pulseWidth);
}

// Move the servo motor to a specific angle between 0 and 180 degrees
void moveServo(int angle) {
    int dutyCycle;

    // Transform angle to duty cycle for PWM
    // 5 = 0 degrees ; 25 = 180 degrees
    dutyCycle = 5 + (angle * 20 / 180);
    
    // Send PWm pulse 
    softPwmWrite(SERVO_PIN, dutyCycle);

    // Espera un poco para que el servo alcance la posición
    delay(500);
}

// CLOSE DEVICES ///////////////////////////

int close_devices ()
{
   int n;
   printf("devices: Closing devices \n");
   n = set_led_1 (0); // Led_1 off
   n = set_led_2 (0); // Led_2 off
   setServoPulse(0);
}
