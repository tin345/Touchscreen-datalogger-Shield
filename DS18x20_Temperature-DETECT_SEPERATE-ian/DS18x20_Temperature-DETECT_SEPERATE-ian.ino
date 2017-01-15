#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
// http://www.pjrc.com/teensy/td_libs_OneWire.html
// The DallasTemperature library can do all this work for you!
// https://www.milesburton.com/Dallas_Temperature_Control_Library

OneWire  ds(A5);  // on pin 10 (a 4.7K resistor is necessary)
byte addr_list[4][4];               // each address is 4 bytes by 4 possible addresses
byte addr_count_max=0;              // Max found addresses
float minute_temp[30][4];           // Minute of temp data
unsigned long minute_time[30];      // Minute of Time data ( millis)


void setup(void) 
{
  byte i;
  byte j;
  byte count;
  byte error;
  ds.reset_search();
  Serial.begin(9600);

// clear arrays
  for(i=0; i<30; i++)
  {
    minute_time[i]=0;
  }
  for(i=0;i<30;i++)
  {
    for(j=0;j<4;j++)
    {
      minute_temp[i][j]=0;
    }
  }

  
//  for( i = 0; i < 8; i++) 
//  {
    error = OneWireGetAddr();
    if (error)
    {
      Serial.print("error = ");
      Serial.println(error); 
//      i=254;
    }  
//  }

    Serial.print("max addr =");
    Serial.println(addr_count_max);

  for( count = 0; count < addr_count_max; count++) 
  {
    Serial.print(",  ROM =");
    for( i = 0; i < 8; i++) 
    {
      Serial.write(' ');
      Serial.print(addr_list[count][i], HEX);
    }
    Serial.println();
  }

} // end setup(void)

void loop(void) 
{
  byte count;
  byte i;
  byte seconds;
  byte seconds_max=10;
  byte present = 0;
  byte type_s;
  byte data[12];
  float celsius, fahrenheit;
 

  for( seconds = 0; seconds < seconds_max; seconds++)
  {
      ds.reset();            // Reset  DS18B20
      ds.write(0xCC);        // Skip_ROM, with parasite power on at the end
      ds.write(0x44);       // start conversion (all devices), with parasite power on at the end
      delay(1000);    // maybe 750ms is enough, maybe not

  Serial.println();
  Serial.print(minute_time[seconds]) ;

  
    //  Read each sensor    
    for( count = 0; count < addr_count_max ; count++) 
    {
     if(addr_list[count][7])
     {

      //Serial.print("Seconds =");
      //Serial.println(seconds);
      //Serial.print("addr =");
      //Serial.println(addr_list[count][7] HEX);
      // the first ROM byte indicates which chip
      
      switch (addr_list[count][0]) 
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
        default:        //Serial.println("Device is not a DS18x20 family device.");
          return;
      } 
    
      //ds.reset();
      //ds.select(addr_list[count]);
      // used skip rom and command above   ds.write(0x44, 1);        // start conversion, with parasite power on at the end 
  
      // we might do a ds.depower() here, but the reset will take care of it.
      present = ds.reset();
      ds.select(addr_list[count]);    
      ds.write(0xBE);         // Read Scratchpad
    
                            //  Serial.print("  Data = ");
                            //  Serial.print(present, HEX);
                            //  Serial.print(" ");
      for ( i = 0; i < 9; i++) 
      {           // we need 9 bytes
        data[i] = ds.read();
                            //    Serial.print(data[i], HEX);
                            //    Serial.print(" ");
      }
    
    
      // Convert the data to actual temperature
      // because the result is a 16 bit signed integer, it should
      // be stored to an "int16_t" type, which is always 16 bits
      // even when compiled on a 32 bit processor.
      int16_t raw = (data[1] << 8) | data[0];
      if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
      }
      celsius = (float)raw / 16.0;
      fahrenheit = celsius * 1.8 + 32.0;
    
      //Serial.print(addr_list[count][7], HEX); // really last byte of addr is checksum??
      //Serial.print(", ");
      //  Serial.print(celsius);
      //  Serial.print(" Celsius, ");
      //Serial.print(fahrenheit);
      //Serial.print(" F, ");
      //Serial.println();
      minute_temp[seconds][count]=fahrenheit;
      minute_time[seconds]=millis();

Serial.print(", ");
Serial.print(fahrenheit);

      
    } // end count loop
    } // end if addr <> 0
  }  // end seconds loop

//      Serial.println("data dump");
  for( seconds = 1; seconds < seconds_max; seconds++)
  {
//    Serial.print(minute_time[seconds]/1000) ;
//   Serial.print(", "); 
//    for( count = 0; count < addr_count_max; count++) 
//    { 
//      Serial.print(minute_temp[seconds][count]);
//      Serial.print(", ");    
//    }
 //   Serial.println();
  }
} // end Loop(void)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
byte OneWireGetAddr()
{
  
  byte i;
  byte j;
  byte type_s;
  
  addr_count_max=0;
  
  for( j = 0; j < 7; j++) //up to 8 oneWire devices
  {
   
  

   Serial.println();
   Serial.print("Count = ");
   Serial.println(j, HEX);
   
  if ( !ds.search(addr_list[j])) 
  {
    Serial.println("No more addresses.");
    for( i = 0; i < 8; i++) // prevent from returning last Addr twice, so zero out addr on fail
    {  
      addr_list[j][i]= 0;
    }
    //Serial.println();
    delay(250);  
    return(1);
  }

  if (OneWire::crc8(addr_list[j], 7) != addr_list[j][7]) 
  {
      Serial.println("CRC is not valid!");
      break;
  }
 
  // the first ROM byte indicates which chip


    // set resolution to 12 bit  
    ds.reset();                    // rest 1-Wire
    ds.select(addr_list[j]);    // select DS18B20
    ds.write(0x4E);                 // write on scratchPad
    ds.write(0x00);                 // User byte 0 - Unused
    ds.write(0x00);                 // User byte 1 - Unused
    ds.write(0x7F);                 // set up en 12 bits (0x7F)
    ds.reset();                     // reset 1-Wire 
  
  
  switch (addr_list[j][0]) 
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
  for( i = 0; i < 8; i++) 
  {
    Serial.write(' ');
    Serial.print(addr_list[j][i], HEX);
  }
  Serial.println();
  } // end for j
return(0); 
}// end OneWireGetAddr()




