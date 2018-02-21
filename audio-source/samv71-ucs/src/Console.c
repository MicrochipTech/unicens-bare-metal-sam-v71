/*------------------------------------------------------------------------------------------------*/
/* Console Print Component                                                                        */
/* Copyright 2017, Microchip Technology Inc. and its subsidiaries.                                */
/*                                                                                                */
/* Redistribution and use in source and binary forms, with or without                             */
/* modification, are permitted provided that the following conditions are met:                    */
/*                                                                                                */
/* 1. Redistributions of source code must retain the above copyright notice, this                 */
/*    list of conditions and the following disclaimer.                                            */
/*                                                                                                */
/* 2. Redistributions in binary form must reproduce the above copyright notice,                   */
/*    this list of conditions and the following disclaimer in the documentation                   */
/*    and/or other materials provided with the distribution.                                      */
/*                                                                                                */
/* 3. Neither the name of the copyright holder nor the names of its                               */
/*    contributors may be used to endorse or promote products derived from                        */
/*    this software without specific prior written permission.                                    */
/*                                                                                                */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                    */
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                      */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE                   */
/* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                     */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR                     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER                     */
/* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,                  */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE                  */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "timetick.h"
#include "Console.h"

#define SEND_BUFFER         (4096)
#define ETHERNET_MAX_LEN    (1300)
#define SOURCE_PORT         (2033)
#define DESTINATION_PORT    (2033)

#define ETHERNET_HEADER     14u
#define IP_HEADER           20u
#define UDP_HEADER          8u
#define TOTAL_UDP_HEADER    (ETHERNET_HEADER + IP_HEADER + UDP_HEADER)
#define HB(value)           ((uint8_t)((uint16_t)(value) >> 8) & 0xFF)
#define LB(value)           ((uint8_t)(value) & 0xFF)

static bool initialied = false;
static ConsolePrio_t minPrio = PRIO_LOW;
static uint8_t ethBuffer[TOTAL_UDP_HEADER];
static char txBuffer[SEND_BUFFER];
static uint32_t txBufPosIn = 0;
static uint32_t txBufPosOut = 0;
static uint32_t txOverflow = 0;

static void InitUdpHeaders();
static bool SendUdp(uint8_t *pPayload, uint32_t payloadLen);

void ConsoleInit()
{
    InitUdpHeaders();
    initialied = true;
}

void ConsoleDeinit(void)
{
    initialied = false;
}

void ConsoleSetPrio(ConsolePrio_t prio)
{
    if (!initialied)
        return;
    minPrio = prio;
}

void ConsolePrintf(ConsolePrio_t prio, const char *statement, ...)
{
    va_list args;
    if (!initialied) return;
    if (prio < minPrio || NULL == statement) return;
    if (0 == txBufPosIn && 0 != txOverflow)
    {
        snprintf(txBuffer, sizeof(txBuffer), RED "!! UART TX overflowed %lu times, increse 'ETHERNET_MAX_LEN' !!" RESETCOLOR "\r\n", txOverflow);
        txBufPosIn = strlen(txBuffer);
        txOverflow = 0;
    }
    va_start(args, statement);
    vsnprintf(&txBuffer[txBufPosIn], (sizeof(txBuffer) - txBufPosIn), statement, args);
    va_end(args);
    txBufPosIn = strlen(txBuffer);
    assert(txBufPosIn < sizeof(txBuffer));
    if ((sizeof(txBuffer) -1) == txBufPosIn)
    {
        ++txOverflow;
    }
    ConsoleCB_OnServiceNeeded();
}

void ConsoleService( void )
{
    if (!initialied) return;
	if (0 == txBufPosIn) return;
    do
    {
        uint32_t sendLen = txBufPosIn - txBufPosOut;
        if (sendLen > ETHERNET_MAX_LEN)
            sendLen = ETHERNET_MAX_LEN;
        assert(txBufPosIn >= txBufPosOut);
        if (SendUdp((uint8_t *)&txBuffer[txBufPosOut], sendLen))
        {
            txBufPosOut += sendLen;
            assert(txBufPosIn >= txBufPosOut);
            if (txBufPosIn == txBufPosOut)
            {
                txBufPosIn = 0;
                txBufPosOut = 0;
                break;
            }
        } else {
            ConsoleCB_OnServiceNeeded();
            break;
        }
    } while (true);
}

static void InitUdpHeaders()
{
    uint8_t *buff = ethBuffer;

    /* ---- Ethernet_HEADER ---- */
    //Destination MAC:
    /* 00 */ *buff++ = 0xffu; /* 01 */ *buff++ = 0xffu; /* 02 */ *buff++ = 0xffu; /* 03 */ *buff++ = 0xffu;
    /* 04 */ *buff++ = 0xffu; /* 05 */ *buff++ = 0xffu;
    //Source MAC:
    /* 06 */ *buff++ = 0x02u; /* 07 */ *buff++ = 0x00u; /* 08 */ *buff++ = 0x00u; /* 09 */ *buff++ = 0x01u;
    /* 10 */ *buff++ = 0x01u; /* 11 */ *buff++ = 0x01u;
    //Type
    /* 12 */ *buff++ = 0x08u; /* 13 */ *buff++ = 0x00u;

    /* ---- IP_HEADER ---- */
    /* 14 */ *buff++ = 0x45u; //Version
    /* 15 */ *buff++ = 0x00u; //Service Field
    /* 16, 17 will be filled by SendUdp() */ buff++; buff++;
    /* 18 */ *buff++ = 0x00u; /* 19 */ *buff++ = 0x0eu; //Identification
    /* 20 */ *buff++ = 0x00u; /* 21 */ *buff++ = 0x00u; //Flags & Fragment Offset
    /* 22 */ *buff++ = 0x40u; //TTL
    /* 23 */ *buff++ = 0x11u; //Protocol
    /* 24, 25 will be filled by SendUdp() */ buff++; buff++;
    //Source IP
    /* 26 */ *buff++ = 0x00u; /* 27 */ *buff++ = 0x00u; /* 28 */ *buff++ = 0x00u; /* 29 */ *buff++ = 0x00u;
    //Destination IP
    /* 30 */ *buff++ = 0xffu, /* 31 */ *buff++ = 0xffu; /* 32 */ *buff++ = 0xffu; /* 33 */ *buff++ = 0xffu;
    //UDP_HEADER
    /* 34 */ *buff++ = HB(SOURCE_PORT); /* 35 */ *buff++ = LB(SOURCE_PORT); //Source Port
    /* 36 */ *buff++ = HB(DESTINATION_PORT); /* 37*/ *buff++ = LB(DESTINATION_PORT); //Destination Port
    /* 38, 39 will be filled by SendUdp() */ buff++; buff++;
    /* 40 */ *buff++ = 0x00u; /* 41 */ *buff++ = 0x00u; //Checksum (optional)
}

static bool SendUdp(uint8_t *pPayload, uint32_t payloadLen)
{
    uint16_t ipLen = ETHERNET_HEADER + IP_HEADER + UDP_HEADER + payloadLen;
    uint32_t crcSum = 0ul;
    uint8_t crcCount = 0u;
    if (NULL == pPayload || 0 == payloadLen)
        return false;

    ethBuffer[16] = HB(ipLen - ETHERNET_HEADER);
    ethBuffer[17] = LB(ipLen - ETHERNET_HEADER); //Total Length

    //Header Checksum
    while (crcCount < 10u)
    {
        crcSum += (uint16_t)(ethBuffer[ETHERNET_HEADER + crcCount++] << 8u);
        crcSum += ethBuffer[ETHERNET_HEADER + crcCount++];
    }
    crcSum = ~((crcSum & 0xfffful) + (crcSum >> 16u));
    ethBuffer[24] = HB(crcSum);
    ethBuffer[25] = LB(crcSum);
    ethBuffer[38] = HB(UDP_HEADER + payloadLen);
    ethBuffer[39] = LB(UDP_HEADER + payloadLen);

    return ConsoleCB_SendDatagram(ethBuffer, sizeof(ethBuffer), pPayload, payloadLen);
}
