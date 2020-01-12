# Arduino Lightsaber

This is an implementation of the controls for a Lightsaber using Arduino for the electronics.

### Parts used

* Arduino Nano
* MPU6050 Accelerometer
* MD-YX5300 MP3 Player
* A TINY audio amplifier
* A small speaker (less than 35mm diameter)
* Neopixel light ring (can be substituted with a light strip)
* 18650 Battery Holder (the one I found had (3) 5V and (3) 3.3V solder points)
* A small switch
* 1 10k resistor


### Construction and .STL File

I've included the .stl for the part I used to hold most of the electronics.  It was terrible.  Make a better one.

I'm terrible at soldering, and was able to get  this working and fit inside a Graphlex replica.
If I can do it, you can too!

### Debugging

The Serial.prints are still active in the sketch as uploaded. 
If after you get everything working you're confident you won't need them, 
remove them to improve performance (although you may need to adjust some of the counters.) 

There is also a lot of "debugging" code tracking variables that can be removed, particularly around the audio.

Using a nano, I haven't had any issues leaving the code in, but if you try this on a different board,
you may want to remove some of this to help increase speed.

### Wiring

Most of the pins needed are defined inside the top of the code.  But here's a quick breakdown:

#### Switch
| Pin           | Connects to...                         |
| :------------- |---------------------------------------:|
| 1             | 5V on Arduino                          |
| 2             | GND on Arduino with 10k resistor inline|
| 3             | Pin 3 on Arduino                       | 


#### MPU6050
| Pin           | Connects to...                         |
| :------------- |---------------------------------------:|
| VCC           | 3.3V on Arduino                        |
| GND           | GND on Arduino                         |
| SCL           | A5 on Arduino                          | 
| SCA           | A4 on Arduino                          | 


#### MD-YX5300
| Pin           | Connects to...                         |
| :------------- |---------------------------------------:|
| VCC           | 5V on 18650 Holder                     |
| GND           | GND on 18650 Holder                    |
| RX            | D5 on Arduino                          | 
| TX            | D7 on Arduino                          | 


#### Neopixel
| Pin           | Connects to...                         |
| :------------- |---------------------------------------:|
| VCC           | 5V on 18650 Holder                     |
| GND           | GND on 18650 Holder                    |
| DI            | D11 on Arduino                         | 

#### Audio Amp
| Pin           | Connects to...                         |
| :------------- |---------------------------------------:|
| VCC           | 3.3V on 18650 Holder                   |
| GND           | GND on 18650 Holder                    |
| Speaker+      | Speaker+                               | 
| Speaker-      | Speaker-                               | 