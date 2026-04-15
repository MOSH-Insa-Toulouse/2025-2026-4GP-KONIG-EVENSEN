/*
Program for the Bluetooth communication via an android application created with the 
program MIT App Inventor and a HC-05 Bluetooth module. The Pins 7 and 8 are connected 
respectively to the TxD and RxD pins of the Bluetooth module, which is also connected 
to the GND and Vcc 5V.



   Arduino UNO Serial connections:
   Uno          Bluetooth Module 
   ------------------------
   5V            Vcc (pin 1)
   GND           GND  (pin 2)


   D07 (TxD)     RxD (pin 4)
   D08 (RxD)     TxD (pin 3)

*/

//Include the library SoftwareSerial in order to set the pins 7 and 8 as serial pins
#include <SoftwareSerial.h>
//Sets the pin 7 as TxD and 8 as RxD on the Arduino UNO
SoftwareSerial switchSerial (8,7);

//Declaration of variables needed for the Bluetooth communication
byte serialRX; //Variable for the bytes received from the application
volatile byte RX = 0; //Variable for the state of RX
int start = 0; //Variable for determining if a measure should be done

//switchSerialEvent is run every time a message is received from the application via Bluetooth
void switchSerialEvent(){
  serialRX = switchSerial.read(); //Reads the message from the application
  RX = 1; //Switches the state of RX
}

//Setup needed for declaring pinmodes and starting serial communication
void setup()
{
  Serial.begin(9600); //Initialization of serial monitor, baudrate 9600
  switchSerial.begin(9600); //Initialization of switchSerial used for communication over Bluetooth, baudrate 9600

  pinMode(8, INPUT); //Declares RxD as input on the arduino
  pinMode(7, OUTPUT); //Declares TxD as output on the arduino

  switchSerial.flush(); //Flushes any data left on the communication bus
}

//Loop function runs continuously when this program is uploaded to the Arduino UNO
void loop() {
  //Creates a switchSerialEvent every time a message is sent from the application over Bluetooth
  if (switchSerial.available() > 0){
    switchSerialEvent();
  }
  
  //When a switchSerialEvent is triggered, this function runs, starting and stopping the measurement
  if (RX){
        RX=0; //Resets the state of RX so that this function is only called when a new message is sent from the application
        switch (serialRX) {
        case 1: //If the Arduino UNO receives the message 1, start is set to 1 and a measure starts
          Serial.println("Start button clicked!");
          start = 1;
          break;
        case 2: //If the Arduino UNO receives the message 2, start is set to 0 and the measure stops
          Serial.println("Stop button clicked!");
          start = 0;
          break;
        }
  }

  //This function, sending the resistance value of the flex sensor at A1, runs between the instants where "Start" and "Stop" are clicked
  if (start){
    int valeur = analogRead(A1);
    float Vflex = valeur * 5 / 1023.0; //Transfers the bit value into tension
    float Rflex = 47000 * (5 / Vflex - 1.0); //Transfers the tension value into resistance
    switchSerial.println(Rflex); //Sends the resistance value to the application
    Serial.println(Rflex); //Prints the resistance value to the Serial monitor
  }

  //Delay between loops to increase the stability of the program
  delay(500);
}
//End of loop





