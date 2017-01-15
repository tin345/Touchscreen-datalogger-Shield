#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
// http://www.pjrc.com/teensy/td_libs_OneWire.html
// The DallasTemperature library can do all this work for you!
// https://www.milesburton.com/Dallas_Temperature_Control_Library

OneWire  ds(A5);                    // on pin 10 (a 4.7K resistor is necessary)
byte addr_list[4][8];               // each address is 4 bytes by 4 possible addresses
byte addr_count_max=0;              // Max found addresses
float minute_temp[30][4];           // Minute of temp data
float minute_time[30];      // Minute of Time data ( millis)


void setup(void) 
{
  byte i;
  byte j;
  byte count;
  byte error;

  Serial.begin(9600);

  // clear arrays
  clear_arrays();
  
  error = OneWireGetAddr();
  if (error)
  {
    Serial.print("error = ");
    Serial.println(error); 
  }  

  Serial.print("max addr =");
  Serial.println(addr_count_max);

  for( i = 0; i < 4; i++) 
  {
    Serial.print("Count = ");
    Serial.println(i, HEX);
    Serial.print("ROM =");
    for( j = 0; j < 8; j++) 
    {
      Serial.write(' ');
      Serial.print(addr_list[i][j], HEX);
    } // end for j
    Serial.println();
  } // end for i


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
      
    minute_time[seconds]=millis()/1000;
//    Serial.println();
//    Serial.print(minute_time[seconds]) ;
  
    //  Read each sensor    
    for( count = 0; count < addr_count_max ; count++) 
    {
     if(addr_list[count][7])
     {
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
        default:          //Serial.println("Device is not a DS18x20 family device.");
          return;
      } 
      present = ds.reset();
      ds.select(addr_list[count]);    
      ds.write(0xBE);    // Read Scratchpad
    
      for ( i = 0; i < 9; i++) // we need 9 bytes
      {           
        data[i] = ds.read();
                            //    Serial.print(data[i], HEX);
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
      
      celsius = (float)raw / 16.0;
      fahrenheit = celsius * 1.8 + 32.0;
    
      minute_temp[seconds][count]=fahrenheit;
      //minute_time[seconds]=millis()/1000;  see above for correct line

//      Serial.print(", ");
//      Serial.print(fahrenheit);

    } // end count loop
    } // end if addr <> 0
  }  // end seconds loop

  //      Serial.println("data dump");
  for( seconds = 1; seconds < seconds_max; seconds++)
  {
    Serial.print(minute_time[seconds]) ;
   Serial.print(", "); 
    for( count = 0; count < addr_count_max; count++) 
    { 
      Serial.print(minute_temp[seconds][count]);
      Serial.print(", ");    
    } // end for count
    Serial.println();
  }  // end for seconds
} // end Loop(void)

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


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void clear_arrays()
{
  byte i;
  byte j;
// clear arrays - for trouble shooting Millis error and to verify memory needs
  for(i=0;i<4;i++)
  {
    for(j=0;j<4;j++)
    {
      addr_list[i][j]=0;
    }
  }  
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

/*
  Serial.println("addr_list");
  for(i=0;i<4;i++)
  {
    for(j=0;j<4;j++)
    {
      Serial.print(addr_list[i][j],HEX);
    }
  Serial.println();
  }  
 
  Serial.println("minute_time =");  
  for(i=0; i<30; i++)
  {
    Serial.print(minute_time[i],HEX);
  }
  Serial.println("minute_temp");

  for(i=0;i<30;i++)
  {
    for(j=0;j<4;j++)
    {
      Serial.print(minute_temp[i][j],HEX);
    }
    Serial.println();
  }
*/
}


