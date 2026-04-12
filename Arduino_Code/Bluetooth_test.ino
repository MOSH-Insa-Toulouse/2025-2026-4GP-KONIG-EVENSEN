/*
Programme de reception Bluetooth via le logiciel MIT APP Inventor 
- connectez deux leds sur les sorties digitales 13 et 12
- connecter le module bluetooth sur les broches 7 et 8 , puis l'alim GND 5V (une fois le programme uploadé)


MATERIEL - MODULE BLUETOOTH V2: 
Pin 1 - VCC => 3.3V Arduino 
Pin 2 - TX => RX Arduino 
Pin 3 - RX => TX Arduino 
Pin 10 - Ground => Ground Arduino

*/

#include <SoftwareSerial.h>

SoftwareSerial switchSerial (8,7);

/* ENTETE DECLARATIVE */
//#define DEBUG

const int ledPin_Red = 12; // la led sera fixée à la broche 12
const int ledPin_Green = 13; // la led sera fixée à la broche 13
byte serialRX; // variable de reception de donnée via RX
byte serialTX; // variable de transmission de données via TX
volatile byte RX = 0; 
boolean lastButtonState = LOW;
boolean buttonState;
int impulsion = 0; 
const int speed_serial = 9600;      // communication Bluetooth 
// digital pin 2 has a pushbutton attached to it. Give it a name:
int pushButton = 2;
volatile int start = 0;
volatile int i = 0;


// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers


void switchSerialEvent(){
  serialRX = switchSerial.read();
  RX = 1;
  /*
  if (start){
    switchSerial.println(i);
    i++;
  }
  */
}


void setup()
{
  Serial.begin(speed_serial); // initialisation de la connexion série (avec le module bluetooth)
  switchSerial.begin(speed_serial);
  //setupBlueToothConnection(); // démarrage liason série bluetooth cf fonction en bas

  pinMode(8, INPUT);
  pinMode(7, OUTPUT);

  //pinMode(ledPin_Red, OUTPUT); // fixe la pin "ledpin" en sortie
  //pinMode(ledPin_Green, OUTPUT); // fixe la pin "ledpin" en sortie
  
  //Déclaration du bouton poussoir
  //pinMode(pushButton, INPUT);
  Serial.print("heisann");
}

void loop() {
  //switchSerial.println(RX);
  /*
  switchSerial.println(i);
  i++;
  */
  int valeur = analogRead(A1)
  switchSerial.println(valeur);

  if (switchSerial.available() > 0){
    switchSerialEvent();
  }
  if (RX){
        RX=0;  
        Serial.println("yo"); 
        switch (serialRX) {
        case 1: // si arduino reçois le chiffre 1 alors
          Serial.println("knapp 1 trykket!");
          start = 1;
          break;
        case 2: // si arduino reçois le chiffre 2 alors
          Serial.println("knapp 2 trykket!");
          start = 0;
          break;
        }
  }

  //serialEvent();
  ///////////////////////////////////////
  // read the input pin: Mettre un pull-up
  //boolean reading  = digitalRead(pushButton);
  //Serial.println(reading);
  // print out the state of the button:

  delay(1000);

  ///////////////////////////////////////

}//fin du loop





