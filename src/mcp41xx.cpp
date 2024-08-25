/*
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*/  


#include <Arduino.h>
#include <mcp41xx.h>


void init_mcp4151(){

  digitalWrite(POT_SCK,1);
  pinMode(POT_SCK,OUTPUT);
  digitalWrite(POTB_CS,1);
  pinMode(POTA_CS,OUTPUT);
  digitalWrite(POTB_CS,1);
  pinMode(POTB_CS,OUTPUT);
  pinMode(POT_MOSI,OUTPUT);

}




unsigned short mcp4151_readData(){
  // Chip takes 10 bit data, but has only 8 bit resistor-values
  unsigned short shift_in=0;

  pinMode(POT_MOSI,INPUT);
  delayMicroseconds(1); 

  for(int i=0;i<10;i++){   // 10 bit
    digitalWrite(POT_SCK,0);        
    delayMicroseconds(1);   // According to the datasheet, we should delay 45nS , so this is more than enough :)
    digitalWrite(POT_SCK,1);    
    if (digitalRead(POT_MOSI)){  // MSB first
      shift_in|=0x01;    
    }
    shift_in<<=1;
    delayMicroseconds(1);   
  }

  pinMode(POT_MOSI,OUTPUT);


#ifdef _DEBUG
  Serial.print (" value :");
  Serial.println (shift_in);
#endif

  return (shift_in);

}

int mcp4151_writeData(unsigned char value){
  // Chip takes 10 bit data, but has only 8 bit resistor-values
  // Highest bit is ignored , but value starts at bit 9
  unsigned short bitmask=0x200;
  unsigned short shift_out=value;

#ifdef _DEBUG
  Serial.print (" value :");
  Serial.println (shift_out);
#endif

  for(int i=0;i<10;i++){   // 10 bit
    digitalWrite(POT_SCK,0);    
    if (shift_out&(bitmask>>i)){  // MSB first
      digitalWrite(POT_MOSI,1);    
    }else{
      digitalWrite(POT_MOSI,0);    
    }
    delayMicroseconds(1);   // According to the datasheet, we should delay 45nS , so this is more than enough :)
    digitalWrite(POT_SCK,1);    
    delayMicroseconds(1);   
  }



}


int mcp4151_writeCmd(unsigned char command){

  unsigned char bitmask=0x20;

  for(int i=0;i<6;i++){   // Command is only 6 bit long
    digitalWrite(POT_SCK,0);    
    if (command&(bitmask>>i)){  // MSB first
      digitalWrite(POT_MOSI,1);    
    }else{
      digitalWrite(POT_MOSI,0);    
    }
    delayMicroseconds(1);   // According to the datasheet, we should delay 45nS , so this is more than enough :)
    digitalWrite(POT_SCK,1);    
    delayMicroseconds(1);   
  }


}


unsigned short mcp4151_readValue(int cs_pin){

#ifdef _DEBUG
  Serial.print ("MCP4151 read from :");
  Serial.print (cs_pin);
#endif
  digitalWrite(POT_MOSI,1);   // Force SPI-Mode 1,1
  digitalWrite(cs_pin,0);
  mcp4151_writeCmd(MCP4151_READ_CMD);
  unsigned short value =mcp4151_readData();
  delayMicroseconds(1);
  digitalWrite(cs_pin,1);

  return(value);

}


void mcp4151_writeValue(int cs_pin, unsigned char value){

#ifdef _DEBUG
  Serial.print ("MCP4151 write to");
  Serial.print (cs_pin);
#endif
  digitalWrite(POT_MOSI,1);   // Force SPI-Mode 1,1
  digitalWrite(cs_pin,0);
  mcp4151_writeCmd(MCP4151_WRITE_CMD);
  mcp4151_writeData(value);
  delayMicroseconds(1);
  digitalWrite(cs_pin,1);

}
