/* Written by Fawadul Haq, Last Edited: 8/14/2019
 * Adapted from Aswinth Raj on Circuit Digest
 * Citation: https://circuitdigest.com/microcontroller-projects/arduino-touch-screen-calculator-tft-lcd-project-code
 * Libraries Used: SPDF5408
 * https://github.com/JoaoLopesF/SPFD5408
 */

// Libraries //
#include <SPFD5408_Adafruit_GFX.h>    // Core graphics library
#include <SPFD5408_Adafruit_TFTLCD.h> // Hardware-specific library
#include <SPFD5408_TouchScreen.h>     // LCD library
///////////////

// I/O //
  // LCD //
  /* Resistive Touch Pins Legend:
   *  First Letter in name: Signifies Axis, X or Y
   *  Second letter: Positive or Negative Deflection
   */
  #define YP A3  // Analog Input pin 3
  #define XN A2  // 
  #define YN 9   // Digital pin 3
  #define XP 8   // 
  /* LCD Commands
   * CS = Chip Select, used for accessing the SD Card
   * CD = Command Selector pin, tied to the SD Card
   * WR = Pin used to write to the LCD
   * RD = Pin used to read from the LCD
   * RESET = Pin used to reset the screen 
   */
  #define LCD_CS A3 
  #define LCD_CD A2 
  #define LCD_WR A1
  #define LCD_RD A0
  #define LCD_RESET A4

  // Sensor //
  #define SEN_RD A5 // Analog input of sensor
//////////

// Screen Settings //
#define WHITE   0xFFFF    //Black->White
#define YELLOW  0xFFE0    //Blue->Yellow
#define RED     0xF800    //Red->Cyan
#define PINK    0x07E0    //Green-> Pink
#define CYAN    0x07FF    //Cyan -> Red
#define GREEN   0x07E0    //Pink -> Green 
#define BLUE    0x001F    //Yellow->Blue
#define BLACK   0x0000    //White-> Black
#define MINPRESSURE 10    // predetermined pressure values in order to be considered a "touch" input
#define MAXPRESSURE 1000 
//////////////////////

// Calibration Params //
#define MAXX 297    // Max X Coordinate
#define MAXY 248    // Max Y Coordinate
#define TS_MINX 113 // Max and min pressures in the x and y coordinates
#define TS_MINY 85  // these further narrow the pressure range to be considered a touch
#define TS_MAXX 977
#define TS_MAXY 923
////////////////////////

// LCD //
TouchScreen ts = TouchScreen(XP, YP, XN, YN, 300); // 300 is the sensitivity
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
/////////

// Graphics //
#define BBLOCX 0    // "Begin Button Location X"
#define BBLOCY 120
#define BBLEN MAXX  // "Begin Button Length"
#define BBWID 60    // "Begin Button Width"

#define SLOCX 0     // "Start Location X"
#define SLOCY 30
/////////////

// Variables //
#define SIZEOFDATA 300   // predetermined size of data array
int alcData[SIZEOFDATA]; // "alcohol Data": Holds 300 data points from the alcohol sensor, later to be averaged
float BAC;
boolean retest = false;  // determines whether to reset the interface at the "Begin Test"
                         // screen or the Countdown screen
int textSize = 4;        // Default text size
unsigned long recovTime; // Timestamp of when recovery process starts after any one use 
/////////////////////


void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(2);
}

///////////////////////////////////////////////////////////////////
void loop() {
  
  // Begin
  if(!retest){
    dispStart();
    while(true){    // "Begin" button touched?
      TSPoint p = waitTouch();
      // if the touch input is within the bounds of the Begin Button
      if(p.x > BBLOCX && p.x < BBLEN){
        if(p.y > BBLOCY && p.y < BBLOCY + BBWID){
          break; 
        }
      }
    } 
  }
  // Display Countdown
  dispCountdown();
  
  // "Calculating" screen
  dispCalc();
  
  unsigned long startTime = millis();
  int i = 0;
  // This while loop collects 300 data points from the alcohol sensor
  // or at least data for a full five seconds
  // Which will later be averaged and used to calculate the BAC
  while(millis() - startTime <= 5000){  // Wait ~5 seconds
    
      // Temporarily storing data
      if(i <= SIZEOFDATA)
        { alcData[i] = analogRead(SEN_RD); }
      delay(20);  // wait 20ms until next reading
    
    i++;
  }

  recovTime = millis(); // start time stamp for recovery process

  // Calculate BAC
  calcData();
  
// Results page
  if(BAC < 0.06){
    tft.fillScreen(GREEN);
  }
  else if( BAC >= 0.08){
    tft.fillScreen(RED);
  }
  else if (BAC == 0.06 || BAC == 0.07){
    tft.fillScreen(YELLOW);
  }
  else {
    tft.fillScreen(YELLOW);
  }

  dispResults();
  delay(10000);   // Display results for 10 seconds
  
  // Delete Data
  BAC = 0;

  // Retest?
  dispRetest();
  
  while(true){    // "Yes" or "No" button touched?
      TSPoint p = waitTouch();
      // If touch is within Yes box, retest = true
      if(p.x > 230 && p.x < MAXX - 10){
        if(p.y > 20 && p.y < 130){
          retest = true;
          break; 
        }
      }
      // If touch is within No box, retest = false
      else if(p.x > 10 && p.x < 120){
        if(p.y > 20 && p.y < 130){
          retest = false;
          break; 
        }
      }
    }

  recovery(); // wait for the device to recover
  
}

/////////////////////  END OF MAIN ///////////////////////////

// Initial screen that displays for user to begin breathalyzer test
void dispStart(){

  tft.fillScreen(BLACK);
  tft.fillRect(BBLOCX,BBLOCY,BBLEN,BBWID,GREEN); // Begin button
  
  tft.setCursor(BBLOCX, BBLOCY + 15);
  tft.setTextSize(textSize);
  tft.setTextColor(WHITE);
  tft.println("BEGIN TEST"); 

}

// Screen and animation displayed when breathalyzer sensor is being used
void dispCountdown(){
  // Countdown animation
  tft.fillScreen(BLACK);
  tft.setTextSize(textSize);
  tft.setTextColor(WHITE);   

  // Numbers
  for (int j = 3; j >= 1; j--){
    startMsg();
    dispNum(j);
    delay(1000);
    tft.fillScreen(BLACK);
  }
  
}

// Displays the screen used when blowing into the tube
void dispCalc(){
  tft.fillScreen(BLACK);

  tft.setCursor(0, 120);
  tft.setTextSize(textSize-2);
  tft.setTextColor(WHITE);
  tft.println("Reading Please Wait");
}

// Displays the results of the calculation
void dispResults() {
  tft.setCursor(0, 0);
  tft.setTextSize(textSize-1);
  tft.setTextColor(WHITE);

  tft.println("Your Current BAC is:");

  tft.setCursor(80,100);
  tft.print(String(BAC));
  tft.println("%");
}

// Calculates the BAC given the alcohol sensor values
// Uses the ratio between two different resistors on the breakout board
// Reference: https://www.teachmemicro.com/mq-3-alcohol-sensor/
void calcData() {

  float sensor_volt; // analog sensor value converted into voltage
  float RS_gas;      // Resistance of sensor
  float R0 = 16000;  // Resistance of peripheral sensor (predetermined through testing)
  float ratio;       // ratio = RS_gas/R0
  int R2 = 1000;     // Resistance of peripheral sensor on breaker board
  unsigned long sensorVal = 0;  // holds the averaged answer 
  
  // Average the alcohol Data
  for(int i = 0; i < SIZEOFDATA; i++){
    sensorVal += alcData[i];
  }
  sensorVal /= SIZEOFDATA;

  // Conversion into BAC
  sensor_volt=(float)sensorVal/1024*5.0;  
  RS_gas = ((5.0 * R2)/sensor_volt) - R2; 
  ratio = RS_gas/R0;    
  double x = 0.4*ratio;   
  BAC = pow(x,-1.431) * 0.0001;  // BAC in mg/L (or %)
}

// Displays the "Start in..." text of the Countdown screen
void startMsg() {
  tft.setCursor(0,40);
  tft.setTextSize(textSize-1);
  tft.println("Start in...");
}

// Display a number in the middle of the screen
void dispNum(int num){
  tft.setCursor(100,140);
  tft.print(String(num));
}

// Displays the "Retest?" screen and buttons
void dispRetest(){
  tft.fillScreen(BLACK);
  tft.setCursor(40,30);
  tft.println("Retest?");

  tft.fillRect(10,200,110,80,GREEN);
  tft.fillRect(130,200,110,80,RED);

  tft.setCursor(40,240);
  tft.print("Yes");
  tft.setCursor(180,240);
  tft.print("No");
}

// Waits until 2 minutes have passed since the last test, for the sensor to recover to ambient conditions
void recovery(){

  if(millis() - recovTime > 120000){
    return;
  }
  
  // display the "Waiting screen"
  tft.fillScreen(BLACK);
  tft.setCursor(0,60);
  tft.println("Recovering...");
  
  while(millis() - recovTime < 120000){}
  
}

// Waits for a touch input that falls within a pressure range and returns the point information
TSPoint waitTouch() {
  TSPoint p;
  do {
    p = ts.getPoint(); 
    pinMode(XN, OUTPUT);
    pinMode(YP, OUTPUT);
  } while((p.z < MINPRESSURE )|| (p.z > MAXPRESSURE));
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, 320);
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, 240);;
  return p;
}
