#define hc12 Serial3


void setup() 
{

  Serial3.begin(9600);
  Serial.begin(250000);
  //A
  pinMode(11, INPUT);
  pinMode(10, INPUT);
  pinMode(13, INPUT);
  pinMode(12, INPUT);
  pinMode(9, INPUT);
  pinMode(8, INPUT);

  //B
  pinMode(5, INPUT);
  pinMode(4, INPUT);
  pinMode(7, INPUT);
  pinMode(6, INPUT);
  pinMode(3, INPUT);
  pinMode(2, INPUT);

  //C
  pinMode(48, INPUT);
  pinMode(46, INPUT);
  pinMode(52, INPUT);
  pinMode(50, INPUT);
  pinMode(44, INPUT);
  pinMode(42, INPUT);

  pinMode(40, OUTPUT);
  pinMode(38, OUTPUT);
  pinMode(36, OUTPUT);
  
//  pinMode(7,OUTPUT);
//  if(Serial.available()) Serial3.write(Serial.read());
//  if(Serial3.available()) Serial.write(Serial3.read());
//  digitalWrite(7,LOW); // enter AT command mode
//  Serial.begin(9600);
//  Serial3.begin(9600);
//  Serial3.print(F("AT+C001\r\n")); // set to channel 1 
//  delay(100);
//  digitalWrite(7,HIGH);// enter transparent mode
}

void loop() {
  //A = 11 10 13 12 9 8
  int Adown = digitalRead(12);
  int Aup = digitalRead(13);
  int ALOO = digitalRead(11);
  int ALOC = digitalRead(10);
  int Adown2 = digitalRead(8);
  int Aup2 = digitalRead(9);


  //B = 5 4 7 6 3 2
  int Bdown = digitalRead(6);
  int Bup = digitalRead(7);
  int BLOO = digitalRead(5);
  int BLOC = digitalRead(4);
  int Bdown2 = digitalRead(2);
  int Bup2 = digitalRead(3);

  //C = 48 46 52 50 44 42
  int Cdown = digitalRead(50);
  int Cup = digitalRead(52);
  int CLOO = digitalRead(48);
  int CLOC = digitalRead(46);
  int Cdown2 = digitalRead(42);
  int Cup2 = digitalRead(44);
  
  hc12.flush();

 //HC-12 output to tethered box communication
  if(Adown == 1){//A down
    hc12.println(5113);
    Serial.print("\nA down");
  }
  delay(20);//delay little for better serial communication

  if(Aup == 1){//A up
    hc12.println(5123);
    Serial.print("\nA up");
  }
  delay(20);
  
  if(ALOO == 1){//A lockout open
    hc12.println(5131);
    Serial.print("\nA lockout open");
  }
  delay(20);

  if(ALOC == 1){// A lockout close
    hc12.println(5132);
    Serial.print("\nA lockout close");
  }
  delay(20);

  if(Adown2 == 1){// A down 1 second
    hc12.println(5140);
    Serial.print("\nA down2");
  }
  delay(20);//delay little for better serial communication

  if(Aup2 == 1){// A up 1 second
    hc12.println(5150);
    Serial.print("\nA up2");
  }
  delay(20);


  
  

  if(Bdown == 1){//B down
    hc12.println(5213);
    Serial.print("\nB down");
  }
  delay(20);

  if(Bup == 1){//B up
    hc12.println(5223);
    Serial.print("\nB up");
  }
  delay(20);

  if(BLOO == 1){//B lockout open
    hc12.println(5231);
    Serial.print("\nB lockout open");
  }
  delay(20);

  if(BLOC == 1){//B lockout close
    hc12.println(5232);
    Serial.print("\nB lockout close");
  }
  delay(20);
  
  if(Bdown2 == 1){// B down 1 second
    hc12.println(5240);
    Serial.print("\nB down2");
  }
  delay(20);//delay little for better serial communication

  if(Bup2 == 1){// B up 1 second
    hc12.println(5250);
    Serial.print("\nB up2");
  }
  delay(20);



  

  if(Cdown == 1){//C down
    hc12.println(5313);
    Serial.print("\nC down");
  }
  delay(20);

  if(Cup == 1){//C up
    hc12.println(5323);
    Serial.print("\nC up");
  }
  delay(20);

  if(CLOO == 1){// C lockout open
    hc12.println(5331);
    Serial.print("\nC lockout open");
  }
  delay(20);
    
  if(CLOC == 1){// C lockout close
    hc12.println(5332);
    Serial.print("\nC lockout close");
  }  
  delay(20);

  if(Cdown2 == 1){// C down 1 second
    hc12.println(5340);
    Serial.print("\nC down2");
  }
  delay(20);//delay little for better serial communication

  if(Cup2 == 1){// C up 1 second
    hc12.println(5350);
    Serial.print("\nC up2");
  }
  delay(20);


//HC-12 INPUT communication

 if (hc12.available()>1){
    int input = hc12.parseInt();

    //BOX A LED OPERATION  SLOWER-DOWN     FASTER-UP
    if (input == 5179) {  //Site Green, box A, Down
      digitalWrite(40, HIGH);
      delay(1000);                     
      digitalWrite(40, LOW);  
      delay(500);
      digitalWrite(40, HIGH);   
      delay(1000);                       
      digitalWrite(40, LOW);    
      delay(500);
    }

    if (input == 5178){   //Site Green, box A, UP
      digitalWrite(40, HIGH);
      delay(500);                     
      digitalWrite(40, LOW);  
      delay(250);
      digitalWrite(40, HIGH);   
      delay(500);                       
      digitalWrite(40, LOW);    
      delay(250);    
    }

    //BOX B LED OPERATION
    if (input == 5279) {  //Site Green, box B, Down
      digitalWrite(38, HIGH);
      delay(1000);                     
      digitalWrite(38, LOW);  
      delay(500);
      digitalWrite(38, HIGH);   
      delay(1000);                       
      digitalWrite(38, LOW);    
      delay(500);      
    }

    if (input == 5278){   //Site Green, box B, UP
      digitalWrite(38, HIGH);
      delay(500);                     
      digitalWrite(38, LOW);  
      delay(250);
      digitalWrite(38, HIGH);   
      delay(500);                       
      digitalWrite(38, LOW);    
      delay(250);  
    }

    //BOX C LED OPERATION
    if (input == 5379) {  //Site Green, box C, Down
      digitalWrite(36, HIGH);
      delay(1000);                     
      digitalWrite(36, LOW);  
      delay(500);
      digitalWrite(36, HIGH);   
      delay(1000);                       
      digitalWrite(36, LOW);    
      delay(500);      
    }

    if (input == 5378){   //Site Green, box C, UP
      digitalWrite(36, HIGH);
      delay(500);                     
      digitalWrite(36, LOW);  
      delay(250);
      digitalWrite(36, HIGH);   
      delay(500);                       
      digitalWrite(36, LOW);    
      delay(250);      
    }  
  }
  
}
