///////////////
// Sketch: Fossasat-1Screen.ino
// Author: Asbjorn Riedel - asbjorn.riedel@gmail.com
//
// Versions:
//    - 0.0.1  First beta - April 2019
// Comments: 
//    Several libraries have been used i as templates and code that works for this particular 
//    SPFD508Shield (from Aliexpress) have been used  (TFT 2.4 inch)
//    https://github.com/JoaoLopesF/SPFD5408
//    https://gist.github.com/calogerus/79ef0c4cf04d9ea33dae9fd77a3a7316
//    
//    Other shileds will surely work differently.
//
///////////////

//  Fossasat-1 Groundstation
//
//  * Online    * Transceiver
//  * Deployed  * Tuning
//  * PongRec   * Transmitting
//
//  Battery:     3.00V    []
//  Battery:     1.00A
//  Solar:       2.00V
//  Resets:      4
//  RSSI: 0.0 %  SNR: 0.0 %


#include <SPFD5408_Adafruit_GFX.h>    // Core graphics library
#include <SPFD5408_Adafruit_TFTLCD.h> // Hardware-specific library
#include <SPFD5408_TouchScreen.h>     // Touch library
#include <Wire.h>                     // for communication with host computer

// Calibrates value
#define SENSIBILITY 200

//These are the pins for the shield!
#define YP A1 
#define XM A2 
#define YM 7  
#define XP 6 

#define Y1 A3  // need two analog inputs
#define X1 A2  // 
#define Y2 9   // 
#define X2 8   //

int16_t P_COL=0; // LCD cursor pointer
int16_t P_ROW=0;
int16_t T_COL=0; // TOUCHSCREEN(TS) detected value
int16_t T_ROW=0;

// TS calibration
uint16_t ROW_F=120; // TS first row 110
uint16_t ROW_L=895; // TS last row 920
uint16_t COL_F=95; // TS first column 110
uint16_t COL_L=860; // TS last column 930

// Init TouchScreen:

TouchScreen ts = TouchScreen(XP, YP, XM, YM, SENSIBILITY);

// LCD Pin

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4 // Optional : otherwise connect to Arduino's reset pin

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Init LCD

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

uint16_t x=0;
uint16_t y=0;

// first status
int newDataAvailable = 1;
int Online = RED;
int Transceiver = RED;
int Deployed = RED;
int Tuning = RED;
int PongRec = RED;
int Transmitting = RED;
uint16_t waitPong = 0;
String RSSI = "0.0";
String SNR = "0.0";
String ID;
String BC = "0.0";
String B  = "0.0";
String TS = "0.0";
String RC = "0";
String DS = "0";

void displayStatus( int Online, int Transceiver, int Deployed, int Tuning, int PongRec, int Transmitting, String RSSI, String SNR, String BC, String B, String TS, String RC) {
  Serial.println("displayStatus");
  
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(3);
  
  Serial.println(F("TFT LCD is ready"));
  Serial.print("TFT size is ");
  Serial.print(tft.width()); 
  Serial.print("x"); 
  Serial.println(tft.height());

  tft.fillScreen(WHITE);
  tft.setTextColor(BLACK);
  tft.setTextSize (2);
  
  tft.setCursor( 10, 0); tft.println("Fossasat-1  GroundStation");
  statusDisplay( 10,40,Online,"Online");
  statusDisplay(150,40,Transceiver,"Transceiver");
  statusDisplay( 10,60,Deployed,"Deployed");
  statusDisplay(150,60,Tuning,"Tuning");
  statusDisplay( 10,80,PongRec,"PongRec");
  statusDisplay(150,80,Transmitting,"Transmitting");

  Serial.println("RSSI="+RSSI+" SNR="+SNR+" BC="+BC+" B="+B+" TS="+TS+" RC="+RC);

  tft.setCursor(10,125); tft.println("Battery:"); tft.fillRect(150,125,150,20,WHITE); tft.setCursor(150,125); tft.println(BC+" V");
  tft.setCursor(10,145); tft.println("Battery:"); tft.fillRect(110,145,150,20,WHITE); tft.setCursor(150,145); tft.println(B+" A");
  tft.setCursor(10,165); tft.println("Solar:");   tft.fillRect(110,165,150,20,WHITE); tft.setCursor(150,165); tft.println(TS+" V");
  tft.setCursor(10,185); tft.println("Resets:");  tft.fillRect(110,185,150,20,WHITE); tft.setCursor(150,185); tft.println(RC);
  
  tft.fillRect(10,205,300,20,WHITE); tft.setCursor(10,205); tft.println("RSSI: "+RSSI);
  tft.setCursor(150,205); tft.println("SNR: "+SNR+" %");

  createFilledRect(240, 140, 70, 30, YELLOW, RED);
  setButtonText(252, 148, "PING", BLACK);
}

void requestEvent() {
  Wire.write("requestEvent "); // respond with message of 6 bytes
}

// Format of data received on wire
// R:1234;S:9.73;9;BC:1.234;B:2.345;TS:3.456;RC:456;DS:1;

void receiveEvent(int bytes) {
  char c;
  String s = "";
  while (Wire.available()) {
    c = Wire.read();
    s += c;
  }
  Serial.println(s);
  
  int indexOfS1 = s.indexOf(':');
  String field = s.substring(0, indexOfS1);
  String data = s.substring(indexOfS1+1, s.length());

  Serial.print("field="); Serial.print(field); Serial.print(" data="); Serial.println(data);

  // extract data from message
  newDataAvailable = 1; // We are optimists
  if ( field == "No") { Serial.println("No data"); return; }

  // status from satellite
  if ( field == "ID") { ID = data; return; }
  if ( field == "BC") { BC = data; return; }
  if ( field == "B")  { B  = data; return; }
  if ( field == "TS") { TS = data; return; }
  if ( field == "RC") { RC = data; return; }
  if ( field == "DS") { DS = data; return; }

  // status from groundstation
  if ( field == "RS") { RSSI = data; return; }
  if ( field == "SN") { SNR = data; return; }
  if ( field == "ONLINE") { Online = GREEN; return; }
  if ( field == "OFFLINE") { Online = RED; return; }
  if ( field == "TUNINGON") { Tuning = GREEN; return; }
  if ( field == "TUNINGOFF") { Tuning = RED; return; }
  Serial.println(s);
  newDataAvailable = 0; // Oh, no data received
  return;
}
//-- Setup

void setup(void) {

  // Serial port for debug, not works if shield is plugged in arduino
  
  Serial.begin(115200);

  Wire.begin(4);                // join i2c bus with address #4
  Wire.onRequest(requestEvent); // register event 
  Wire.onReceive(receiveEvent);
  
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(3);

  tft.fillScreen(WHITE);
  tft.setTextColor(BLACK);
  tft.setTextSize (2);
}

// -- Loop

int16_t maxrow=0;
int16_t maxcol=0;
int16_t minrow= 32000;
int16_t mincol= 32000;
uint32_t touch;
int16_t row;
int16_t col;

void loop(){
  if ( newDataAvailable == 1 ){
    if (ID == "7") { Transceiver = GREEN; }
    if (ID == "8") { Transceiver = RED; }
    if (DS == 0) { Deployed = RED; } else { Deployed = GREEN; }
    Tuning = GREEN;
    if (ID == "6") { PongRec = GREEN; } else { PongRec = RED; }
    if (ID == "7") { Transmitting = GREEN; } else { Transmitting = RED; }
  
    displayStatus( Online, Transceiver, Deployed, Tuning, PongRec, Transmitting, RSSI, SNR, BC, B, TS, RC);
    newDataAvailable = 0;
  }

  touch = ReadTouch() ; // returns integer with row and col
  if(touch > 0) {
    row = touch / 1000;
    col = touch % 1000;

    // clicked on button and repeat clicks will be igtnored 10 seconds
    if (( col > 250) && (col < 320) && (row > 150) && (row < 180) && (waitPong == 0)) {
      Serial.println("send ping");
      createFilledRect(240, 140, 70, 30, RED, BLACK);
      setButtonText(252, 148, "PING", YELLOW);
      waitPong = 10; // wait 10 seconds
      Wire.write("SEND PING");
    }
    else { Serial.print(row); Serial.print(" "); Serial.print(col); Serial.print(" "); Serial.print(waitPong);
    Serial.println(" ( col > 250) && (col < 320) && (row > 150) && (row < 180) && (waitPong == 0)"); }
    touch = 0;
  }
  
  if (( waitPong < 2 ) && (waitPong != 0)) {
    Serial.println("ping sent");
    createFilledRect(240, 140, 70, 30, YELLOW, RED);
    setButtonText(252, 148, "PING", BLACK);
  }
  if ( waitPong > 0 ) { waitPong--; }
  delay(50);
}

void setButtonText(int16_t x, int16_t y, char* buttonText, char* buttonColor){
  tft.setCursor(x,y);
  tft.println(buttonText);
}

void createFilledRect(int16_t x, int16_t y, int16_t height, int16_t width, uint16_t color1, uint16_t color2) {
  tft.fillRect(x, y, height, width, color1);
  tft.drawRect(x, y, height, width, color2);
}

void statusDisplay(int16_t x, int16_t y, int color, String text){
  Serial.print(text);
  if ( color == RED ){ Serial.println("=RED"); } else { Serial.println("=GREEN"); }
  tft.fillCircle(x,y,4,color);
  tft.setCursor(x+10,y-5);
  tft.println(text);  
}

void fillDot3(int16_t x, int16_t y, int color){
  if ((x == 0) || (y == 0)) { return; }
  tft.fillCircle(x,y,4,color); 
}


void onlineStatus(boolean state){
  if (state == true){
    fillDot3(10,30,GREEN);
  }
  else{
    fillDot3(10,30,RED);
  }
}


// This code is adapted on basis of ILI9341_7 (see links above)
void BD_as_output(void) {
  // Pins 7-2 as output, no change for pins 1,0 (RX TX)
  DDRD = DDRD | B11111100; 
  // Pins 8-9 as output
  DDRB = DDRB | B00000011; 
}

uint32_t ReadTouch(void) {
// return row and col in an integer
// return 0 if no touch within 10000 thingies
  //Y1 A3  
  //X1 A2   
  //Y2 9   
  //X2 8   
  int16_t row, col;
  int8_t touch, valid;
  int16_t loop_cnt = 20000;
  valid=0;
  
  while (loop_cnt) {
    pinMode(Y1, INPUT); 
    pinMode(Y2, INPUT_PULLUP); 
    
    pinMode(X1, OUTPUT);
    pinMode(X2, OUTPUT);
    digitalWrite(X1, LOW);
    digitalWrite(X2, LOW);
    
    touch = !digitalRead(Y1); // 0 - touched
    if (touch) {
      //delay(5);
      digitalWrite(X1, HIGH);   // X variant A
      //digitalWrite(X2, HIGH);   // X variant B
      delay(1);
      row = analogRead(Y1);
      delay(4); 
      if (abs(analogRead(Y1)-row)>3) {
        BD_as_output();
        DDRC = DDRC | B00011111;
        return 0;
      }
      delay(3);
      if (abs(analogRead(Y1)-row)>3) { 
        BD_as_output();
        DDRC = DDRC | B00011111;
        return 0;
      }
      
      pinMode(X1, INPUT); 
      pinMode(X2, INPUT_PULLUP); 
      
      pinMode(Y1, OUTPUT);
      pinMode(Y2, OUTPUT);
      digitalWrite(Y1, LOW);  // Y variant B
      digitalWrite(Y2, HIGH);  // Y variant B
      delay(1);
      col = analogRead(X1);
      delay(4);  
      if (abs(analogRead(X1)-col)>3) { 
        BD_as_output();
        DDRC = DDRC | B00011111;
        return 0;
      }
      delay(3);
      if (abs(analogRead(X1)-col)>3) { 
        BD_as_output();
        DDRC = DDRC | B00011111;
        return 0;
      }
      
      digitalWrite(Y2, LOW);  // Y variant B
      touch = !digitalRead(X1); // 0 - dotyk
      if (touch) {
        int16_t rows=ROW_L-ROW_F;
        int16_t cols=COL_L-COL_F;
        float row1=float(row-ROW_F)/rows*240;
        float col1=float(col-COL_F)/cols*320;
        T_ROW=int(row1);
        T_COL=int(col1);
        valid=1;
      }
      loop_cnt = 1; //get out of loop
    }
    loop_cnt--;
  }
  // Re-Set A2 A3 8 9 for ILI9341
  BD_as_output();
  DDRC = DDRC | B00011111; // A0-A4 as outputs  

  if (!valid) { return 0; }

  // Has to be subtracted from 240, for some reason
  row=(240-T_ROW);
  col=T_COL;
  if ( row < 1 ) { row = 1; }
  if ( col < 1 ) { col = 1; }
  if ( row > 240 ) { row = 240; }
  if ( row > 320 ) { row = 320; }
  
  uint32_t retval = ( uint32_t(row) * 1000 ) + col;  
  return (( uint32_t(row) * 1000 ) + col);
}
