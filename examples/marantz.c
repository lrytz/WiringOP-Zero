/*
 * http://www.lacavedetryphon.fr/2017/10/piloter-un-amplificateur-hifi-marantz.html
 * Signal is RC5 without carrier frequency.
 *
 * https://www.sbprojects.net/knowledge/ir/rc5.php
 *  - a 1 bit is 889 µs of 0 then 889 µs of 1 (0-1)
 *  - a 0 bit is 1-0
 *  - every RC5 code starts with two 1 bits
 *  - the 3rd bit is a toggle bit. inverted each time a key is released and a new key is pressed.
 *    distinguish repeated press from holding. holding: signal repeats every 114 ms.
 *  - 5 bit IR device address (the "system"), MSB first
 *  - 6 bit command, MSB first
 *
 * RC5 allocations
 *  - system: https://en.wikipedia.org/wiki/RC-5#System_number_allocations
 *  - command: https://en.wikipedia.org/wiki/RC-5#Command_tables
 *
 * From excel sheet
 *  - system: 16 (0x10) for amplifier. though french guy uses 12 (0x0c).
 *  - command: power toggle is 12 (0xc)
 *
 * Example: send power toggle 0x10|0x0c
 *  - bit sequence: 11|1|10000|001100
 *  - power sequence: 0101|01|0110101010|101001011010
 *
 * The second start bit is often used to extend the command range.
 * If the second start bit is 0, 64 is added to the command, i.e.,
 * the second start bit is the inverted MSB of command.
 *
 * RC5x
 * http://lirc.10951.n7.nabble.com/Marantz-RC5-22-bits-Extend-Data-Word-possible-with-lircd-conf-semantic-td9784.html
 *
 * RC5x has
 *  - start bit 1
 *  - inverted MSB of command
 *  - toggle
 *  - 5 bit system
 *  - pause: 2 bit times where nothing is sent (not two 0s)
 *  - 6 remaining bits of command
 *  - 6 bits of data / extension
 *
 * Example: send power on 0x10|0x0c|0x01
 *  - bit sequence: 1|1|0|10000|PAUSE|001100|000001
 *  - power sequence: 01|01|10|0110101010|0000|101001011010|101010101001
 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define	RC_PIN 8

#define POWER_ON_OFF     0x0000100c
#define POWER_ON         0x00100c01
#define POWER_OFF        0x00100c01

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("RC5/RC5x command missing\nUsage: %s <code>\n  <code> = 0xSSCC or 0xSSCCDD\n  where SS = system, CC = command, DD = data\n", argv[0]);
    exit(1);
  }

  char * endptr;
  int command = (int)strtol(argv[1], &endptr, 0);

  if (*endptr != '\0') {
    printf("Failed to parse integer '%s'\n", argv[1]);
    exit(1);
  }
  
  printf ("Int: %i\n", command) ;

  wiringPiSetup () ;
  pinMode (RC_PIN, OUTPUT) ;

  for (;;)
  {
    digitalWrite (RC_PIN, HIGH) ;	// On
    delay (500) ;		// mS
    digitalWrite (RC_PIN, LOW) ;	// Off
    delay (500) ;
  }
  return 0 ;
}
