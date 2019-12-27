#include <MD_YX5300.h>

#include <Adafruit_NeoPixel.h>

#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>

#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//Control
bool saberOn = false;

//Accelerometer
MPU6050 accelgyro;
int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t oldAx, oldAy, oldAz;

const int SWING_THRESHOLD = 3000;
const int CLASH_THRESHOLD = 5000;
const int CLASH_LOOP_COUNT = 3;

int clashLoopCount = 0;

#define BAUD_RATE 9600
//Lights
#define LIGHT_PIN 9
#define NUM_LIGHTS 7
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LIGHTS, LIGHT_PIN, NEO_GRB);
uint32_t black = strip.Color(0, 0, 0);
uint32_t blue = strip.Color(50, 50, 200);
uint32_t light_blue = strip.Color(50, 50, 150);
uint32_t white = strip.Color(50, 50, 100);
uint32_t white_blue = strip.Color(50, 50, 200);

int light_loop = 0;

//Audio
uint8_t  FOLDER  = 1;
uint8_t  TRACK_SABER_ON  = 1;
uint8_t  TRACK_SABER_HUM_A  = 2;
uint8_t  TRACK_SABER_HUM_B  = 3;
uint8_t  TRACK_SABER_SWING_A = 4;
uint8_t  TRACK_SABER_SWING_B = 5;
uint8_t  TRACK_SABER_CLASH_A = 6;
uint8_t  TRACK_SABER_CLASH_B = 7;
uint8_t  TRACK_SABER_OFF = 8;
uint8_t  EQUALIZER_MODE = 5;
uint8_t  VOLUME = 30;
#define ARDUINO_RX 0//should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 1//connect to RX of the module
MD_YX5300 mp3(ARDUINO_RX, ARDUINO_TX);
boolean audioPlaying = false;
uint8_t trackQueue = 0;
int trackDelay = 3;
int lastPlayedCount = 0;
boolean mandatoryTrackPlaying = false;
int audioPlayingCount = 0;
int audioPlayingLimit = 500;

//Buttons
const int  buttonPin = 2;
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

//************************Lights***************************//
void initializeLights()
{
  strip.begin();
  strip.show();
}

void checkLights()
{
  if (light_loop == 20) {
    setPixels(0, blue);
  }

  light_loop = light_loop + 1;

  if (light_loop == 860) {
    setPixels(0, light_blue);
    light_loop = 0;
  }
}

void setPixels(int wait, uint32_t color)
{

  for (int i = 0; i < NUM_LIGHTS; i++)
  {
    strip.setPixelColor(i, color);
    delay(wait);
    if (delay > 0) {
      strip.show();
    }
  }

  strip.show();
}

void setPixelsReverse(int wait, uint32_t color)
{

  for (int i = NUM_LIGHTS; i >= 0; i--)
  {
    strip.setPixelColor(i, color);
    delay(wait);
    if (delay > 0) {
      strip.show();
    }
  }

  strip.show();
}

//************************Audio***************************//

void initializeAudio()
{
  mp3.begin();
  mp3.setTimeout(1000);
  mp3.setCallback(cbResponse);
  mp3.setSynchronous(false);
  mp3.equalizer(EQUALIZER_MODE);
  mp3.volume(VOLUME);
}

void cbResponse(const MD_YX5300::cbData *status)
{
  if (status->code != MD_YX5300::STS_ACK_OK)
  {
    lastPlayedCount++;
    if (lastPlayedCount >= trackDelay || status->code == MD_YX5300::STS_FILE_END) {
      lastPlayedCount = 0;
      audioPlaying = false;
      mandatoryTrackPlaying = false;
      Serial.println("audioPlaying = false");
    }
  }
}

void checkAudio(boolean override) {

  audioPlayingCount++;

  if(audioPlayingCount > audioPlayingLimit){
    mandatoryTrackPlaying = false;
  }

  if (override && !mandatoryTrackPlaying) {
    Serial.println("\nPlaying Track" + trackQueue);
    mp3.playSpecific(FOLDER, trackQueue);
    trackQueue = 0;
    Serial.println("\naudioPlaying = true");
    audioPlaying = true;
  } else if (!audioPlaying) {
    Serial.println("\naudioPlaying hum");
    mp3.playSpecific(FOLDER, TRACK_SABER_HUM_A);
    audioPlaying = true;
  }
}

void playAudio(uint8_t track) {
  trackQueue = track;
}

//************************Gyro***************************//
void initializeGyro()
{
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  Serial.begin(BAUD_RATE);
  accelgyro.initialize();
}

void checkMovement()
{
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  if ( clashLoopCount >= CLASH_LOOP_COUNT)
  {
    if ( abs(ax) - oldAx > CLASH_THRESHOLD ||  abs(ay) - oldAy > CLASH_THRESHOLD || abs(az) - oldAz > CLASH_THRESHOLD )
    {
      clash();
    }

    oldAx = abs(ax);
    oldAy = abs(ay);
    oldAz = abs(az);

    clashLoopCount = 0;
  }

  if ( abs(ax) - oldAx > SWING_THRESHOLD ||  abs(ay) - oldAy > SWING_THRESHOLD || abs(az) - oldAz > SWING_THRESHOLD )
  {

    swing();
  }

  clashLoopCount = clashLoopCount + 1;
}

//Buttons
void checkForButtonPush() {
  buttonState  = digitalRead(buttonPin);
  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      saberOn = !saberOn;

      if (saberOn) {
        powerOn();
      } else {
        powerOff();
      }

    }
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;
}

//************************Control***************************//

void clash()
{
  long randomNumber = random(10, 20);

  if (randomNumber > 15) {
    playAudio(TRACK_SABER_CLASH_A);
    Serial.println("\nCLASH SOUNDS!  RAWWWW!\n");
    setPixels(0, white);
  } else {
    playAudio(TRACK_SABER_CLASH_B);
    Serial.println("\nCLASH SOUNDS!  BRRRREEE!\n");
    setPixels(0, white_blue);
  }
  light_loop = 0;
  checkAudio(true);
  mandatoryTrackPlaying = true;
}

void swing()
{
  Serial.println("\nSWIIIIING!\n");

  long randomNumber = random(10, 20);

  if (randomNumber > 15) {
    playAudio(TRACK_SABER_SWING_A);
    Serial.println("\nCLASH SOUNDS!  RAWWWW!\n");
    setPixels(0, white);
  } else {
    playAudio(TRACK_SABER_SWING_B);
    Serial.println("\nCLASH SOUNDS!  BRRRREEE!\n");
    setPixels(0, white_blue);
  }
  light_loop = 0;
  checkAudio(true);
  mandatoryTrackPlaying = true;
}

void powerOn() {
  Serial.println("\n**********POWER ON**********\n");
  Serial.println("\naudioPlaying = false");
  audioPlaying = false;
  saberOn = true;
  playAudio(TRACK_SABER_ON);
  checkAudio(true);
  mandatoryTrackPlaying = true;
  setPixels(100, blue);
}

void powerOff() {
  Serial.println("\n**********POWER OFF**********\n");
  saberOn = false;
  playAudio(TRACK_SABER_OFF);
  checkAudio(true);
  mandatoryTrackPlaying = true;
  setPixelsReverse(100, black);
  audioPlaying = false;
}

void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println("\nBooting Up...");

  //Setup the Gyro
  initializeGyro();

  //Setup the Lgiths
  initializeLights();

  //Get the On/Off button ready
  pinMode(buttonPin, INPUT);

  //Setup the audio
  initializeAudio();
}

void loop()
{
  if (saberOn) {
    checkMovement();
    checkAudio(false);
    checkLights();
  }
  mp3.check();
  checkForButtonPush();
}
