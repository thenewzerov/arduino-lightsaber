#include <MD_YX5300.h>
#include <Adafruit_NeoPixel.h>

#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#define USE_SOFTWARESERIAL 1

//Switch
#define BUTTON_PIN 3

//Accelerometer
#define BAUD_RATE 9600
#define SWING_THRESHOLD 9000
#define CLASH_THRESHOLD 11000
#define CLASH_LOOP_COUNT 3
#define MAX_ZEROS_LIMIT 10

//Audio
#define ARDUINO_RX 7//should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 5//connect to RX of the module
#define TRACK_DELAY 3
#define AUDIO_PLAYING_LIMIT 50
#define TIME_BETWEEN_SWINGS 50

//Light
#define LIGHT_PIN 11
#define NUM_LIGHTS 7


//Control
bool saberOn = false;
bool lightOn = true;

//Accelerometer
MPU6050 accelgyro;
int16_t ax, ay, az;
int16_t gx, gy, gz;
int xOld, yOld, zOld; 
int clashLoopCount = 0;
int zerosCount = 0;

//Lights
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LIGHTS, LIGHT_PIN, NEO_GRB + NEO_KHZ800);
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
MD_YX5300 mp3(ARDUINO_RX, ARDUINO_TX);
boolean audioPlaying = false;
uint8_t trackQueue = 0;
int lastPlayedCount = 0;
boolean mandatoryTrackPlaying = false;
int audioPlayingCount = 0;
int lastSwingTime = 0;
int maxDifference = 0;

//Buttons
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

//************************Lights***************************//
void initializeLights()
{
  Serial.print("Initializing Lights...");
  strip.begin();
  strip.setBrightness(255);
  strip.show();
  Serial.println("Lights Initialized");
}

void checkLights()
{
  Serial.println("   Checking Lights");
  if (light_loop == 1) {
    setPixels(0, blue);
  }

  light_loop = light_loop + 1;

  if (light_loop == 4) {
    setPixels(0, light_blue);
    light_loop = 0;
  }
}

void setPixels(int wait, uint32_t color)
{
  Serial.println("      Setting Lights");
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
  Serial.println("      Setting Lights Reverse");
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
  Serial.print("\nInitializing Audio...");
  mp3.begin();
  mp3.setTimeout(1000);
  mp3.setCallback(cbResponse);
  mp3.setSynchronous(false);
  mp3.equalizer(EQUALIZER_MODE);
  mp3.volume(VOLUME);
  Serial.println("\nAudio Initialized.");
}

void cbResponse(const MD_YX5300::cbData *status)
{
  if (status->code != MD_YX5300::STS_ACK_OK)
  {
    lastPlayedCount++;
    if (lastPlayedCount >= TRACK_DELAY || status->code == MD_YX5300::STS_FILE_END) {
      lastPlayedCount = 0;
      audioPlaying = false;
      mandatoryTrackPlaying = false;
      Serial.println("audioPlaying = false");
    }
  }
}

void checkAudio(boolean override) {
  Serial.println("   Checking Audio");
  audioPlayingCount++;

  //If this is over, we can start a new track
  if(audioPlayingCount > AUDIO_PLAYING_LIMIT){
    mandatoryTrackPlaying = false;
  }

  if (override && !mandatoryTrackPlaying) {
    Serial.print("\nPlaying Track ");
    Serial.println(trackQueue);
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
  Serial.print("\nInitializing Gyro...");
  Wire.begin();
  accelgyro.initialize();
  accelgyro.CalibrateAccel(6);
  accelgyro.CalibrateGyro(6);

  for(int x = 0; x < 5; x++){
    Serial.print("*");
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    xOld = abs(ax);
    yOld = abs(ay);
    zOld = abs(az);
  }
  
  Serial.println("Gyro Initialized");
}

void checkMovement()
{
  Serial.println("   Checking Movement");
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

   Serial.print("      Old Values: ");
   Serial.print(xOld);
   Serial.print(", ");
   Serial.print(yOld);
   Serial.print(", ");
   Serial.println(zOld);
   Serial.print("      New Values: ");
   Serial.print(ax);
   Serial.print(", ");
   Serial.print(ay);
   Serial.print(", ");
   Serial.println(az);
   Serial.print("      Difference: ");
   Serial.print(abs(abs(ax) - xOld));
   Serial.print(", ");
   Serial.print(abs(abs(ay) - yOld));
   Serial.print(", ");
   Serial.println(abs(abs(az) - zOld));
   

  if(zerosCount < MAX_ZEROS_LIMIT){
    accelgyro.initialize();
  }

  //We're not getting values, return.
  if( ax == 0 && ay == 0 & az == 0){
    zerosCount = zerosCount + 1;
    return;
  }

  if(saberOn){
    if ( clashLoopCount >= CLASH_LOOP_COUNT)
    {
      if ( abs(abs(ax) - xOld) > CLASH_THRESHOLD ||  abs(abs(ay) - yOld) > CLASH_THRESHOLD || abs(abs(az) - zOld) > CLASH_THRESHOLD )
      {
        clash();
        Serial.println("      Clash Detected!");
      }
  
      clashLoopCount = 0;
    }

    if(lastSwingTime > TIME_BETWEEN_SWINGS || lastSwingTime ==
    0){
      if ( abs(abs(ax) - xOld) > SWING_THRESHOLD ||  abs(abs(ay) - yOld) > SWING_THRESHOLD || abs(abs(az) - zOld) > SWING_THRESHOLD )
      {
        swing();
        Serial.println("      Swing Detected");
      }
    }
    clashLoopCount = clashLoopCount + 1;
    lastSwingTime = lastSwingTime + 1;
  }

  if(abs(abs(ax) - xOld) > maxDifference){
    maxDifference = abs(abs(ax) - xOld);
  }

  if(abs(abs(ay) - yOld) > maxDifference){
    maxDifference = abs(abs(ay) - yOld);
  }

  if(abs(abs(az) - zOld) > maxDifference){
    maxDifference = abs(abs(az) - zOld);
  }

  Serial.print("      Max Difference:  ");
  Serial.println(maxDifference);
  
  xOld = abs(ax);
  yOld = abs(ay);
  zOld = abs(az);
}

//Buttons
void checkForButtonPush() {
  Serial.println("\n   Checking Button State");
  
  buttonState  = digitalRead(BUTTON_PIN);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      Serial.println("      Button Pushed");
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
    Serial.println("\n   CLASH SOUNDS!  RAWWWW!\n");
    setPixels(0, white);
  } else {
    playAudio(TRACK_SABER_CLASH_B);
    Serial.println("\n   CLASH SOUNDS!  BRRRREEE!\n");
    setPixels(0, white_blue);
  }
  light_loop = 0;
  checkAudio(true);
  mandatoryTrackPlaying = true;
}

void swing()
{
  Serial.println("\n   SWIIIIING!\n");

  long randomNumber = random(10, 20);

  if (randomNumber > 15) {
    playAudio(TRACK_SABER_SWING_A);
    Serial.println("\n   CLASH SOUNDS!  RAWWWW!\n");
    setPixels(0, white);
  } else {
    playAudio(TRACK_SABER_SWING_B);
    Serial.println("\n   CLASH SOUNDS!  BRRRREEE!\n");
    setPixels(0, white_blue);
  }
  lastSwingTime = 0;
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(250);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  
  Serial.begin(BAUD_RATE);
  Serial.println("\nBooting Up...");

  //Setup the Gyro
  initializeGyro();

  //Setup the Lgiths
  initializeLights();

  //Get the On/Off button ready
  pinMode(BUTTON_PIN, INPUT);
  digitalRead(BUTTON_PIN);
  //Setup the audio
  initializeAudio();
}

void loop()
{
  Serial.println("Loop Started [");
  if(lightOn){
    digitalWrite(LED_BUILTIN, LOW);
    lightOn = false;
  }else{
    digitalWrite(LED_BUILTIN, HIGH);
    lightOn = true;
  }
  
  if (saberOn) {
    checkAudio(false);
    checkLights();
    mp3.check();
  }
  
  checkMovement();
  checkForButtonPush();
  Serial.println("]\nLoop Ended");
}
