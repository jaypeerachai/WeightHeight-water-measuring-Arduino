/**
 * This is the program that is used for the Arduino especially Mega with Ultrasonic sensor (HC-SR04), weight scale (Load Cell) with HX711 amplifier broad, 
 * Micro SD card adapter, and GPRS+GPS Module (A7 Ai-Thinker).
 * This program has the purpose to measure and monitor distance and weight&height of water, send to moblie phone via SMS, and also write to SD card.
 *  Arduino Mega pin 6 -> HX711 CLK
 *  Arduino Mega pin 5 -> HX711 DOUT
 *  Arduino Mega pin 5V -> HX711 VCC
 *  Arduino Mega pin GND -> HX711 GND
 * 25/6/2018
 */
 
#include <SD.h>          // Import SD library for allowing reading from and writing to SD cards.
#include <SPI.h>         // Import SPI (Serial Peripheral Interface) library for communicating with SPI devices.
#include <HX711.h>       // Import HX711 library to be used for HX711 24-bit Analog-to-Digital Converter for load cell.

#define fileName "DISSMS10.txt"  // Intiallize name of file to use for reading and writing in SD card. This is the max name length, longer than this will not work.
#define g 9.8                    // Gravity
#define p 997                    // Density of water
#define r 0.00775                // Radius of hanging tube, this is the value that depend on the tube for each environment
#define pi 3.14159               // Pi
#define weightAir 88.40          // Weight of the hanging tube 

struct weightHeightWater         // Struct for storing the value of weight and height of water 
{
  float w;    // weight
  float h;    // height
};

File file;                  		   // Create file object named "file"
weightHeightWater weightHeight;   	   // Create struct of weightHeightWater named "weightHeight" 
const char phone_no[]="07930896294";   // Phone number to recieve the SMS
const int trigPin = 2;      		   // Intialize trigger pin to pin 2 For Ultrasonic sensor (HC-SR04).
const int echoPin = 3;      		   // Intialize echo pin to pin 3 For Ultrasonic sensor (HC-SR04).
const int loadcellDout = 5;            // Intialize dout pin for HX711 amplifier Load Cell.
const int loadcellClk = 6;             // Set clock pin for HX711 amplifier Load Cell.
const int CS_PIN = 53;      		   // Use pin 53 if connected to MEGA, 10 for UNO. Explain what CS_PIN does
int j=0;                    		   // To keep track of Logging - in other words this is a counter, initially set to zero and then incremented at each reading.
float bF, v;                		   // Buoyant force and Volume of part of the tube sunk in the water
float duration, distance;   		   // These are used later.
//float calibration_factor = -2350;    // With -2350, it works for 50 grams snickers
float calibration_factor = -2150;      // With -2150, it works for 88 grams tube

HX711 scale(loadcellDout, loadcellClk);   // Set analog pin of dout and clock

/**
 * The setup() function is used to initialize and set the initial values. 
 * This uses SD and HX711 library.
 */
void setup() 	
{       
  pinMode(CS_PIN, OUTPUT);    // Set CS_PIN  (53) to output mode
  pinMode(trigPin, OUTPUT);   // Set trigPin  (2) to output mode
  pinMode(echoPin, INPUT);    // Set echoPin  (3) to input mode
  scale.set_scale();		  // calibrate the scale
  scale.tare();               // Reset the scale to 0
  long zero_factor = scale.read_average();  //Get a baseline reading
  Serial.print("Zero factor: ");            //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  
  // check whether SD.begin is passed or not.
  if( SD.begin(CS_PIN) )
  {
    // Initialize the SD libary             
    Serial.println("SD.begin() - passed");
  }
  else
  {
    // failed to initialize SD library
    Serial.println("SD.begin() - failed");
  }
  
  delay(2000);                      // just wait for everything's readiness
  Serial.begin(115200);             // Set the maximum baud rate of Arduino Mega.
}

/**
 * The loop() function is used to actively control the Arduino board for measuring and monitoring distance and weight&height of water, 
 * sending to moblie phone via SMS, and also writing to SD card.
 */
void loop()
{
  j++;                              // To keep track of Logging
  distance = calculateDistance();   // call calculateDistance() function and keep value that returns from this function into "distance".
  weightHeight = calculateWeightHeight();  // call calculateWeightHeught() fucntion and keep value of weight into index[0] and height into index[].
  writeSD();                        // call writeSD() function to write to SD card.

  /*
   * Display on Monitor for checking correctness
   */
  Serial.print("Log ");
  Serial.print(j);
  Serial.print(": Distance ");
  Serial.print(distance);
  Serial.print(" || Weight: ");
  Serial.print(weightHeight.w, 5);
  Serial.print(" grams");
  Serial.print(" || Height: ");
  Serial.println(weightHeight.h, 5);
  
  sendSMS();      // call sendSMS() function to send data as SMS to mobile phone.
  
  delay(600000);  // How long for each interval in ms, 600000 for 10 minutes.
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
float calculateDistance()
{
  float d;  // distance
  digitalWrite(trigPin, LOW);         // reset the ultrasound device
  delayMicroseconds(2);               // wait for it to stabilise
  digitalWrite(trigPin, HIGH);        // trigger a reading
  delayMicroseconds(10);              // wait for a stable reading by holding the trigger pin high for 10 microseconds
  digitalWrite(trigPin, LOW);         // set trigger low again
  duration = pulseIn(echoPin, HIGH);  // Read the length of time the echo-pin is held high, which is proportional to distance
  d = duration*0.034/2.0;             // calculate actual distance
  if(distance > 1000) distance = -1;  // set a flag if the value is outside the range of distance
                                      // If the distance is either too close or too far, the device will return incorrect value. The optimal distance is between 2 cm and 300 cm.
  return d; // return value of distance
} 

 /** 
 * This function is used to calculate the weight and height of water. 
 * According to the equation 
 *   Fb = (weight in the air) - (weight in the water)
 * We keep the weight in the air before running the code
 * After we change 'weightAir' variable above, we can run the code and calculate Fb - Buoyant force
 *
 * According to the equation
 *   Fb = pVg
 * We know the Fb, density of the water, and g. 
 * Therefore, we can calculate Volume of the sunk part of the hanging tube with this -> V = Fb/(pg)
 *
 * According to the equation
 *   v = Pi*r^2*height
 * We can cal culate the volume of cylinder shape object.
 * Since we know the volume and the radius of the hanging tube, we can get the height of the sunk tube.
 *   height = v/(Pi*r^2)
 *   
 * @return wh
 */
weightHeightWater calculateWeightHeight()
{
  weightHeightWater wh;
  scale.set_scale(calibration_factor);  // adjust calibration factor
  wh.w = abs(scale.get_units());        // get weight value
  bF = weightAir - wh.w;
  v = bF / (p*g);
  wh.h = v / (pi*r*r);
  return wh;                            // return value of weight and height
}

/**
 * This function is used to write the data to SD card 
 * including value of distance and weight.
 */
void writeSD()
{
  file = SD.open(fileName, FILE_WRITE);   // open file "DISSMS10.txt" for writing.
  delay(5000);                            // Delay to let the system finish the work
  file.print("Log ");
  file.print(j);
  file.print(": Distance ");
  file.print(distance);
  file.print(" || Weight: ");
  file.print(weightHeight.w, 5);
  file.print(" grams");
  file.print(" || Height: ");
  file.println(weightHeight.h, 5);
  file.close();
}

/**
 * This function uses AT command to send the data to mobile phone via SMS.
 */
void sendSMS()
{
  Serial3.begin(115200);
  Serial3.println("AT+CMGF=1");   // set input and output format of messages to Text mode.
  delay(5000);                    // Delay to let the system finish the work
  Serial3.print("AT+CMGS=\"");    // Begin to send SMS to given number
  Serial3.print(phone_no);        // Phone number here
  Serial3.write(0x22);            // "
  Serial3.write(0x0D);            // /r
  Serial3.write(0x0A);            // /n                 /r/n = new line in Windows
  delay(5000);
  // The actual message in the SMS
  Serial3.print("Log ");
  Serial3.print(j);
  Serial3.print(": Distance ");
  Serial3.print(distance);
  Serial3.print(" || Weight: ");
  Serial3.print(weightHeight.w, 5);
  Serial3.print(" grams");
  Serial3.print(" || Height: ");
  Serial3.println(weightHeight.h, 5);
  delay(5000);
  Serial3.println(char(26));
  // The end of the message
  delay(5000); // Delay to let the system finish the work
  Serial3.end();
}





