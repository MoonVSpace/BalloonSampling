//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Launched Balloon Code - Sampler Box Mega
// Developed by: Margaret Mooney (2017)
// Western Michigan University - Aerospace Engineering 
// Aerospace Lab of Plasma Experiments 
// Code uses Metric Measurment System (Meters, Pa,....)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  #include "RTClib.h"
  #include <stdio.h>          //used for SD file creation      
  #include <Adafruit_BME280.h>// barometer, temperature and humidity sensor
  #include <SPI.h>           //required by the BME280 and SD
  #include <SD.h>            // For Logging Data on SD Card
  #include <Servo.h>         // servo support for deployment actuators
  #define SEALEVELPRESSURE_HPA (1013.25) //hectaPascal
  #define HC12 Serial3
  Servo CrankS, LO1, LO2;
  RTC_PCF8523 rtc;
  
//limit switch
  #define LEVER_SWITCH_PIN 25
  int pressSwitch = 0;
  int analogPin = 23; 
  int whilemillis = 0;
  int endmillis = 0;
  #define ledPin 13         //so we know what blinks
  #define BME_VIN 33       //these let us use DIO pins to drive and read the BME280
  #define BME_3VO 35      //without extra wires
  #define BME_GND 37     //see BME init
  #define BME_SCK 39    // Signal CloCK
  #define BME_MISO 41  // Master In Slave Out//same as SDO 
  #define BME_MOSI 43 // Master Out Slave In //same as SDI
  #define BME_CS 45  // Cable Select
  Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);//here's where we tell the library where we are pinned in
  bool BME_GO=false;
  
//Creation of case structure. If you wish to add an additional case, just simple add the name here
  enum programState {ascend, deploy, collect, reploy, descend} state;
  
/////////////////////////Variables, timers, and pin select//////////////////////////////////
  const int chipSelect = 10;             //SD data
  int startTime = 0;                     //beginning of sample time
  int pos = 0;                           //lockout servos
  int record = 10000;                    //Records data every ten seconds
  unsigned long sampTime = 7200000;    //Sample time - 2 hours. (millis)
  unsigned long previousMillis = 0;
  int PressureAltitude = 54100;          //Pressure in Pa at 5km
  unsigned long openanyway = 3600000;    //Open after 1 hour in air (millis)
  unsigned long halfhour = 1800000;      //half hour after box opens
  unsigned long pbottom = 61640;         //pressure at 4km
  unsigned long pbottom2 = 57728;        //pressure at 4.5km
   
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
      return (c - 'A')+10;
  }
  
  void error(uint8_t errno) {
    while(1) {
      uint8_t i;
      for (i=0; i<errno; i++) {
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
      }
      for (i=errno; i<10; i++) {
        delay(200);
      }
    }
  }
    
void setup() {
  Serial.begin(250000);
  Serial.println("\r\nBalloon Data Logger: Launched code");
  //BME Setup
  //establish BME280 communication
   if (!BME_GO)
        {
          pinMode(BME_GND,OUTPUT);       //This will be ground.
          digitalWrite(BME_GND,LOW);    //We set it to low so it will sink current.
          pinMode(BME_3VO,INPUT);      //We want this pin to float so that we neither load it or drive it.
          pinMode(BME_VIN,OUTPUT);    //Here's our power source good to 20 mA.
          digitalWrite(BME_VIN,HIGH);//and we turn it on
          pinMode(BME_CS,OUTPUT);   //This is cable select. The library functions drive it but we must make it output.
          Serial.print("establishing BME280 communication... ");
          if (bme.begin())
          {
            BME_GO=true;
            Serial.println("bme280 GO!"); 
          }
          else
          {
            BME_GO=false;
            Serial.println("bme280 NoGO"); 
            void(*resetfunc)(void) = 0;
            resetfunc();
          }
        }
//HC12 setup        
  pinMode(6, OUTPUT);
  if(Serial.available()) HC12.write(Serial.read());
  if(HC12.available()) Serial.write(HC12.read());
  digitalWrite(6, LOW); //command mode
  HC12.begin(9600);
  HC12.print(F("AT+C001\r\n")); //set to channel 1
  delay(100);
  digitalWrite(6, HIGH);
       
//SD setup
  Serial.print("Initializing SD card...");
    pinMode(chipSelect, OUTPUT);
    digitalWrite(chipSelect, HIGH);
// see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while(1);
  }
  
  Serial.println("card initialized.");

  //Creation of the file
  char filename[15];
  strcpy(filename, "KALLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    
  }
  Serial.print("Writing to ");         
  Serial.println(filename);

//initializing RTC clock
  Wire.begin();
  if(!rtc.begin()){
    logfile.println("RTC failed");
    Serial.println("Couldn't find RTC");
  }

//tops of the columns for the text file
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

  //Limit switch initialization
  pinMode(LEVER_SWITCH_PIN, INPUT);
  pressSwitch = digitalRead(LEVER_SWITCH_PIN);
  digitalWrite(29, LOW);
  analogWrite(analogPin, 0);
  Serial.println(pressSwitch);
}

void loop() {
  unsigned long currentMillis = millis();
  DateTime now = rtc.now();
/////////////////////////////Begin Case Structure//////////////////////////////////
  switch (state)
  {
   case ascend:
    if (currentMillis - previousMillis >= record){
      collector();
      previousMillis = currentMillis;
    }

    if(currentMillis>=openanyway || bme.readPressure() < PressureAltitude){
      Serial.println("Box opened due to 1 hour in air reached");
      logfile.println("Box opened due to 1 hour in air reached");
      delay(500);
      state=deploy;  //run deploy case if the 1 hour in air
    }

    if(bme.readPressure() <= PressureAltitude){
      Serial.println("5km reached. Yey!");
      logfile.println("5km reached. Yey!");
      delay(500);
      state=deploy;  //run deploy at 5km
    }

    if(HC12.available()>1){
        int input = HC12.parseInt();
        Serial.println("GOT IT");
        logfile.println("GOT IT");
        Serial.println("Recieved Signal from Tracker Box");
        logfile.println("Recieved Signal from Tracker Box");
        Serial.println(input);

       // HC12 Commands From Mini
      // * & 1 is 1424 (AKA HotWire Command)
          if(input == 1424){
            Serial.println("Burn Baby Burn! Hot Wire Command Triggered");
            hotwire();
          }
          if (input == 3863){
            Serial.println("Tow Line Release Command Triggered");
            releaseTowline();
            delay(500);
          }
      }
   break;

   case deploy:
        Serial.println("Sampler Open Started");
        logfile.println("Sampler Open Started");      
        deployTray();
        
        //start timing sampling
        startTime = currentMillis;
        Serial.println("Sampler is Open");
        logfile.println("Sampler is Open");
        state=collect;
   break;

   case collect:
    if(currentMillis-startTime<=halfhour){
      if (currentMillis - previousMillis >= record){
        collector();
        previousMillis = currentMillis;
      }
      if(bme.readPressure()>=pbottom){
        Serial.println("Box closing. Going below 4km");
        logfile.println("Box closing. Going below 4km");
        logfile.println();
        state=reploy;
      }
    }
    if(currentMillis - startTime>=halfhour){
      if (currentMillis - previousMillis >= record){
        collector();
        previousMillis = currentMillis;
      }
      if(bme.readPressure()>=pbottom2){
        Serial.println("Box closing. Going below 4.5km");
        logfile.println("Box closing. Going below 4.5km");
        logfile.println();
        state=reploy;
      }
      if(currentMillis-startTime>=sampTime){
        Serial.println("Box cloxing. Time reached");
        logfile.println("Box closing. Time reached");
        logfile.println();
        state=reploy;
      }
    }
    if(HC12.available()>1){
        int input = HC12.parseInt();
        Serial.println("GOT IT");
        logfile.println("GOT IT");
        Serial.println("Recieved Signal from Tracker Box");
        logfile.println("Recieved Signal from Tracker Box");
        Serial.println(input);

       // HC12 Commands From Mini
      // * & 1 is 1424 (AKA HotWire Command)
          if(input == 1424){
            Serial.println("Burn Baby Burn! Hot Wire Command Triggered");
            hotwire();
          }
          if (input == 3863){
            Serial.println("Tow Line Release Command Triggered");
            releaseTowline();
            delay(500);
          }
      }
   break;

   case reploy:
        Serial.println("Sampler Close Started");
        logfile.println("Sampler Close Started");      
        reployTray();
        Serial.println("Sampler is Closed");
        logfile.println("Sampler is Closed");
        state=descend;
   break;
   
   case descend:
    if (currentMillis - previousMillis >= record){
        collector();
        previousMillis = currentMillis;
      }
      
   if(HC12.available()>1){
        int input = HC12.parseInt();
        Serial.println("GOT IT");
        logfile.println("GOT IT");
        Serial.println("Recieved Signal from Tracker Box");
        logfile.println("Recieved Signal from Tracker Box");
        Serial.println(input);

       // HC12 Commands From Mini
      // * & 1 is 1424 (AKA HotWire Command)
          if(input == 1424){
            Serial.println("Burn Baby Burn! Hot Wire Command Triggered");
            hotwire();
          }
          if (input == 3863){
            Serial.println("Tow Line Release Command Triggered");
            releaseTowline();
            delay(500);
          }
      }
   break;
  }
}

//This is the collector method. It runs fairly similar to an interrupt. It runs in all cases except for reploy and deploy
void collector(){
  DateTime now = rtc.now();
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
      delay(500);
}

//deploy trays method
void deployTray()
{
      Serial.print("Begin Sampling");
      logfile.println("Samplin Begin");
      DateTime now = rtc.now();
      
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
      delay(11000); // change this value for crank shaft time going down
      CrankS.detach();
      HC12.println(9999); //tells tracker box that the sampler is open
      HC12.flush();
      return;
}

void reployTray(){
      DateTime now = rtc.now();
      
      analogWrite(analogPin, 255);
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
      endmillis = millis()+ x;
      
      Serial.println(pressSwitch);
      millis();
      while(pressSwitch==0 &&  millis() < endmillis){
        Serial.println(millis());
        Serial.println(endmillis); 
        CrankS.write(180);
        delay(500);
        pressSwitch = digitalRead(LEVER_SWITCH_PIN);
        Serial.println(pressSwitch);

      }
      delay(100);
      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      for(pos = 0; pos <180; pos++)
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

      HC12.println(1111);
      delay(15000);     
      HC12.println(2222);
      return;
}

void hotwire(){
  Serial.println("\n\nHOT WIRE");
  logfile.println("\n\nHOT WIRE");

  DateTime now = rtc.now();
      
      analogWrite(analogPin, 255);
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
      endmillis = millis()+ x;
      
      Serial.println(pressSwitch);
      millis();
      while(pressSwitch==0 &&  millis() < endmillis){
        Serial.println(millis());
        Serial.println(endmillis); 
        CrankS.write(180);
        delay(500);
        pressSwitch = digitalRead(LEVER_SWITCH_PIN);
        Serial.println(pressSwitch);

      }
      delay(100);
      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      for(pos = 0; pos <180; pos++)
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
  Serial.println("Sampling Complete");
  HC12.flush();
  delay(1000);
  HC12.print(1111);
  return;
}

void releaseTowline(){
  Serial.println("\n\nRelease Tow Line");
  logfile.println("\n\nRelease Tow Line");
  DateTime now = rtc.now();
      
      analogWrite(analogPin, 255);
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
      endmillis = millis()+ x;
      
      Serial.println(pressSwitch);
      millis();
      while(pressSwitch==0 &&  millis() < endmillis){
        Serial.println(millis());
        Serial.println(endmillis); 
        CrankS.write(180);
        delay(500);
        pressSwitch = digitalRead(LEVER_SWITCH_PIN);
        Serial.println(pressSwitch);

      }
      delay(100);
      LO1.attach(4);
      LO2.attach(5);
      delay(500);

      for(pos = 0; pos <180; pos++)
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
  Serial.println("Sampling Complete");
  Serial.println("REPLOY RAN");
  Serial.println("2222");
  HC12.println(2222); //tow line
  return;
}



