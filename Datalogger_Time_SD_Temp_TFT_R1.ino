/*
  Datalogger

This code is made from numerous examples.
Several examples were from Adafruit.

Datalogger performs the following:
+ Inputs from 1 to 4, onewire temperature sensors DS182x type
+ Reads a Real time clock DS3231, over I2c, for a time stamp
+ Writes time stamp and data to Micro SD card over SPI and to USB serial port
+ Writes First two sensor temperatuers to LCD display
+ Graphs the First temperature sensor on LCD display

Hardware:
- no name oneWire temperature sensors DS182x attached to A0
- no name I2C Real time clock DS3231 on A4 and A5
- Adafruit 2.8" TFT LCD Shied V2, with resistive touch screen (not used yet)
- no name screw shield (the LCD Shield covers all the pins)
- no name AA battery back 


 The circuit:
 ** analog sensors on analog ins 1, 2, and 3
 * TFT display and SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** SD_CARD Chip Select - pin 4 
 ** TFT DC - Pin 9
 ** TFT Chip Select - Pin 10
 
 */

#include <SPI.h>        // SPI communication lib
#include <Wire.h>       // I2C, TWI communication lib
#include <OneWire.h>    // One Wire lib
#include <SD.h>         // SD card librarly talks over SPI.h
#include "RTClib.h"     // Real time clock DS3231 via I2C and Wire lib
#include "Adafruit_GFX.h"     // Ada Graphics lib
#include "Adafruit_ILI9341.h" // Ada TFT lib

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Define objects for reading time
RTC_DS3231 rtc;  // Create rtc Object of class RTC_DS3231
DateTime now;    // Create now Object of type DateTime

//Define DS18xxx temp sensor via one wire
OneWire  ds(A0);  // on pin 10 (a 4.7K resistor is necessary)

const uint8_t rotation=1; //Screen rotation 90deg landscape (could be a variable)
const int SD_CS = 4;   // SD card chip select 
byte addr_list[4][8];  // 4 possible sensor addresses 8 bytes each
byte addr_count_max=0; // Max found addresses
String dataString = "";
String tempString = "";
String timeString = "";
float temp1=0;
float temp2=0;
int xPos = 30; // position of the line on screen
int minute_now=0; //present minute
int minute_save=-1; // last save
int data_sample_rate = 1; // SD card and plot sample rate in minutes

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void setup() 
{
  byte error;
  
  
  Serial.begin(9600);  // Open serial communications and wait for port to open:
  tft.begin();
  tft.setRotation(rotation);
  tft.fillScreen(ILI9341_BLACK);
  setup_SD();
  setup_RTC();
  error = OneWireGetAddr();
  if (error)
  {
    Serial.print("Temp error = ");
    Serial.println(error); 
  }  
  Serial.print("Temp max addr =");
  Serial.println(addr_count_max);
  list_temp_addr();

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void loop() 
{
  temp_start(); // start temp conversion early it takes 750ms
  timeString = "";
  tempString="";
  dataString = "";

  timenow();      // create timeString
  Serial.print(timeString);
  delay(750);    // maybe 750ms is enough, maybe not
  temp_read();
  Serial.print(tempString);
  Serial.println();
  testText();
  if (minute_save+data_sample_rate > 60){minute_save-=60;}  
  // Write SD and Graph based upon sample rate
  Serial.print(minute_now);
  Serial.print(" = ");
  Serial.println(minute_save);
  if (minute_now == minute_save+data_sample_rate)
  {
  Serial.print("SD Write");
  Serial.println();
  minute_save=minute_now;
  graph_data();  
     //  analog_read();  // create dataString  NO ANALOG DATA
  SD_write();
  }
  delay(100);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void testText(void) {
      //  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);    tft.setTextSize(3);
  tft.print(F("Temp1= "));
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);    tft.setTextSize(3);
  tft.print(String(temp1));
  tft.println(F("F"));
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);    tft.setTextSize(3);
  tft.print(F("Temp2= "));
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);    tft.setTextSize(3);
  tft.print(String(temp2));
  tft.println(F("F"));
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void graph_data()
{
  // read the sensor and map it to the screen height
  //int sensor = analogRead(A3); //sensor 'was' analog data
  const int MAX_CHART = 90;
  const int MIN_CHART = 70;
  const int CHT_TXT_SIZE =1;
  
  int sensor1 = int(temp1*10);
  int sensor2 = int(temp2*10);
  const int textHeight = 55;
  int drawHeight1 = map(sensor1, 700, 900, 1, tft.height()-textHeight);
  int drawHeight2 = map(sensor2, 700, 900, 1, tft.height()-textHeight);

  
  // Chart each sensors, 
  if(drawHeight1 > drawHeight2) // smallest in front tallest in back
  {
    tft.drawFastVLine(xPos, tft.height() - drawHeight1, tft.height(),ILI9341_WHITE);
    tft.drawFastVLine(xPos, tft.height() - drawHeight2, tft.height(),ILI9341_GREEN);
  } else
  {
    tft.drawFastVLine(xPos, tft.height() - drawHeight2, tft.height(),ILI9341_GREEN); 
    tft.drawFastVLine(xPos, tft.height() - drawHeight1, tft.height(),ILI9341_WHITE);
  }
  
  // if the graph has reached the screen edge
  // erase the graph area and start again
  if (xPos >= 320) 
  {
    xPos = 30;
    tft.fillRect(0, textHeight, tft.width(), tft.height()-textHeight, ILI9341_BLACK);
  } else {
    xPos++;// increment the horizontal position:
  }
  
  // Print chart lines and lables
  tft.drawFastHLine(30, textHeight, tft.width(),ILI9341_RED); 
  tft.setCursor(0, textHeight-5);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(CHT_TXT_SIZE);
  tft.println(F("90F"));

  tft.drawFastHLine(30, textHeight+(tft.height()-textHeight)*1/4, tft.width(),ILI9341_RED);
  tft.setCursor(0, textHeight-5+(tft.height()-textHeight)*1/4);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(CHT_TXT_SIZE);
  tft.println(F("85F"));

  tft.drawFastHLine(30, textHeight+(tft.height()-textHeight)*2/4, tft.width(),ILI9341_RED);
  tft.setCursor(0, textHeight-5+(tft.height()-textHeight)*2/4);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(CHT_TXT_SIZE);
  tft.println(F("80F"));

  tft.drawFastHLine(30, textHeight+(tft.height()-textHeight)*3/4, tft.width(),ILI9341_RED);
  tft.setCursor(0, textHeight-5+(tft.height()-textHeight)*3/4);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(CHT_TXT_SIZE);
  tft.println(F("75F"));

  tft.drawFastHLine(30, tft.height()-1, tft.width(),ILI9341_RED);
  tft.setCursor(0, tft.height()-7);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(CHT_TXT_SIZE);
  tft.println(F("70F"));
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void analog_read()
{
  // make a string for assembling the data to log:
  //String dataString = "";

  // read three sensors and append to the string:
  for (int analogPin = 0; analogPin < 3; analogPin++) {
    int sensor = analogRead(analogPin);
    dataString += String(sensor);
    if (analogPin < 2) {
      dataString += ",";
    }
  }  
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void SD_write()
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.print(timeString);
    dataFile.println(tempString);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void setup_SD(void)
{
   // see if the card is present and can be initialized:
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized."); 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void setup_RTC(void)
{
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  } 
  now = rtc.now();
  minute_now = now.minute();
  minute_save=minute_now-data_sample_rate;      
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void timenow(void) // create timeString as date,time,
{
  now = rtc.now();
  minute_now = now.minute();    
  timeString = String(now.year())+"/"+String(now.month())+"/"+String(now.day())+" ";
  timeString += String(now.hour())+":"+String(now.minute())+":"+String(now.second())+",";
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void temp_read(void)  //  Read each sensor
{
  byte count;
  byte i;
//  byte present = 0;
  byte type_s;
  byte data[12];
//  float celsius, fahrenheit;
  float  fahrenheit;
  
  for( count = 0; count < addr_count_max ; count++) 
  {
   if(addr_list[count][7])
   {
    switch (addr_list[count][0]) // check DS chip type
    {
      case 0x10:        //Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:        //Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:        //Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:          //Serial.println("Device is not a DS18x20 family device.");
        return;
    } 
//    present = ds.reset();
    ds.reset();
    ds.select(addr_list[count]);    
    ds.write(0xBE);    // Read Scratchpad
  
    for ( i = 0; i < 9; i++) // we need 9 bytes
    {           
      data[i] = ds.read();
                          //    Serial.print(data[i], HEX);  // Show raw data.
                          //    Serial.print(" ");
    } // end for i
  
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) 
    {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else 
    {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    
//    celsius = (float)raw / 16.0;
    fahrenheit = (float)raw / 16.0 * 1.8 + 32.0;
    if (count == 0)temp1=fahrenheit;
    if (count == 1)temp2=fahrenheit;
    tempString += String(fahrenheit);
    tempString += ",";
    } // end if addr <> 0
  } // end count loop
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void temp_start(void)
{
    ds.reset();         // Reset  DS18B20
    ds.write(0xCC);     // Skip_ROM, with parasite power on at the end
    ds.write(0x44);     // start conversion (all devices), with parasite power on at the end 
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
byte OneWireGetAddr()
{
  
  
  byte i;
  byte j;
  byte type_s;
   
  addr_count_max=0;
  ds.reset();                     // reset 1-Wire 
  ds.reset_search();              // reset 1-Wire Address search
    
  for( i = 0; i < 4; i++) //up to 4 oneWire devices
  {
   Serial.println();
   Serial.print("Count = ");
   Serial.println(i, HEX);
   
  if ( !ds.search(addr_list[i])) 
  {
    Serial.println("No more addresses.");
    for( j = 0; j < 8; j++) // prevent from returning last Addr twice, so zero out addr on fail
    {                       // this should not be needed ... but it is...
      addr_list[i][j]= 0;
    }  
//    delay(250);  // don't wait to fail
    return(1);  // finish checking addresses
  }

  if (OneWire::crc8(addr_list[i], 7) != addr_list[i][7]) 
  {
      Serial.println("CRC is not valid!");
      break;
  }
 
  // the first ROM byte indicates which chip

    // set resolution to 12 bit  
    ds.reset();                    // rest 1-Wire
    ds.select(addr_list[i]);    // select DS18B20
    ds.write(0x4E);                 // write on scratchPad
    ds.write(0x00);                 // User byte 0 - Unused
    ds.write(0x00);                 // User byte 1 - Unused
    ds.write(0x7F);                 // set up en 12 bits (0x7F)
    ds.reset();                     // reset 1-Wire 
  
  
  switch (addr_list[i][0]) 
  {
    case 0x10:
      Serial.print("Chip = DS18S20");  // or old DS1820
      break;
    case 0x28:
      Serial.print("Chip = DS18B20");
      break;
    case 0x22:
      Serial.print("Chip = DS1822");
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return(1);
  }
  addr_count_max++;
  Serial.print(",  ROM =");  // print full address
  for( j = 0; j < 8; j++) 
  {
    Serial.write(' ');
    Serial.print(addr_list[i][j], HEX);
  }
  Serial.println();
  } // end for i
return(0); 
}// end OneWireGetAddr()

void list_temp_addr()
{
  byte i;
  byte j;
  for( i = 0; i < 4; i++) 
  {
    Serial.print("Temp Count = ");
    Serial.println(i, HEX);
    Serial.print("ROM =");
    for( j = 0; j < 8; j++) 
    {
      Serial.write(' ');
      Serial.print(addr_list[i][j], HEX);
    } // end for j
    Serial.println();
  } // end for i
} // end list_temp_addr



