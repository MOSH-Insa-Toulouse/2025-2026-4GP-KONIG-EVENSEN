/*
Program for the handling of a rotary encoder, an OLED screen, Bluetooth communication 
with an android application made with MIT App Inventor via a HC-05 bluetooth module, a
commercial flex sensor and an amplifier circuit using a digital potentiometer to determine
the resistance of a LowTech graphite strain sensor.
The program is based on three different modes:
"Flex Sensor mode":
- Measures the resistance of the commercial flex sensor and transfers it to the android application
  where it is plotted to a graph for resistance and relative resistance.
"LowTech Sensor mode":
- Measures the resistance of the LowTech graphite sensor and transfers it to the android application
  where it is plotted to a graph for resistance and relative resistance.
"GitHub QR":
- Displays a QR code from the homemade arduino library QR.h which leads directly to the GitHub page
  of this project, and can be scanned using the android application.


Full description of the wiring of the components needed for this program:
  /////////////////////////////////////////
   Arduino Uno Rotary Encoder connections:
   Uno            KY-040
   ------------------------
   GND           GND (pin 1)
   5V            +   (pin 2)
   D02           SW  (pin 3)
   D03           CLK (pin 5 (A))
   D04           DT  (pin 4 (B))
  /////////////////////////////////////////
   Arduino Uno OLED screen (I2C) connections:
   Uno           SBC-OLED01
   ------------------------
   GND           GND (pin 1)
   5V            VCC (pin 2)
   A04           SDA (pin 4)
   A05           SCL (pin 3)
  /////////////////////////////////////////
   Arduino Uno Serial Bluetooth connections:
   Uno            HC-05
   ------------------------
   5V            VCC (pin 1)
   GND           GND (pin 2)
   D07 (TxD)     RxD (pin 4)
   D08 (RxD)     TxD (pin 3)
  /////////////////////////////////////////
   Arduino Uno Flex Sensor connections:
   Uno         SpectraSymbol FS
   ------------------------
   5V            Vcc (pin 1)
   A01           Output (pin 2)
  /////////////////////////////////////////
   Arduino Uno SPI connections:
   Uno            MCP41100
   ------------------------
   D10 (SS)      CS  (pin 1)
   D11 (MOSI)    SI  (pin 3)
   D13 (SCK)     SCK (pin 2)
   GND           VSS (pin 4)
   GND           PA0 (pin 5)
   GND           PWD (pin 6)
   (AOP pin 6)   PB0 (pin 7)
   5V            VDD (pin 8)
  /////////////////////////////////////////
   Arduino Uno Amplifier Circuit connections:
   Uno            LTC1050
   ------------------------
   A00           VADC
   5V            Vcc (Graphite sensor pin 1)
   GND           V- (pin 4)
   5V            V+ (pin 7)
   See KiCad_Schema.png for more information
   about the circuit wiring.
  /////////////////////////////////////////
*/


// Inclusion of libraries needed for the program
#include <SPI.h> // Library used for SPI bus, communication with the digital potentiometer
#include <Adafruit_SSD1306.h> // Library used for the initialization of the OLED screen
#include <QR.h> // Library used for the implementation of a QR code leading directly to the GitHub for this project MOSH-Insa-Toulouse/2025-2026-4GP-KONIG-EVENSEN
#include <SoftwareSerial.h> // Library used for the Bluetooth serial communication between an android application created with the program MIT App Inventor and a HC-05 Bluetooth module


// Variables for the rotary encoder
#define Switch        2       // Switch connection
#define encoder0PinA  3       // CLK Output A needs to be at pin 2 or 3 as we are using interrupts
#define encoder0PinB  4       // DT Output B
int lastButton = HIGH;        // Variable for the last state of the rotary encoder button in order to handle clicks
volatile int encoder0Pos = 1; // Variable for the position of the rotary encoder in the menu list
volatile int touched = 0;     // Variable for determining if the rotary encoder has been clicked


// Variables for the OLED screen
#define OLED_width      128         // Width of OLED screen in pixels
#define OLED_height     64          // Height of OLED screen in pixels
#define OLED_pinReset   -1          // Reset of OLED shared with the Arduino
#define OLED_I2Caddress 0x3C        // Address of OLED screen on I2C bus corresponding to 0x78 at the back of the OLED screen
Adafruit_SSD1306 OLED_screen(OLED_width, OLED_height, &Wire, OLED_pinReset); // Initialization of the OLED screen


// Variables for the Bluetooth serial communication
byte serialRX;          // Variable for the bytes received from the application
volatile byte RX = 0;   // Variable for the state of RX
int start = 0;          // Variable for determining if a measure should be done
SoftwareSerial switchSerial (8,7); // Initialization of the serial communication of the bluetooth, sets the pin 7 as TxD and 8 as RxD on the Arduino UNO


// Variables for the commercial flex sensor
const int flexPin = A1;                 // Pin connected to voltage divider output
const float Vcc = 5;                    // Voltage at Ardunio 5V line
const float R_DIV = 47000.0;            // Resistor used to create a voltage divider 47kOhm
const float flatResistance = 22000.0;   // Resistance when flat
const float bendResistance =  63000.0;  // Resistance at 90 deg


// Variables for the potentiometer
const byte csPin                    = 10;      // MCP42100 chip select pin
const int  maxPositions             = 256;     // The digital potentiometer wiper can move from 0 to 255 = 256 positions
const long rAB                      = 92500;   // 100k pot resistance between terminals A and B
const byte rWiper                   = 125;     // 125 ohms pot wiper resistance
const byte pot0                     = 0x11;    // pot0 addr // B 0001 0001
const byte pot0Shutdown             = 0x21;    // pot0 shutdown // B 0010 0001
volatile int potentiometerWiperPos  = 20;      // Initial wiper position of the digital potentiometer corresponding to 7351 Ohms
long       resistanceWB             = ((rAB * potentiometerWiperPos) / maxPositions ) + rWiper;   // Works as R2 in the amplifier circuit
volatile int calibrated             = 0;       // Variable for determining if the potentiometer has been calibrated


// Variables for the amplification circuit of the graphite sensor
const int graphitePin = A0;           // The signal Vadc is received on the pin A0 of the Arduino
const int VccBits = 1024;             // The Vcc voltage of 5V given in bits
const float r1 = 100.0*(pow(10,3));   // The resistance value of R1 100kOhm
const float r3 = 100.0*(pow(10,3));   // The resistance value of R3 100kOhm
const float r5 = 10.0*(pow(10,3));    // The resistance value of R5 10kOhm


// Setup needed for declaring pinmodes and establishing communication routes
void setup() {
  // Initialization of serial monitor, baudrate 9600
  Serial.begin(9600);

  // Rotary encoder
  pinMode(encoder0PinA, INPUT);           // Set CLK pin to input
  digitalWrite(encoder0PinA, HIGH);       // Turn on pullup resistor
  pinMode(encoder0PinB, INPUT);           // Set DT pin to input
  digitalWrite(encoder0PinB, HIGH);       // Turn on pullup resistor
  attachInterrupt(1, encoder_interrupt, RISING);  // Encoder pin on interrupt 1 --> pin D03 of the Arduino UNO

  // OLED
  if(!OLED_screen.begin(SSD1306_SWITCHCAPVCC, OLED_I2Caddress)) // Initialization of OLED screen
    while(1); // Stops the program by creating an infinite loop if the OLED initialization fails

  // Bluetooth module
  pinMode(8, INPUT);        // Declares RxD as input on the arduino
  pinMode(7, OUTPUT);       // Declares TxD as output on the arduino
  switchSerial.begin(9600); // Initialization of switchSerial used for communication over Bluetooth, baudrate 9600
  switchSerial.flush();     // Flushes any data left on the communication bus

  // Flex Sensor
  pinMode(flexPin, INPUT); // Set flexPin in input mode

  // Potentiometer
  digitalWrite(csPin, HIGH);   // Chip select default to de-selected
  pinMode(csPin, OUTPUT);      // Configure chip select as output
  SPI.begin();                 // Initialization of SPI bus for communication with the digital potentiometer

  // Graphite Sensor
  pinMode(graphitePin, INPUT); // Set graphitePin in input mode
}


// Loop function runs continuously when the program is uploaded to the Arduino UNO
void loop() {
  // Prints the position of the rotary encoder from the menu
  Serial.print("Position:");
  Serial.println (encoder0Pos, DEC);
  
  // Reads the rotary encoder button to check if it has been clicked
  read_encoder_button();
  
  // Updates the OLED screen
  screen_OLED();

  // Creates a switchSerialEvent every time a message is sent from the application over Bluetooth
  if (switchSerial.available() > 0){
    switchSerialEvent();
  }

  // When a switchSerialEvent is triggered, this function runs, starting and stopping the measurement
  bluetooth_data_handling();
  
  // This function, sending the resistance value of the flex sensor (1) or the graphite sensor (2), runs between the instants where "Start" and "Stop" are clicked
  if (start){
    if (touched == 1 && encoder0Pos == 1) {
      flex_getResistance();
    } 
    if (touched == 1 && encoder0Pos == 2) {
      graphite_getResistance();
    } 
  }

  // Delay between loops to increase the stability of the program
  delay(500);
}


/////////////////////////////////////////// ROTARY ENCODER FUNCTIONS ////////////////////////////////////////////////
// Function managing a rotary encoder button click
void read_encoder_button(){
  int currentButton = digitalRead(Switch); // Reads the current state of SW, 0 when held in, 1 when at rest
  if (lastButton == HIGH && currentButton == LOW){ // Triggered when the current state of SW is 0 and the last was 1
    touched = !touched; // Changes the value of touched, 1 when entering a menu choice, 0 when at main menu
    delay(50); // Simple debounce for stability
  }
  lastButton = currentButton; // Updates the last button state
}


// Function for handling interrupts when the rotary encoder is rotated
void encoder_interrupt() {
  // This case corresponds to a clockwise rotation of the rotary encoder and increases the encoder0Pos up to 3 until it resets at 1
  if (digitalRead(encoder0PinA)==HIGH && digitalRead(encoder0PinB)==LOW) {
    if (encoder0Pos < 3){
      encoder0Pos++;
    } else encoder0Pos = 1;
  // This case corresponds to a anti-clockwise rotation of the rotary encoder and decreases the encoder0Pos down to 1 until it resets at 3
  } else if (digitalRead(encoder0PinA)==HIGH && digitalRead(encoder0PinB)==HIGH) {
    if (encoder0Pos > 1){
      encoder0Pos--;
    } else encoder0Pos = 3;
  }
}


/////////////////////////////////////////// OLED SCREEN FUNCTIONS ///////////////////////////////////////////////////
void screen_OLED() {
  // If touched is 0, the main menu is displayed
  if (touched == 0) {
    OLED_screen.clearDisplay();               // Clears the display
    OLED_screen.setTextSize(2);               // Sets the size of the characters
    OLED_screen.setCursor(0, 0);              // Initializes the cursor at (0,0)
    OLED_screen.setTextColor(SSD1306_WHITE);  // Sets the text color to white
    // Prints the menu choices in order: "Flex", "Graphite" and "GitHub QR"
    OLED_screen.println("Flex");
    OLED_screen.println("Graphite");
    OLED_screen.println("GitHub QR");
    // Changes the color of the background to white and the text to black for the current menu choice
    if(encoder0Pos == 1) { // encoder0Pos == 1 corresponds to the "Flex" choice
      OLED_screen.setCursor(0, 0); // Sets the cursor at (0,0) which corresponds to the first line
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Text color black and background color white
      OLED_screen.println("Flex"); // Reprints the "Flex" choice with new colors
    }
    else if(encoder0Pos == 2) { // encoder0Pos == 2 corresponds to the "Graphite" choice
      OLED_screen.setCursor(0, 15); // Sets the cursor at (0,15) which corresponds to the second line
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Text color black and background color white
      OLED_screen.println("Graphite"); // Reprints the "Graphite" choice with new colors
    }
    else if(encoder0Pos == 3) { // encoder0Pos == 3 corresponds to the "GitHub QR" choice
      OLED_screen.setCursor(0, 30); // Sets the cursor at (0,30) which corresponds to the third line
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Text color black and background color white
      OLED_screen.println("GitHub QR"); // Reprints the "GitHub QR" choice with new colors
      }
    }
  // If touched is 1, a choice of mode has been made by clicking the rotatry encoder button, and the corresponding mode is displayed
  else {
    if(encoder0Pos == 1) { // encoder0Pos == 1 corresponds to the "Flex sensor" mode
      OLED_screen.clearDisplay();       // Clears the display
      OLED_screen.setCursor(0, 0);      // Initializes the cursor at (0,0)
      OLED_screen.setTextSize(3);       // Increases the size of the characters
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Text color black and background color white
      // Prints "Flex Sensor" to the OLED screen
      OLED_screen.println("Flex");
      OLED_screen.println("Sensor");
    }
    else if(encoder0Pos == 2) { // encoder0Pos == 2 corresponds to the "Lowtech sensor" mode
      OLED_screen.clearDisplay();       // Clears the display
      OLED_screen.setCursor(0, 0);      // Initializes the cursor at (0,0)
      OLED_screen.setTextSize(3);       // Increases the size of the characters
      OLED_screen.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Text color black and background color white
      // Prints "Lowtech Sensor" to the OLED screen
      OLED_screen.println("Lowtech");
      OLED_screen.println("Sensor");
    }
    else if(encoder0Pos == 3) { // encoder0Pos == 3 corresponds to the "GitHub QR" mode
      OLED_screen.clearDisplay(); // Clears the display
      OLED_screen.drawBitmap(0, 0, epd_bitmap_frame, 128, 64, WHITE); // Prints a bitmap of the QR code from the homemade library QR.h
    } 
  }
  // Updates the information displayed on the OLED screen
  OLED_screen.display();
}


/////////////////////////////////////////// BLUETOOTH COMMUNICATION FUNCTIONS ///////////////////////////////////////
// switchSerialEvent is run every time data is received from the application via Bluetooth
void switchSerialEvent(){
  serialRX = switchSerial.read(); // Reads the message from the application
  RX = 1; // Switches the state of RX
}


// Function for handling the incomming data from the application via Bluetooth
void bluetooth_data_handling(){
  if (RX){ // RX is 1 if a switchSerialEvent has been triggered, indicating there is data to receive
    RX=0;  // Resets the state of RX so that the function is only called when a new message is sent from the application
    switch (serialRX) {
    case 1: // If the Arduino UNO receives the message 1, start is set to 1 and a measure starts
      Serial.println("Start button pressed!");
      start = 1;
      break;
    case 2: // If the Arduino UNO receives the message 2, start is set to 0 and the measure stops
      Serial.println("Stop button pressed!");
      start = 0;
      break;
    }
  }
}


/////////////////////////////////////////// FLEX SENSOR FUNCTIONS ///////////////////////////////////////////////////
// Function for determining the resistance of the commercial flex sensor
void flex_getResistance() {
  int ADCflex = analogRead(flexPin); // Gets the value of the flexPin in bits
  float Vflex = ADCflex * Vcc / 1023.0; // Transfers the bit value into tension
  float Rflex = R_DIV * (Vcc / Vflex - 1.0); // Transfers the tension value into resistance
  Serial.println("Resistance: " + String(Rflex) + " ohms"); // Prints the resistance to the serial monitor
  switchSerial.println(Rflex); // Transfers the resistance value to the application over Bluetooth

  // Uses the calculated resistance to estimate the sensor's bend angle:
  float angle = map(Rflex, flatResistance, bendResistance, 0, 90.0); // Calculates the angle based on the flat resistance, the resistance at 90 degrees and the current resistance
  Serial.println("Bend: " + String(angle) + " degrees"); // Prints the current angle to the serial monitor
}


/////////////////////////////////////////// DIGITAL POTENTIOMETER FUNCTIONS /////////////////////////////////////////
// Function for changing the value of the digital potentiometer
void potentiometer_setWiper(int addr, int pos) {
  pos = constrain(pos, 0, 255);            // Limits wiper setting to range of 0 to 255
  digitalWrite(csPin, LOW);                // Selects chip
  SPI.transfer(addr);                      // Configures target pot with wiper position
  SPI.transfer(pos);                       // Configures potentiometer position
  digitalWrite(csPin, HIGH);               // De-selects chip
  resistanceWB = ((rAB * potentiometerWiperPos) / maxPositions ) + rWiper; // Calculates the new potentiometer resistance
}


// Function for calibrating the potentiometer the first time "graphite mode" is entered
void calibrate_potentiometer() {
  int Vadc = 0; // Initializes Vadc
  Serial.println("Calibrating potentiometer");
  while(!calibrated) {
    potentiometer_setWiper(pot0, potentiometerWiperPos); // Sets the current wiper position
    Vadc = analogRead(graphitePin); // Checks the current value of Vadc (output signal form the graphite sensor)
    Serial.print("Vadc : ");
    Serial.println(Vadc); // Prints the current Vadc value to the serial monitor
    // If Vadc never reaches the goal value the potentiometer will continue lowering its wiper position, this if stops it at 1
    if (potentiometerWiperPos < 2) { 
      calibrated = 1;
      break;
    }
    // Sets the potentiometer value so that the graphite sensor signal ends up between 500 and 520 bits (the middle of the spectrum)
    if (Vadc < 500){
      potentiometerWiperPos--;
    }
    else if (Vadc > 520){
      potentiometerWiperPos++;
    }
    else calibrated = 1;
    // Prints the wiper position of the potentiometer to the serial monitor
    Serial.println(potentiometerWiperPos);
    delay(100); //Small delay for stability
  }
}


/////////////////////////////////////////// GRAPHITE SENSOR FUNCTIONS ///////////////////////////////////////////////
// Function for determining the resistance of the LowTech graphite sensor
void graphite_getResistance() {
  // Updates the potentiometer value
  potentiometer_setWiper(pot0, potentiometerWiperPos);

  // Calculates and transmits the resistance value of the graphite sensor
  float sensorValue = analogRead(graphitePin); // Reads the input value on pin A0 in bits
  if (!calibrated) calibrate_potentiometer();  // Calibrates the digital potentiometer the first time the program enters "graphite mode"
  else{ // Runs when in "graphite mode" after the first calibration of the potentiometer
    float Rc = (float(1024)/float(sensorValue))*r1*(1+r3/float(resistanceWB))-r1-r5; // Calculates the resistance of the graphite sensor
    Serial.print ("Resistance of graphite sensor ");
    Serial.println (Rc); // Prints the resistance to the serial monitor
    switchSerial.println(Rc); // Transfers the resistance value to the application over Bluetooth
  }   
}
