#include "RTClib.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <Adafruit_BME280.h>
#define LEVER_SWITCH_PIN 25
int pressSwitch = 0;
int analogPin = 23;
int whilemillis = 0;
int endmillis = 0;

unsigned long samptime = 28800000;

int startTime = 0;
unsigned long previousMillis = 0;
int interval = 10000; //data read and stored once a minute

Servo CrankS, LO1, LO2;

#define hc12 Serial3

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const int chipSelect = 10;
int nosignal = 52;
int pos = 0;

#define HC12 Serial3

#define ledPin 13         //so we know what blinks
#define BME_VIN 33       //these let us use DIO pins to drive and read the BME280
#define BME_3VO 35      //without extra wires
#define BME_GND 37     //see BME init
#define BME_SCK 39    // Signal CloCK
#define BME_MISO 41  // Master In Slave Out//same as SDO 
#define BME_MOSI 43 // Master Out Slave In //same as SDI
#define BME_CS 45  // Cable Select
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);//here's where we tell the library where we are pinned in
bool BME_GO = false;

#define SEALEVELPRESSURE_HPA (1013.25) //hectaPascal
RTC_PCF8523 rtc;

File logfile;

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A') + 10;
}

void error(uint8_t errno) {
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  //rtc.adjust(DateTime(2016, 10, 16, 11, 31, 0));  //adjust this for the date
  Serial.println(F("BME280 test"));

  //establish BME280 communication
  if (!BME_GO)
  {
    pinMode(BME_GND, OUTPUT);      //This will be ground.
    digitalWrite(BME_GND, LOW);   //We set it to low so it will sink current.
    pinMode(BME_3VO, INPUT);     //We want this pin to float so that we neither load it or drive it.
    pinMode(BME_VIN, OUTPUT);   //Here's our power source good to 20 mA.
    digitalWrite(BME_VIN, HIGH); //and we turn it on
    pinMode(BME_CS, OUTPUT);  //This is cable select. The library functions drive it but we must make it output.
    Serial.print("establishing BME280 communication... ");
    if (bme.begin())
    {
      BME_GO = true;
      Serial.println("bme280 GO!");
    }
    else
    {
      BME_GO = false;
      Serial.println("bme280 NoGO");
      void(*resetfunc)(void) = 0;
      resetfunc();
    }
  }
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  pinMode(6, OUTPUT);
  if (Serial.available()) HC12.write(Serial.read());
  if (HC12.available()) Serial.write(HC12.read());
  digitalWrite(6, LOW); //command mode
  HC12.begin(9600);
  HC12.print(F("AT+C001\r\n")); //set to channel 1
  delay(100);
  digitalWrite(6, HIGH);

  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }

  Serial.println("card initialized.");

  char filename[15];
  strcpy(filename, "KALLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i / 10;
    filename[7] = '0' + i % 10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if ( ! logfile ) {
    Serial.print("Couldnt create ");
    Serial.println(filename);

  }
  Serial.print("Writing to ");
  Serial.println(filename);

  Wire.begin();
  if (!rtc.begin()) {
    logfile.println("RTC failed");
    Serial.println("Couldn't find RTC");
  }

  logfile.print("Date/time");
  logfile.print(',');
  logfile.print("Altitude (sea level Pressure)");
  logfile.print(',');
  logfile.print("Temperature");
  logfile.print(',');
  logfile.print("Pressure");
  logfile.print(',');
  logfile.print("Humidity");
  logfile.println("\n");
  Serial.print("Ready!");


  pinMode(nosignal, OUTPUT);
  pinMode(LEVER_SWITCH_PIN, INPUT);
  pressSwitch = digitalRead(LEVER_SWITCH_PIN);
  digitalWrite(29, LOW);
  analogWrite(analogPin, 0);
  Serial.println(pressSwitch);
}

void loop()
{
  unsigned long currentMillis = millis();
  DateTime now = rtc.now();

  if (HC12.available() > 1) {
    int input = HC12.parseInt();

    if (input == 5113) {  //Site 1, 2m box, Down
      //Sampling begins; sampler opens
      Serial.print("Begin Sampling");
      logfile.println("Samplin Begin");

      delay(500);
      logfile.print("Begin Sampling");
      logfile.println();
      logfile.print(now.year(), DEC);
      logfile.print("/");
      logfile.print(now.month(), DEC);
      logfile.print("/");
      logfile.print(now.day(), DEC);
      logfile.print("/");
      logfile.print(now.hour(), DEC);
      logfile.print(':');
      logfile.print(now.minute(), DEC);
      logfile.print(':');
      logfile.print(now.second(), DEC);
      logfile.print(',');
      logfile.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
      logfile.print(',');
      logfile.print(bme.readTemperature());
      logfile.print(',');
      logfile.print(bme.readPressure());
      logfile.print(',');
      logfile.println(bme.readHumidity());
      logfile.flush();
      delay(1000);


      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      //this line is for the lockout servos
      for (pos = 180; pos >= 0; pos--)
      {
        LO1.write(pos);
        LO2.write(pos);
        delay(5);
      }

      LO1.detach();
      LO2.detach();
      delay(500);
      CrankS.attach(9);
      CrankS.write(0);
      delay(11000);                                  // change this value for crank shaft time going down
      CrankS.detach();
      delay(500);
      HC12.print(5179);       //G-A LED flash down
      delay(500);
      HC12.flush();
      void(*resetfunc)(void) = 0;
      resetfunc();
    }

    if (input == 5131) { //site 1, Box 2m, Lockout open
      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      for (pos = 180; pos >= 0; pos--)
      {
        LO1.write(pos);
        LO2.write(pos);
        delay(5);
      }

      LO1.detach();
      LO2.detach();
      delay(500);
      void(*resetfunc)(void) = 0;
      resetfunc();
    }

    if (input == 5132) { //site 1, Box 2m, lockout close
      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      for (pos = 0; pos <= 180; pos++)
      {
        LO1.write(pos);
        LO2.write(pos);
        delay(5);
      }

      LO1.detach();
      LO2.detach();
      delay(500);
      void(*resetfunc)(void) = 0;
      resetfunc();
    }


    if (input == 5123 || currentMillis - previousMillis > samptime) { //site 1, box 2m, up(end sample)
      //Sampling ends, box closes

      analogWrite(analogPin, 255);

      Serial.print("back up");
      Serial.println(now.year(), DEC);

      delay(500);
      logfile.print("Sampling End");
      logfile.println();
      logfile.print(now.year(), DEC);
      logfile.print("/");
      logfile.print(now.month(), DEC);
      logfile.print("/");
      logfile.print(now.day(), DEC);
      logfile.print("/");
      logfile.print(now.hour(), DEC);
      logfile.print(':');
      logfile.print(now.minute(), DEC);
      logfile.print(':');
      logfile.print(now.second(), DEC);
      logfile.print(',');
      logfile.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
      logfile.print(',');
      logfile.print(bme.readTemperature());
      logfile.print(',');
      logfile.print(bme.readPressure());
      logfile.print(',');
      logfile.println(bme.readHumidity());
      logfile.flush();
      delay(1000);

      Serial.println(pressSwitch);
      CrankS.attach(9);
      delay(500);

      //startcs = millis();
      static unsigned long endmillis;
      int x = 20000;
      endmillis = millis() + x;

      Serial.println(pressSwitch);
      millis();
      while (pressSwitch == 0 &&  millis() < endmillis) {
        Serial.println(millis());
        Serial.println(endmillis);
        CrankS.write(180);
        delay(500);
        pressSwitch = digitalRead(LEVER_SWITCH_PIN);
        Serial.println(pressSwitch);

        if (pressSwitch == 1) {
          HC12.print(5178); //G-A LED flash up
          delay(500);
          HC12.flush();
        }
      }
      delay(100);
      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      for (pos = 0; pos < 180; pos++)
      {
        LO1.write(pos);
        LO2.write(pos);
        delay(5);
      }
      LO1.detach();
      LO2.detach();
      CrankS.detach();
      delay(500);

      analogWrite(analogPin, 0);
      // }

      void(*resetfunc)(void) = 0;
      resetfunc();
    }
    if (input == 5140) { //site 1, Box 2m, up 1 second
      CrankS.attach(9);
      CrankS.write(180);
      delay(1000);
      CrankS.detach();
      void(*resetfunc)(void) = 0;
      resetfunc();
    }

    if (input == 5150) { //site 1, Box 2m, down 1 second
      CrankS.attach(9);
      CrankS.write(0);
      delay(1000);
      CrankS.detach();
      void(*resetfunc)(void) = 0;
      resetfunc();
    }
  }

  if (currentMillis - previousMillis >= interval) {
    Serial.print(now.year(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.day(), DEC);
    Serial.print("/");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print(',');
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.print(',');
    Serial.print(bme.readTemperature());
    Serial.print(',');
    Serial.print(bme.readPressure());
    Serial.print(',');
    Serial.println(bme.readHumidity());
    delay(500);
    logfile.println();
    logfile.print(now.year(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.day(), DEC);
    logfile.print("/");
    logfile.print(now.hour(), DEC);
    logfile.print(':');
    logfile.print(now.minute(), DEC);
    logfile.print(':');
    logfile.print(now.second(), DEC);
    logfile.print(',');
    logfile.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    logfile.print(',');
    logfile.print(bme.readTemperature());
    logfile.print(',');
    logfile.print(bme.readPressure());
    logfile.print(',');
    logfile.println(bme.readHumidity());
    logfile.flush();
    delay(1000);

    previousMillis = currentMillis;
  }
  if (currentMillis - startTime >= samptime) { //site 1, box 150m, up(end sample)
    //Sampling ends, box closes
    Serial.print("back up");
    logfile.println("End Sampling");

    delay(500);
    logfile.print("End Sampling");
    logfile.println();
    logfile.print(now.year(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.day(), DEC);
    logfile.print("/");
    logfile.print(now.hour(), DEC);
    logfile.print(':');
    logfile.print(now.minute(), DEC);
    logfile.print(':');
    logfile.print(now.second(), DEC);
    logfile.print(',');
    logfile.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    logfile.print(',');
    logfile.print(bme.readTemperature());
    logfile.print(',');
    logfile.print(bme.readPressure());
    logfile.print(',');
    logfile.println(bme.readHumidity());
    logfile.flush();
    delay(1000);


    //LIMIT SWITCH OPERATION HERE

    analogWrite(analogPin, 255);
    Serial.println(analogPin);
    Serial.println(pressSwitch);
    CrankS.attach(9);
    delay(500);

    while (pressSwitch == 0) {

      CrankS.write(180);
      delay(500);
      pressSwitch = digitalRead(LEVER_SWITCH_PIN);
      Serial.println(pressSwitch);
    }
    CrankS.attach(9);
    CrankS.write(180);
    delay(15000);            //change this for the value for the crank shaft going up

    LO1.attach(4);
    LO2.attach(5);
    delay(500);

    for (pos = 0; pos < 180; pos++)
    {
      LO1.write(pos);
      LO2.write(pos);
      delay(5);
    }
    CrankS.detach();
    delay(500);

    HC12.print(5178); //G-A LED flash up
    HC12.flush();
    analogWrite(analogPin, 0);
    void(*resetfunc)(void) = 0;
    resetfunc();
  }
}
