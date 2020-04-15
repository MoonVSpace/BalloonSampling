#include <Servo.h>
int n = 0; //hotwire
int w = 0; //motor
int f =0; //mexicao emergency
int k = 0;
int pos = 0;
#define HC12 Serial

Servo servo;
#define inputStringLength 20        // to buffer messages from the MEGA
String inputString = "";           // a string to hold incoming data
String workingString = "";        // a copy if inputString for processing
boolean stringComplete = false; //determines if the string is complete
void setup(){
  servo.attach(10);
  pinMode(7, INPUT);
  pinMode(11, INPUT);
  pinMode(12, INPUT);
  pinMode(12, INPUT);
  
  pinMode(8, INPUT); //"5"
  pinMode(9, OUTPUT); //LED flash
  
  pinMode(2, INPUT); //*
  pinMode(3, INPUT); //A
  pinMode(4, OUTPUT); //hot wire
  
  pinMode(5, INPUT); //#
  pinMode(6, INPUT);  //B
  pinMode(10, OUTPUT); //motor
  for (pos = 0; pos <= 90; pos ++){
      servo.write(pos);
      delay(15);
  }
  servo.detach();
}

void loop(){  
  if (stringComplete) 
  {
    workingString=inputString;
    inputString = "";
    stringComplete = false;
    //Serial.println(workingString.compareTo(Astring));
    Serial.println(workingString);
    if (workingString.equals("Sorrite?"))
      {
        Serial.println("Sorrite!");
      }  
  }

  if(HC12.available()>1){
    int input = HC12.parseInt();

    if(input == 1111){
      HotWireCutLine();
    }

    if(input == 2222){
      ReleaseTowLine();
    }
  }

  
  if(digitalRead(8) == HIGH){ //LED 
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
    
  if(digitalRead(2) == HIGH){
    n++;
  }
  
  if((digitalRead(3) == HIGH) && n >=1){
    for (int i=0; i <= 50000; i++){
      Serial.print(1424);
      delay(10);
   } 
    
    n=0;
    delay(500);
  }
  
  if(digitalRead(5) == HIGH){
    w++;
  }
  
  if((digitalRead(6) == HIGH) && w>=1){
    //Tell the mega code to close the box
    for (int b=0; b <= 50000,; b++){
      Serial.print(3863);
      delay(10);                                                 
   } 
    w=0;
    delay(500);
  }

  if(digitalRead(12) == HIGH){
    f++;
  }

  if(digitalRead(11) == HIGH){
    k++;
  }

  if(digitalRead(13) == HIGH && f>=1 && w>=1 && n>=1 && k++){ //Mexico Emergency
    ReleaseTowLine();
    delay(500);
    HotWireCutLine();
    delay(500);
    Serial.println(6336); //Told mega emergency cut down
    f=0;
    w=0;
    n=0;
    k=0;  
  }
  
}

void ReleaseTowLine()
{
  servo.attach(10);
    for(pos = 90; pos >=0; pos--){
      servo.write(pos);
      delay(15);
    }
    servo.detach();
  return;
}

void HotWireCutLine()
{
    digitalWrite(4, HIGH);
    delay(30000);
    digitalWrite(4, LOW);
}

