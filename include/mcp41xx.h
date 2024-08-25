/*
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*/  

#ifndef _MCP41XX_H_
#define _MCP41XX_H_


#define POTA_CS     11
#define POTB_CS     10
#define POT_MOSI   7
#define POT_SCK    8


#define MCP4151_WRITE_CMD 0x00
#define MCP4151_READ_CMD  0x03

/* Initialize Pins and default state */
void init_mcp4151();

/* Read in resistor data from a given CS Pin */
unsigned short mcp4151_readValue(int cs_pin);

/* Write resistor data to a given CS Pin */
void mcp4151_writeValue(int cs_pin, unsigned char value);


#endif