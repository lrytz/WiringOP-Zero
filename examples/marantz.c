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
 *  - bit sequence (14 bits): 11|1|10000|001100
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
 *  - bit sequence (20 bits, 2 pause): 1|1|0|10000|PAUSE|001100|000001
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
#include <stdbool.h>
#include <wiringPi.h>

#define	RC_PIN 8

#define PULSE 889
// #define PULSE 88900 // LED DEBUG SPEED

#define DBG 0

int inv(int bit) {
  if (bit == 1) return 0;
  return 1;
}

void send(int signal, bool extended) {
  int x = 13;
  if (extended)
    x = 21;
  while (x >= 0) {
    if (extended && x == 13) {
      if (DBG) printf("PP");
      digitalWrite(RC_PIN, LOW);
      delayMicroseconds(PULSE * 4);
      x -= 2;
    } else {
      int bit = (signal >> x) & 1;
      if (DBG) printf("%i", bit);
      digitalWrite(RC_PIN, inv(bit));
      delayMicroseconds(PULSE);
      digitalWrite(RC_PIN, bit);
      delayMicroseconds(PULSE);
      x -= 1;
    }
  }
  digitalWrite(RC_PIN, LOW);
  if (DBG) printf("\n");
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("RC5/RC5x command missing\nUsage: %s <code>\n  <code> = 0xSSCC or 0xSSCCDD\n  where SS = system, CC = command, DD = data\n", argv[0]);
    exit(1);
  }

  char * endptr;
  int code = (int)strtol(argv[1], &endptr, 0);

  if (*endptr != '\0') {
    printf("Failed to parse integer '%s'\n", argv[1]);
    exit(1);
  }
  
  if (DBG) printf("Int: %i\n", code);

  bool extended = (code & 0xff0000) != 0;

  wiringPiSetup() ;
  pinMode(RC_PIN, OUTPUT);

  int signal = 0;

  if (!extended) {
    int system = (code & 0xff00) >> 8;
    int command = code & 0xff;
    signal |= 1 << 13;
    if (command < 64)
      signal |= 1 << 12;
    signal |= system << 6;
    signal |= command & 0x3f;
    if (DBG) printf("normal signal %i\n", signal);
    send(signal, extended);
  } else {
    int system = (code & 0xff0000) >> 16;
    int command = (code & 0xff00) >> 8;
    int data = code & 0xff;
    signal |= 1 << 21;
    if (command < 64)
      signal |= 1 << 20;
    signal |= system << 14;
    signal |= (command & 0x3f) << 6;
    signal |= data;
    if (DBG) printf("extended signal %i\n", signal);
    send(signal, extended);
  }

  return 0 ;
}
