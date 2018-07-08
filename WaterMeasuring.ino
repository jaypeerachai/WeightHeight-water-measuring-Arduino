/**
 * This is the program that is used for the Arduino especially mega with Ultrasonic sensor (HC-SR04), weight scale (Load Cell) with HX711 amplifier borad, 
 * Micro SD card adapter, and GPRS+GPS Module (A7 Ai-Thinker).
 * This program has the purpose to measure and monitor distance and weight&height of water, send to moblie phone via SMS, and also write to SD card.
 *
 * Pin connection between Arduino Mega and HX711 
 *  Arduino Mega pin 6 -> HX711 CLK
 *  Arduino Mega pin 5 -> HX711 DOUT
 *  Arduino Mega pin 5V -> HX711 VCC
 *  Arduino Mega pin GND -> HX711 GND
 *  
 * Pin connection between Arduino Mega and A7 Ai-Thinker
 *  Arduino Mega TX3 -> A7 Ai-Thinker RX
 *  Arduino Mega RX3 -> A7 Ai-Thinker TX
 *  A7 Ai-Thinker 5V -> A7 Ai-Thinker PWR
 *  
 * Pin connection between Arduino Mega and Load Cell and HX711
 *  HX711 E+ -> Load Cell RED
 *  HX711 E- -> Load Cell BLACK
 *  HX711 A- -> Load Cell WHITE
 *  HX711 A+ -> Load Cell GREEN
 *  
 * Pin Connection between Arduino Mega and MicroSD card adapter
 *  Arduino Mega pin 53 -> MicroSD card adapter SS
 *  Arduino Mega pin 51 -> MicroSD card adapter MOSI
 *  Arduino Mega pin 50 -> MicroSD card adapter MISO
 *  Arduino Mega pin 52 -> MicroSD card adapter SCK
 *
 */
 
 /**
 * - Optimize the code by grouping code as a function, adding more comment, and adjusting the delay time.
 * - Add more comment and description.
 * 25/6/2018
 */

/**
 *  - Add time in the message by using <TimeLib.h>
 *  - In order to send data out, it need to send Time + Code + Data
 *    The Code section is a string of characters that the reciever can determine the
 *    coordinator of that sensor.
 *  - Add 'r' button when use with LoadCell to change the weight in air to the current weight
 * read by the LoadCell
 *  - Added the BootPin and SleepPin from David to help save power when the board go to sleep mode.
 *  - Added David's moethod for changing weight into height
 * 2/7/2018
 */

/**
 * - The calibration method with 2 known weight object is added
 * - Add the option for the user to enter 3 value at the beginning of the program
 *    - The first height and the second height
 *    - The offset for the ultrasonic
 * 
 * - Removed the time stamp when sending with SMS
 * - Removed many unused variables from the program, like g and pi.
 * 5/7/2018
 */


#include <SD.h>          // Import SD library for allowing reading from and writing to SD cards.
#include <SPI.h>         // Import SPI (Serial Peripheral Interface) library for communicating with SPI devices.
#include <HX711.h>       // Import HX711 library to be used for HX711 24-bit Analog-to-Digital Converter for load cell.
#include <TimeLib.h>     // To send data with time.

#define fileName "DISSMS10.txt"  // Intiallize name of file to use for reading and writing in SD card. This is the max name length, longer than this will not work.

String message;          // message is use to keep all data that will be send along with time and code

struct weightHeightWater         // Struct of water to store the value of weigth and height of water 
{
  float w;    // weight
  float h;    // height
};

File file;                  // Create file object named "file"
weightHeightWater weightHeight;   // Create struct of weightHeightWater named "weightHeight" 
const char phone_no[]="07930895716";   // Phone number to recieve the SMS
const char code[] = "A-1";
const int trigPin = 2;      // Intialize trigger pin to pin 2 For Ultrasonic sensor (HC-SR04).
const int echoPin = 3;      // Intialize echo pin to pin 3 For Ultrasonic sensor (HC-SR04).
const int loadcellDout = 5; // Intialize dout pin for HX711 amplifier Load Cell.
const int loadcellClk = 6;  // Set clock pin for HX711 amplifier Load Cell.
const int CS_PIN = 53;      // Use pin 53 if connected to MEGA, 10 for UNO. Explain what CS_PIN does
const int BootPin = 7;      // Intialize trigger pin to pin 7 to boot the GSM board.
const int SleepPin = 8;      // Intialize trigger pin to pin 8 to sleep and wake the GSM board.
int j=0;                    // To keep track of Logging - in other words this is a counter, initially set to zero and then incremented at each reading.
float duration, distance;   // These are used later.
float calibration_factor = -2200;     // With -2150, it works for 88 grams tube
float offset = 0;           // Use for distance measurement to turn read data into water level

float height1, height2, weight1, weight2, currentweight, currentheight;
//height1, height2, weight1, weight2 are use for two-known-weight calibration
//currentweight and currentheight are the variable to kept the data that sensor read in one time
String num; //to keep temporary user input
HX711 scale(loadcellDout, loadcellClk);   // Set analog pin of dout and clock

/**
 * The setup() function is used to initialize and set the initial values. 
 * This uses SD and HX711 library.
 */
void setup(){       
  pinMode(CS_PIN, OUTPUT);    // Set CS_PIN  (53) to output mode
  pinMode(trigPin, OUTPUT);   // Set trigPin  (2) to output mode
  pinMode(echoPin, INPUT);    // Set echoPin  (3) to input mode

  // Set up pin for Wake up and Sleep cycle
  pinMode(BootPin, OUTPUT);     // Set BootPin (7) to output mode to Boot the GSM board each time the Arduino board starts.
  pinMode(SleepPin, OUTPUT);    // Set SleepPin (8) to output mode to sleep and wake the GSM board at each reading cycle.
  digitalWrite(BootPin, HIGH);                 // start the GSM board by sending the BootPin high for >2 seconds
  delay(3000);
    
  digitalWrite(BootPin, LOW);                  //Power ON..     // It should be running now - and the two power lights should be on
  Serial.println("A7 Power ON!");
  
  
  scale.set_scale();
  scale.tare();               // Reset the scale to 0
  //long zero_factor = scale.read_average();  //Get a baseline reading
  //Serial.print("Zero factor: ");            //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  //Serial.println(zero_factor);

  Serial.begin(115200);             // Set the maximum baud rate of Arduino Mega.
  Serial.println("Enter first calibration height in centimeter.");
  while (!Serial.available());   //make the board stop and wait user to enter the input
  num = Serial.readString();     //read user input into String
  height1 = num.toFloat();       //convert String to float
  scale.set_scale(calibration_factor);  // adjust calibration factor
  weight1 = abs(scale.get_units());       // get weight value
  Serial.print("Height1 and Weight 1 is ");
  Serial.print(height1);
  Serial.print(" and ");
  Serial.println(weight1);
  
  Serial.println("Enter second calibration height in centimeter.");
  while (!Serial.available());   //make the board stop and wait user to enter the input
  num = Serial.readString();     //read user input into String
  height2 = num.toFloat();       //convert String to float
  scale.set_scale(calibration_factor);  // adjust calibration factor
  weight2 = abs(scale.get_units());       // get weight value
  Serial.print("Height2 and Weight2 is ");
  Serial.print(height2);
  Serial.print(" and ");
  Serial.println(weight2);
  
  Serial.println("Enter the offset, the distance from loadCell to the bottom.");
  while (!Serial.available());    //make the board stop and wait user to enter the input
  num = Serial.readString();      //read user input into String
  offset = num.toFloat();         //convert String to float
  Serial.print("Offset is ");
  Serial.println(offset);

  
  // check whether SD.begin is passed or not.
  if( SD.begin(CS_PIN) ){
    // Initialize the SD libary             
    Serial.println("SD.begin() - passed");
  }
  else{
    // failed to initialize SD library
    Serial.println("SD.begin() - failed");
  }
  delay(2000);                      // just wait for everything's readiness..
}

/**
 * The loop() function is used to actively control the Arduino board for measuring and monitoring distance and weight, 
 * sending to moblie phone via SMS, and also writing to SD card.
 */
void loop(){
  
  // Wake up routine
  digitalWrite(SleepPin, HIGH);   // wake up
  delay(1000);
  Serial.println("The A7 will wake up from low-power mode"); 
  
  
  j++;                              // To keep track of Logging
  distance = calculateDistance();   // call calculateDistance() function and keep value that returns from this function into "distance".
  weightHeight = calculateWeightHeight();  // call calculateWeightHeught() fucntion and keep value of weight into index[0] and height into index[].
  writeSD();                        // call writeSD() function to write to SD card.
  
  /**
   *  Create String to use later in the program
   *  The String 'message' will be used later when write to SD and SMS
   */
  message = String(j) + ", ";
  message += String(code) + ", " + distance + ", ";
  message += String(weightHeight.w) + ", " + String(weightHeight.h);
  Serial.println(message);
  
  sendSMS();      // call sendSMS() function to send data as SMS to mobile phone.
  delay(100);  // How long for each interval in ms, 600000 for 10 minutes.

  
  // Sleep Routine
  Serial.println("The A7 will go to sleep in low-power mode now. Sweet dreams.");
  digitalWrite(SleepPin, LOW);  //Sleep

  delay(30000);  // How long for each interval in ms, 30000 for 30 seconds
}

/**
 * This function is used to calculate the actual distance.
 * Speed of sound = 340 m/s = 34000 cm/s = 34 cm/ms = 0.034 cm/µs
 * time = distance / speed
 * distance = time x speed
 * distance apart (cm) = time x speed / 2 = time x 0.034 / 2   *** distance is halved as signal goes to the object and back
 * therefore, d = duration*0.034/2.0
 * 
 * @return d
 */
float calculateDistance(){
int i, n = 20;
  float validNum = n;   // Count the number of valid read from ultrasonic
  float accDis = 0;
  for(i=0; i<n; i++){
    float d;  // distance
    digitalWrite(trigPin, LOW);            // reset the ultrasound device
    delayMicroseconds(2);               // wait for it to stabilise
    digitalWrite(trigPin, HIGH);           // trigger a reading
    delayMicroseconds(10);              // wait for a stable reading by holding the trigger pin high for 10 microseconds
    digitalWrite(trigPin, LOW);            // set trigger low again
    duration = pulseIn(echoPin, HIGH);     // Read the length of time the echo-pin is held high, which is proportional to distance
    d = duration*0.034/2.0;             // calculate actual distance
    if(d > 1000) {
      d = 0;                            // set a flag if the value is outside the range of distance
      validNum--;
      if (validNum == 0) validNum = 1;  // If the valid number is 0, the devider can't be 0.
    }
    // If the distance is either too close or too far, the device will return incorrect value. The optimal distance is between 2 cm and 300 cm.
    else{
      accDis += d;
    }
  }
  return offset - (accDis/validNum);               // return value of distance 
} 

/**
 * This function will calibrate the weight read by loadCell
 * By using 2 known weight method
 */
weightHeightWater calculateWeightHeight(){
  weightHeightWater wh;
  scale.set_scale(calibration_factor);  // adjust calibration factor
  wh.w = abs(scale.get_units());        // get weight value
  wh.h = height1-(((weight1-wh.w)/(weight1-weight2))*(height1-height2));
  return wh;                             // return value of weight and height
}

/**
 * This function is used to write the data to SD card 
 * including value of distance and weight.
 */
void writeSD(){
  file = SD.open(fileName, FILE_WRITE);   // open file "DISSMS10.txt" for writing.
  delay(5000);                            // Delay to let the system finish the work
  file.print(message);
  file.close();
}

/**
 * This function uses AT command to send the data to mobile phone via SMS.
 */
void sendSMS(){
  Serial3.begin(115200);
  Serial3.println("AT+CMGF=1");   // set input and output format of messages to Text mode.
  delay(5000);                    // Delay to let the system finish the work
  Serial3.print("AT+CMGS=\"");    // Begin to send SMS to given number
  Serial3.print(phone_no);        // Phone number here
  Serial3.write(0x22);            // 
  Serial3.write(0x0D);            // /r
  Serial3.write(0x0A);            // /n                 /r/n = new line in Windows
  delay(5000);
  // The actual message in the SMS
  Serial3.print(message);
  delay(5000);
  Serial3.println(char(26));
  // The end of the message
  delay(5000); // Delay to let the system finish the work
  Serial3.end();
}
