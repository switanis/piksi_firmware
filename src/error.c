/*
 * Copyright (C) 2011 Fergus Noble <fergusnoble@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/f4/usart.h>
#include <libopencm3/stm32/f4/dma.h>

#include "hw/leds.h"
#include "hw/usart.h"

#include "debug.h"
#include "error.h"

/** Last resort, low-level error function.
 * Halts the program while continually sending a non-descript error message in
 * debug message format to the FTDI USART, in a way that should get the message  * through to the Python console even if it's interrupting another transmission.
 */
void screaming_death(void)
{
  __asm__("CPSID if;");           /* Disable ALL interrupts. */
  DMA2_S7CR = 0;                  /* Disable USART TX DMA */
  USART6_CR3 &= ~USART_CR3_DMAT;  /* Disable USART DMA */

  #define ERR_MSG_N = 7           /* Length of error message */

  static char err_msg[ERR_MSG_N+6] = {DEBUG_MAGIC_1, DEBUG_MAGIC_2,
                                      MSG_PRINT, ERR_MSG_N,
                                      'E', 'R', 'R', 'O', 'R', '!', '\n',
                                      /* hard coded CRC for this message */
                                      [ERR_MSG_N+4] = 0x3c,
                                      [ERR_MSG_N+5] = 0x54};
  u8 i = 0;
  while (1) {
    while (!(USART6_SR & USART_SR_TXE));
    USART6_DR = err_msg[i];
    if (++i == (ERR_MSG_N + 6)) {
      i = 0;
      led_toggle(LED_RED);
      for (u32 d = 0; d < 5000000; d++)
        __asm__("nop");
    }
  }
};

/** Last resort, low-level error message function.
 * Halts the program while continually sending a fixed error message in debug
 * message format to the FTDI USART, in a way that should get the message
 * through to the Python console even if it's interrupting another transmission.
 *
 * \param msg A pointer to an array of chars containing the error message.
 */
void speaking_death(char *msg)
{
  __asm__("CPSID if;");           /* Disable all interrupts and faults */
  DMA2_S7CR = 0;                  /* Disable USART TX DMA */
  USART6_CR3 &= ~USART_CR3_DMAT;  /* Disable USART DMA */

  #define ERR_MSG_N 64            /* Maximum length of error message */

  static char err_msg[ERR_MSG_N+6] = {DEBUG_MAGIC_1, DEBUG_MAGIC_2,
                                      MSG_PRINT, ERR_MSG_N,
                                      'E', 'R', 'R', 'O', 'R', ':', ' ',
                                      [11 ... ERR_MSG_N+2] = '!',
                                      [ERR_MSG_N+3] = '\n',
                                      /* Placeholders, CRC calculated below */
                                      [ERR_MSG_N+4] = 0x00,
                                      [ERR_MSG_N+5] = 0x00};

  /* Insert message */
  u8 i=0;
  while (*msg && i < ERR_MSG_N)   /* Don't want to use C library memcpy */
    err_msg[11+(i++)] = *msg++;

  /* Insert CRC */
  u16 crc = crc16_ccitt(&err_msg[2], 2, 0);
  crc = crc16_ccitt(&err_msg[4], ERR_MSG_N, crc);
  err_msg[ERR_MSG_N+4] = (crc >> 8) & 0xFF;
  err_msg[ERR_MSG_N+5] = crc & 0xFF;

  /* Continuously send error message */
  i=0;
  while (1) {
    while (!(USART6_SR & USART_SR_TXE));
    USART6_DR = err_msg[i];
    if (++i == (ERR_MSG_N + 6)) {
      i = 0;
      led_toggle(LED_RED);
      for (u32 d = 0; d < 5000000; d++)
        __asm__("nop");
    }
  }
}
