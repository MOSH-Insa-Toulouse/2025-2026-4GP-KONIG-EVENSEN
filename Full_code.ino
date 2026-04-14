#include <QR.h>

/*
   Arduino Uno SPI connections:
   Uno            MCP41100
   ------------------------
   D02           CLK (pin A)
   D03           DT  (pin B)
   D04           SW


   D10           CS  (pin 1)
   D11 (MOSI)    SI  (pin 3)
   D13 (SCK)     SCK (pin 2)
*/

#include <SPI.h>
#include <Adafruit_SSD1306.h>


// Variables for the potentiometer
const byte csPin                    = 10;      // MCP42100 chip select pin
const int  maxPositions             = 256;     // wiper can move from 0 to 255 = 256 positions
const long rAB                      = 92500;   // 100k pot resistance between terminals A and B (mais pour ajuster au multimètre, je mets 92500) 
const byte rWiper                   = 125;     // 125 ohms pot wiper resistance
const byte pot0                     = 0x11;    // pot0 addr // B 0001 0001
const byte pot0Shutdown             = 0x21;    // pot0 shutdown // B 0010 0001
volatile int potentiometerWiperPos  = 20;
long       resistanceWB             = ((rAB * potentiometerWiperPos) / maxPositions ) + rWiper;   // works as R2 in AOP circuit
volatile int calibrated             = 0;

// Variables for the amplification circuit of the graphite sensor
const int graphitePin = A0;
const int VccBits = 1024;
const float r1 = 100.0*(pow(10,3));
const float r3 = 100.0*(pow(10,3));
const float r5 = 10.0*(pow(10,3));

// Variables for the commercial flex sensor
const int flexPin = A1;                 // Pin connected to voltage divider output
const float Vcc = 5;                    // voltage at Ardunio 5V line
const float R_DIV = 47000.0;            // resistor used to create a voltage divider
const float flatResistance = 29600.0;   // resistance when flat
const float bendResistance =  56000.0;  // resistance at 90 deg

// Variables for the OLED screen
#define OLED_width      128         // Width of OLED screen in pixels
#define OLED_height     64          // Height of OLED screen in pixels
#define OLED_pinReset   -1          // Reset of OLED shared with the Arduino
#define OLED_I2Caddress 0x3C        // Addresse of OLED screen on I2C bus (0x3C or 0x3D)
Adafruit_SSD1306 OLED_screen(OLED_width, OLED_height, &Wire, OLED_pinReset);

/*
// Variables for the bluetooth connection
#include <SoftwareSerial.h>

SoftwareSerial switchSerial (8,7);
byte serialRX; // variable de reception de donnée via RX
byte serialTX; // variable de transmission de données via TX
volatile byte RX = 0; 
int start = 0;

void switchSerialEvent(){
  serialRX = switchSerial.read();
  RX = 1;
}
*/

// Variables for the rotary encoder
#define Switch        2  // Switch connection if available
#define encoder0PinA  3  //CLK Output A Do not use other pin for clock as we are using interrupt
#define encoder0PinB  4  //DT Output B
int lastButton = HIGH;

volatile int encoder0Pos = 1;
volatile int touched = 0;


/////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);

  // Potentiometer
  digitalWrite(csPin, HIGH);        // chip select default to de-selected
  pinMode(csPin, OUTPUT);           // configure chip select as output
  
  // Flex Sensor
  pinMode(flexPin, INPUT);

  // Graphite Sensor
  pinMode(graphitePin, INPUT);

  // Rotary encoder
  pinMode(encoder0PinA, INPUT); 
  digitalWrite(encoder0PinA, HIGH);       // turn on pullup resistor
  pinMode(encoder0PinB, INPUT); 
  digitalWrite(encoder0PinB, HIGH);       // turn on pullup resistor
  attachInterrupt(1, encoder_interrupt, RISING);  // encoder pin on interrupt 1 --> pin3


  // OLED
  if(!OLED_screen.begin(SSD1306_SWITCHCAPVCC, OLED_I2Caddress))
    while(1);                               // Arrêt du programme (boucle infinie) si échec d'initialisation
  //screen_OLED();
  /*
  switchSerial.begin(9600);

  pinMode(8, INPUT);
  pinMode(7, OUTPUT);
  switchSerial.flush();
  */
  SPI.begin();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  potentiometer_setWiper(pot0, potentiometerWiperPos);

  Serial.print("Position:");
  Serial.println (encoder0Pos, DEC);  //Angle = (360 / Encoder_Resolution) * encoder0Pos
  read_encoder_button();

  screen_OLED();
  Serial.println(digitalRead(Switch));

  /*
  if (switchSerial.available() > 0){
    switchSerialEvent();
  }
  if (RX){
        RX=0;  
        switch (serialRX) {
        case 1: // si arduino reçois le chiffre 1 alors
          Serial.println("start button pressed");
          start = 1;
          break;
        case 2: // si arduino reçois le chiffre 2 alors
          Serial.println("stop button pressed");
          start = 0;
          break;
        }
  }

  if (start){
  */
  if (touched == 1 && encoder0Pos == 1) {
    flex_getResistance();
  } 
  if (touched == 1 && encoder0Pos == 2) {
    graphite_getResistance();
  } 
  //}

  //delay(100);  // delay in between reads for stability
  Serial.println(potentiometerWiperPos);
}


void read_encoder_button(){
  int currentButton = digitalRead(Switch);
  Serial.println (currentButton);
  if (lastButton == HIGH && currentButton == LOW){
    touched = !touched;
    delay(50); //simple debounce
  }
  lastButton = currentButton;
}


void potentiometer_setWiper(int addr, int pos) {
  pos = constrain(pos, 0, 255);            // limit wiper setting to range of 0 to 255
  digitalWrite(csPin, LOW);                // select chip
  SPI.transfer(addr);                      // configure target pot with wiper position
  SPI.transfer(pos);
  digitalWrite(csPin, HIGH);               // de-select chip
  resistanceWB = ((rAB * potentiometerWiperPos) / maxPositions ) + rWiper;
}


void graphite_getResistance() {
  float sensorValue = analogRead(graphitePin);
  if (!calibrated) calibrate_potentiometer();
  else{
    Serial.println(sensorValue);                         // read the input on pin A0:
    float Rc = (float(1024)/float(sensorValue))*r1*(1+r3/float(resistanceWB))-r1-r5;
    Serial.print ("Resistance of graphite sensor ");
    Serial.println (Rc);
    //switchSerial.println(Rc);
  }   
}


void flex_getResistance() {
  int ADCflex = analogRead(flexPin);
  float Vflex = ADCflex * Vcc / 1023.0;
  float Rflex = R_DIV * (Vcc / Vflex - 1.0);
  Serial.println("Resistance: " + String(Rflex) + " ohms");

  // Use the calculated resistance to estimate the sensor's bend angle:
  float angle = map(Rflex, flatResistance, bendResistance, 0, 90.0);
  Serial.println("Bend: " + String(angle) + " degrees");
  //switchSerial.println(Rflex);
}


void screen_OLED() {
  if (touched == 0) {
    OLED_screen.clearDisplay();
    OLED_screen.setTextSize(2);                   // Taille des caractères (1:1, puis 2:1, puis 3:1)
    OLED_screen.setCursor(0, 0); 
    OLED_screen.setTextColor(SSD1306_WHITE);  
    OLED_screen.println("Flex");
    OLED_screen.println("Graphite");
    OLED_screen.println("GitHub QR");
    if(encoder0Pos == 1) {
      OLED_screen.setCursor(0, 0); 
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // Couleur du texte, et couleur du fond
      OLED_screen.println("Flex");
    }
    else if(encoder0Pos == 2) {
      OLED_screen.setCursor(0, 15); 
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // Couleur du texte, et couleur du fond
      OLED_screen.println("Graphite");
    }
    else if(encoder0Pos == 3) {
      OLED_screen.setCursor(0, 30); 
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // Couleur du texte, et couleur du fond
      OLED_screen.println("GitHub QR");
      }
    }
  else {
    if(encoder0Pos == 1) {
      OLED_screen.clearDisplay();
      OLED_screen.setCursor(0, 0);
      OLED_screen.setTextSize(3); 
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // Couleur du texte, et couleur du fond
      OLED_screen.println("Flex");
      OLED_screen.println("Sensor");
      Serial.println("flex touched");
    }
    else if(encoder0Pos == 2) {
      OLED_screen.clearDisplay();
      OLED_screen.setCursor(0, 0);
      OLED_screen.setTextSize(3); 
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // Couleur du texte, et couleur du fond
      OLED_screen.println("Lowtech");
      OLED_screen.println("Sensor");
      Serial.println("graphite touched");
    }
    else if(encoder0Pos == 3) {
      OLED_screen.clearDisplay();
      OLED_screen.drawBitmap(0, 0, epd_bitmap_frame, 128, 64, WHITE); //QR code from the homemade library QR.h
    } 
  }
  OLED_screen.display();
}


void encoder_interrupt() {
  if (digitalRead(encoder0PinA)==HIGH && digitalRead(encoder0PinB)==LOW) {
    if (encoder0Pos < 3){
      encoder0Pos++;
    } else encoder0Pos = 1;
  } else if (digitalRead(encoder0PinA)==HIGH && digitalRead(encoder0PinB)==HIGH) {
    if (encoder0Pos > 1){
      encoder0Pos--;
    } else encoder0Pos = 3;
  }
  // Serial.println (encoder0Pos, DEC);  //Angle = (360 / Encoder_Resolution) * encoder0Pos
}


void calibrate_potentiometer() {
  int Vadc = 0;
  Serial.println("Calibrating potentiometer");
  while(!calibrated) {
    potentiometer_setWiper(pot0, potentiometerWiperPos);
    Vadc = analogRead(graphitePin);
    Serial.print("Vadc : ");
    Serial.println(Vadc);
    if (potentiometerWiperPos < 2) {
      calibrated = 1;
      break;
    }
    if (Vadc < 500){
      potentiometerWiperPos--;
    }
    else if (Vadc > 520){
      potentiometerWiperPos++;
    }
    else calibrated = 1;
    Serial.println(potentiometerWiperPos);
    delay(100);
  }
}
