//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Launched Balloon Code - Tracker Box Mini
// Developed by: Margaret Mooney (2017)
// Western Michigan University - Aerospace Engineering 
// Aerospace Lab of Plasma Experiments 
// Code uses Metric Measurment System (Meters, Pa,....)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Libraries & Defintions Before Set Up 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  #include <Servo.h>
  int n = 0; //hotwire
  int w = 0; //motor
  int f =0; //mexicao emergency
  int k = 0;
  int p=0;
  int pos = 0;
  int startTime=0;
  int currentmillis=0;
  unsigned long timer1 = 14400000; //4 hours
  unsigned long timer2 = 12600000; //3.5 hours
  
  #define HC12 Serial
  Servo servo;

// Void Set Up  
  void setup(){
    Serial.begin(9600);
    servo.attach(10);
 
// DTMF Inputs 

    pinMode(7, INPUT);
    pinMode(11, INPUT); // 8 on DTMF
    pinMode(12, INPUT); // 7 on DTMF
    pinMode(13, INPUT); // 6 on DTMF
    pinMode(8, INPUT); //"5"
    pinMode(9, OUTPUT); //LED flash
    pinMode(2, INPUT); //*
    pinMode(3, INPUT); //1
    pinMode(4, OUTPUT); //hot wire
    pinMode(5, INPUT); //#
    pinMode(6, INPUT);  //2
    pinMode(10, OUTPUT); //motor
  
    for (pos = 0; pos <= 90; pos ++){
          servo.write(pos);
          delay(15);
      }
      servo.detach();
    }

// Void Loop 
void loop(){  
// Inputs from Sampler Box HC 12 
  currentmillis = millis();
  
  
  if(HC12.available()>1){
    int input = HC12.parseInt();
      Serial.println(input);
    if(input == 1111){
      HotWireCutLine();
    }
    if(input == 2222){
      ReleaseTowLine();
    }
    if(input == 9999){
      p++;
      unsigned long openbox = currentmillis;
    }
  }

// LED Blink Light Code
  if(digitalRead(8) == HIGH){ //LED 
    Serial.print("5555");
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
  }


// DTMF: PTT + * + 1 Code 
  if(digitalRead(2) == HIGH){
    n++;
  }
  
  if((digitalRead(3) == HIGH) && n >=1){
   
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);

      Serial.println(1424);

      n=0;
      Serial.flush();
  }

// DTMF: PTT + # + 2 Code 
  if(digitalRead(5) == HIGH){
    w++;
  }
  if((digitalRead(6) == HIGH) && w>=1){
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);
    delay(250);
    //Tell the mega code to close the box

      Serial.println(3863);
      delay(500);
    w=0;
    delay(500);
  }


// Mexico Cut Down Emergency Operation 
  // DTMF = 7 + 8 + 6  
  if(digitalRead(12) == HIGH){ 
    f++; 
  }
  if(digitalRead(11) == HIGH && f>=1){
    k++;
  }
  if(digitalRead(13) == HIGH && k>=1){ //Mexico Emergency
    ReleaseTowLine();
    delay(500);
    HotWireCutLine();
    delay(500);
    Serial.println(6336); //Told mega emergency cut down
    Serial.println("Mexico Emergency triggered by DTMF");
    f=0;
    w=0;
    n=0;
    k=0;  
  }


  if(currentmillis>=timer1 && p=0){
    ReleaseTowLine();
    delay(1000);
    HotWireCutLine();
  }

  if(currentmillis>=timer2 && p>=1){
    ReleaseTowLine();
    delay(1000);
    HotWireCutLine();
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Methods called from loop

// Tow Line Release Function
void ReleaseTowLine(){
  servo.attach(10);
    for(pos = 90; pos >=0; pos--){
      servo.write(pos);
      delay(15);
    }
    servo.detach();
    return;
   }
// Hot Wire Function
void HotWireCutLine() {
    digitalWrite(4, HIGH);
    delay(10000);
    digitalWrite(4, LOW);
    return;
    }
 

