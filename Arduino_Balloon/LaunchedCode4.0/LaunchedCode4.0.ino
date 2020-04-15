/*Balloon data logger
 * This program expects an Arduino Mega 2560 or equivelent
 */
#include <stdio.h>           //I'm not sure about this
#include <Adafruit_BME280.h>// our barometer, temperature and humidity sensor
#include <SPI.h>           //required by the BME280 and SD
#include <Adafruit_GPS.h> //for the Ultimate GPS shield
#include <SD.h>// This requires the Adafruit modified SD library that can work with any pin.
              //https://github.com/adafruit/SD (http://adafru.it/aP6)
             //Arduino IDE will want to update you afterward, so watch out
//#include <avr/sleep.h>    // does not seem to be used
#include <Servo.h>         // servo support for deployment actuators
enum programState {initialize, ascend, deploy, collect, reploy, reploy2, descend, alarm} state;
#define SEALEVELPRESSURE_HPA (1013.25) //hectaPascal
#define minHeight 50                              //height is distance above ground. All distances in meters.
float groundElevation=270; //m above sealevel    //we'll read this from GPS later and recalculate
float height=500; //m above ground
float altitudeSetPoint=groundElevation + height;//our goal
#define underAltitudeLimit 50
float shutdownAltitude=altitudeSetPoint-underAltitudeLimit;


//PARAMETERS TO CHANGE FOR TESTING
float PressureAltitude = 55000;               //Pa at 5km
float shutdownPressure = 60000;               //Pa at 4.5km. If the box reads pressure higher than 60Pa, it runs reploy2() see line 358
float endearlytime=1800000;                   //Half hour altitude control. in reploy2() if the box is in reploy2() for longer than a half hour, it shuts down.
//unsigned long samplingDuration=60000;      //milliseconds. Sampling time. We will be sampling from deployment to reployment
int samptime=30;
int sampcount=0;                    
int checkalt=5000;                            //5000 Pa. Hot wire is first line of deploy. If after 5 mintues the box isn't about half a km below the start of the cut down, it will run the tow line.
int timereach=0;
int openanyway=30;                          //in seconds. open the box after a half hour
//DO NOT CHANGE ANYTHING OUTSIDE OF THESE FOR TESTINGS


#define HOURS 1 
int setpointReached=0, setpointMinCount=6;    //we count how many consecutive readings are above minimum altitude
Servo myservoF1, myservoF2, myservoCS;        //F1->feather 1; F2->feather 2; CS->Crank Shaft
int pos = 0;                                  //initial position of the feather motors
#define alarmPin 53                           // this will drive the alarm buzzer we may want to move this to the MINI
#define mySerial Serial3                      // the MEGA has 4 hardware serial ports We're using Serial3 for GPS 
Adafruit_GPS GPS(&mySerial);                  //and the default soft serial pins have been neutered on the Ultimate GPS shield
#define HC12 Serial2                          //We'll talk to the HC-12 Wireless Serial Port Communication Module on Serial2 
#define HC12baud 2400                         //nice and slow. We don't have much to say anyway, just "cutdown" and "release towline"
bool HC12_GO=false;
int currentMillis = 0;
int start = 0;
int previousMillis = 0;
int shutdowninitialized = 0;
int coll = 0;

int reedPin = 2; //Reed switch pin
int reedVal = 0; //Reed switch pin value

int altcount = 0; //int counter for altitude
int initialdescalt = 0;

boolean usingInterrupt = false; // flag to track of whether we're using the interrupt
void useInterrupt(boolean); 
bool GPS_parse=false;          //flag to check for parsed data
#define GPSECHO  true        //echo the the raw GPS sentences GPS data to the Serial console
#define LOG_FIXONLY false    // set to true to only log to SD when GPS has a fix, for debugging, keep it false  
#define UTCOffset -4        //time zone correction for FAT CLOCK GPS is always UTC

#define SD_CS 10            // Cable Select pin for SD card
#define SD_MOSI 11         // Master Out Slave In 
#define SD_MISO 12        // Master In Slave Out
#define SD_SCK 13        // Signal CloCK
bool SD_GO=false;

#define ledPin 13         //so we know what blinks
#define BME_VIN 31       //these let us use DIO pins to drive and read the BME280
#define BME_3VO 33      //without extra wires
#define BME_GND 35     //see BME init
#define BME_SCK 37    // Signal CloCK
#define BME_MISO 39  // Master In Slave Out//same as SDO 
#define BME_MOSI 41 // Master Out Slave In //same as SDI
#define BME_CS 43  // Cable Select
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);//here's where we tell the library where we are pinned in
bool BME_GO=false;
#define inputStringLength 20        // to buffer messages from the MEGA
String inputString = "";           // a string to hold incoming data
String workingString = "";        // a copy if inputString for processing
boolean stringComplete = false;  // whether the string is complete


File logfile;

void setup() 
{
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  
                            // make sure that the default chip select pin is set to
                           // output, even if you don't use it it avoids problems in the SD library:
  pinMode(10, OUTPUT);    //specifically
  pinMode(SD_CS, OUTPUT);//Yes, I know this is redundant but it happens to be the same as the default
  Serial.begin(250000); //to the terminal
  Serial.println("\r\nBalloon data logger");
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCK)) {
    Serial.println("Card init. failed!");
    while(1);
    
  }

  pinMode(3, INPUT);  //this is the toggle switch at the top of the box
  pinMode(reedPin, INPUT);
  Serial2.begin(9600);
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
 
          logfile.print("GPS.latitudeDegrees");
          logfile.print(',');
          logfile.print("GPS.longitudeDegrees");
          logfile.print(',');
          logfile.print("GPS.altitude");   
          logfile.print(',');
          logfile.print("GPS.satellites");
          logfile.print(',');
          logfile.print("bme Altitude(HPA)");
          logfile.print(',');
          logfile.print("Temperature");
          logfile.print(',');
          logfile.print("Pressure");
          logfile.print(',');
          logfile.print("Humidity");
          logfile.print(',');
          logfile.print("GPS.hour");
          logfile.print(':');
          logfile.print("GPS.minute");
          logfile.print(':');
          logfile.print("GPS.seconds");
          logfile.println();
          logfile.flush();
  
    Serial.println("Before GPS init.");                                             
                                             //establish GPS communication
  GPS.begin(9600);                          // connect to the GPS at the default rate. Other rates require sending a set baud rate command. 
                                           //Might be worth while but tricky. Rate resets to default after reset restart power failure etc.
  //GPS.sendCommand(PMTK_SET_BAUD_57600); //will set the baud rate to 57600 but the rate resets after power cycles or resets
  //GPS.end();                           //of course, if you do you'll need to switch to that rate here too.
  //GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); //RMC (recommended minimum) and GGA (fix data) including altitude
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate
  GPS.sendCommand(PGCMD_NOANTENNA);           // Turn off updates on antenna status, if the firmware permits it
  useInterrupt(true);                        // see GPS example code for Adafruit Ultimate GPS shield
  //delay (2000);                             // to let it get started

  Serial.println("after GPS init.");  
}
// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef UDR0
      if (GPSECHO)
        if (c) UDR0 = c;  
      // writing direct to UDR0 is much much faster than Serial.print 
      // but only one character can be written at a time. 
  #endif
}

void useInterrupt(boolean v) {
  if (v) {
    Serial.println("UseInterrupt");
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

   ISR(TIMER2_COMPA_vect){//timer0 interrupt 2kHz toggles pin 8
    //generates pulse wave of frequency 2kHz/2 = 1kHz (takes two cycles for full wave- toggle high then toggle low)
     // Serial.println("GOT IT");
      if(HC12.available()>1){
        int input = HC12.parseInt();
        Serial.println("GOT IT");
        cutdownactivate();
      }     
    }

void error(uint8_t errno) { // blink out an error code
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
void loop() 
{   
  if (stringComplete) // then there is a message from the PRO MINI. 
                     //We do this before checking for GPS messages in case there 
                    //is something wrong with the GPS and DTMF is used to abort
  {
    workingString=inputString;              //We make a copy of the incoming message
    inputString = "";                      // and empty it so that a new one can accumulate
    if (workingString.equals("Sorrite!")) // communication has been established
    {
      HC12_GO=true;
      Serial.println("HC12 GO!"); 
    }
    else if (workingString.equals("DTMF cut line."))//then PRO MINI recieved a cut line command
    {
      Serial.println(workingString); 
      state=reploy;                              //we retract the trays and prepare for landing
    }
  }
  if (GPS.newNMEAreceived()) //check GPS for news (this may happen many times before it is true)
  {
    float altitude;
    char *stringptr = GPS.lastNMEA();
    if (GPS_parse=!GPS.parse(stringptr))return;                     // this resets the newNMEAreceived() flag to false for you
    if (BME_GO) altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); //read bme280
    long now= millis(),startTime,endTime;
    byte filenamelen=16;
    char filename[filenamelen];
    byte datalen=80;
    char dataline[datalen];
    
    
    //CASE STRUCTURES BEGIN
    switch (state)
    {
      case initialize:

            groundElevation=GPS.altitude;                       //m above sealevel
            altitudeSetPoint=groundElevation + height;          //our goal
            shutdownAltitude=altitudeSetPoint-underAltitudeLimit;
            Serial.print("All things done");  
        
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
          }
        }

       //.This will be used once I start communication with the HC12
        if (!HC12_GO)
        {
          HC12.begin(HC12baud);//we want to find out if we can get through to the PRO MINI before launch
          HC12.println("Sorrite?");
        }


        
        if (BME_GO)
        {
          Serial.println("SD Go, BME Go, HC12 Go. Anchors aweigh!");
          //Serial.println("ending serial communication");  Serial.end();      //use this for non-troubleshooting
          state = ascend;
        }
      break;
      
      case ascend:
       //wait until we reach altitude
        Serial.println("ascend");
        collecter(stringptr);
        Serial.println("After collect");
       //Serial.println(bme.readPressure());
       if (bme.readPressure() < PressureAltitude || timereach>=openanyway) //Change for 5km (551hpa)
        {
          Serial.print("pressure");
          start++;
          if (start>20){  //change for pressure double check time
            Serial.println("start loop");
                if(bme.readPressure()<PressureAltitude){
                    state=deploy; 
                  }
                  else start=0;
          }
        }

      timereach++;
      Serial.println(timereach);
      if(timereach>=openanyway){
        state=deploy;
      }
      break;
      
      case deploy:
        //We're at altitude
        //deploy sample trays
        deployTray();
        //start timing sampling
        startTime=now;
//        endTime=startTime+samplingDuration;
        state=collect;
      break;
      
      case collect:
        Serial.println("\n\n\n\n\n\nCollect");
        sampcount++;      
        collecter(stringptr);
        Serial.println(sampcount);
        if (sampcount>=samptime){
          state= reploy;
        }

        if(bme.readPressure()>shutdownPressure){
          shutdowninitialized = now;
          state=reploy2; 
        }        
      break;
      
      case reploy:
        //we're done sampling 
        reployTray();// the sample trays
        //release tow line
        logfile.print("Descending");
        state=descend;
        //close SD file maybe
      break;

      case reploy2:
      //shut down pressure
          collecter(stringptr);      
          if(bme.readPressure()<PressureAltitude){
            //opens box back
            deployTray();
            state=collect;
          }
    
          if(now-shutdowninitialized>endearlytime){
            //bring box down
            reployTray();
            logfile.println("Descending");
            logfile.println();
            logfile.println("Early cut down due to low altitude time:");
            logfile.print(GPS.hour);
            logfile.print(':');
            logfile.print(GPS.minute);
            logfile.print(':');
            logfile.print(GPS.seconds);
            logfile.println();
            logfile.flush();
           initialdescalt= bme.readPressure(); //inital descent altitude
            state=descend;
          }
      break;          
            
      case descend:
        altcount++;
        
        collecter(stringptr);
        if(altcount>60){
          if(bme.readPressure()<initialdescalt-checkalt){ //if hot wire did not cut, cut with tow line
            HC12.print(2222);
          }
          
        }
        
        if (altitude < minHeight) state = alarm;
      break;
      
      case alarm:
        while (1)
        {
          collecter(stringptr);
          digitalWrite(alarmPin, HIGH); 
          delay(1000);
          digitalWrite(alarmPin, LOW);
          delay(1000);
        }
      break;
    }
  }
}

void collecter(char stringptr[]){
   //write to SDfile
        coll++;
        *stringptr = GPS.lastNMEA();
        Serial.println("Log");
     
        if (!GPS.parse(stringptr))   // this also sets the newNMEAreceived() flag to false
          return;  // we can fail to parse a sentence in which case we should just wait for another
        if (strstr(stringptr, "GGA"))
        {
          byte timelen=30;
          char timestamp[timelen];
          snprintf(timestamp,timelen,"%02d:%02d:%02d.%03d,",GPS.hour,GPS.minute,GPS.seconds,GPS.milliseconds);

          Serial.print(GPS.latitudeDegrees,5);
          Serial.print(',');
          Serial.print(GPS.longitudeDegrees,5);
          Serial.print(',');
          Serial.print(GPS.altitude);   
          Serial.print(',');
          Serial.print(GPS.satellites);
          Serial.print(',');
          Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
          Serial.print(',');
          Serial.print(bme.readTemperature());
          Serial.print(',');
          Serial.print(bme.readPressure());
          Serial.print(',');
          Serial.println(bme.readHumidity());

          if(coll>20){
              logfile.print(GPS.latitudeDegrees, 5);
              logfile.print(',');
              logfile.print(GPS.longitudeDegrees, 5);
              logfile.print(',');
              logfile.print(GPS.altitude);   
              logfile.print(',');
              logfile.print(GPS.satellites);
              logfile.print(',');
              logfile.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
              logfile.print(',');
              logfile.print(bme.readTemperature());
              logfile.print(',');
              logfile.print(bme.readPressure());
              logfile.print(',');
              logfile.print(bme.readHumidity());
              logfile.print(',');
              logfile.print(GPS.hour);
              logfile.print(':');
              logfile.print(GPS.minute);
              logfile.print(':');
              logfile.print(GPS.seconds);
              logfile.println();
              logfile.flush();
              coll = 0;
          }
        }
}

//utility functions from here on down

// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time)
{
 // return date using FAT_DATE macro to format fields
 *date = FAT_DATE(GPS.year+2000, GPS.month, GPS.day);
 // return time using FAT_TIME macro to format fields
 *time = FAT_TIME(GPS.hour+UTCOffset, GPS.minute, GPS.seconds);
}
/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;   // if the incoming character is a newline, set a flag
      break;
    }
    // add it to the inputString:
    if (inChar != '\r') inputString += inChar;//we throw away carriage returns
    // so the main loop can do something about it:
  }
}

void deployTray()
{
  
  char *message="Deploying trays";
  Serial.print(message);
  logfile.print("message");
  
  myservoF1.attach(3);
  myservoF2.attach(5);
  delay(500);
  for(pos = 0; pos <180; pos++)
    {
      myservoF1.write(pos);
      myservoF2.write(pos);
      delay(5);
    }
  myservoF1.detach();
  myservoF2.detach();
  delay(500);
  myservoCS.attach(9);
  myservoCS.write(180);
  delay(5000);
//  if(reedVal == LOW){ //wait until reed value is high
//    myservoCS.write(45);  
//  }
//  else{
//    myservoCS.detach();
//    delay(100);  
//  }
  myservoCS.detach();
  delay(500);
  return;
}

void reployTray(){
  char *message="retracting trays"; 
  Serial.print(message);
  logfile.print(message);
  myservoCS.attach(9);
  myservoCS.write(145);
  
  if( digitalRead(4) == HIGH){ //toggle switch
    myservoCS.detach();
    delay(100);
  }
  else{
    myservoCS.write(145);
  }
  delay(500);
  myservoF1.attach(3);
  myservoF2.attach(5);
  delay(500);
  for(pos = 180; pos>=0; pos--)
  {
    myservoF1.write(pos);
    myservoF2.write(pos);
    delay(5);
  }
  myservoF1.detach();
  myservoF2.detach();
  delay(500);
  Serial.println("Sampling Complete");
    
  return;
}

void cutdownactivate(){
  //This is where the cut down will tell the box to close and come down.
  Serial.print("Cut Down");
    if(HC12.available()>1){
              int input = HC12.parseInt();

              if(input == 6336){
                Serial.println("Mexico Emergency");
                //Mini was cut down
              }
    
              if(input == 3863){
                Serial.println("DTMF cut down");
                    myservoCS.attach(9);
                    myservoCS.write(145);
                    
                    if( digitalRead(4) == HIGH){ //toggle switch
                      myservoCS.detach();
                      delay(100);
                    }
                    else{
                      myservoCS.write(145);
                    }
                    delay(500);
                    myservoF1.attach(3);
                    myservoF2.attach(5);
                    delay(500);
                    for(pos = 180; pos>=0; pos--)
                    {
                      myservoF1.write(pos);
                      myservoF2.write(pos);
                      delay(5);
                    }
                    myservoF1.detach();
                    myservoF2.detach();
                    delay(500);
                    Serial.println("Sampling Complete");
                    HC12.print(2222);
              }
              if(input == 1424){
                Serial.print("hot wire"); 
                    myservoCS.attach(9);
                    myservoCS.write(145);
                    
                    if( digitalRead(4) == HIGH){ //toggle switch
                      myservoCS.detach();
                      delay(100);
                    }
                    else{
                      myservoCS.write(145);
                    }
                    delay(500);
                    myservoF1.attach(3);
                    myservoF2.attach(5);
                    delay(500);
                    for(pos = 180; pos>=0; pos--)
                    {
                      myservoF1.write(pos);
                      myservoF2.write(pos);
                      delay(5);
                    }
                    myservoF1.detach();
                    myservoF2.detach();
                    delay(500);
                    Serial.println("Sampling Complete");
                delay(500);           
                HC12.print(1111);
              }
              
            }
}




