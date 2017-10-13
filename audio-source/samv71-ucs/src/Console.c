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
#include "timetick.h"
#include "Console.h"

#define SUPPRESS_THRESHOLD  (100)
#define SOURCE_PORT         (2033)
#define DESTINATION_PORT    (2033)

#define ETHERNET_HEADER     14u
#define IP_HEADER           20u
#define UDP_HEADER          8u
#define TOTAL_UDP_HEADER    (ETHERNET_HEADER + IP_HEADER + UDP_HEADER)

static bool initialied = false;
static ConsolePrio_t minPrio = PRIO_LOW;
static ConsolePrio_t sectionPrio = PRIO_LOW;
static uint32_t bufferOffset = TOTAL_UDP_HEADER;
static char buffer[1300];
static char cbuffer[1300];
static uint32_t callCount = 0;
static uint16_t lastTime = 0;

#define BUF_LEN (sizeof(buffer) - bufferOffset)
#define CBUF_LEN (sizeof(cbuffer) - TOTAL_UDP_HEADER)

static void SendUdp(uint8_t *pBuf, uint32_t len);
static bool CheckTime(void);

void ConsoleInit()
{
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

void ConsoleAddPrefix(const char *prefix)
{
    if (!initialied)
        return;
    uint32_t len;
    if (NULL == prefix)
        return;
    len = strlen(prefix);
    if (0 == len || len > sizeof(buffer))
        return;
    bufferOffset += len;
    memcpy(&buffer[TOTAL_UDP_HEADER], prefix, len);
    memcpy(&cbuffer[TOTAL_UDP_HEADER], prefix, len);
}

void ConsolePrintf(ConsolePrio_t prio, const char *statement, ...)
{
    if (!initialied)
        return;
    if (prio < minPrio || NULL == statement)
        return;
	if (!CheckTime())
		return;

    va_list args;
    va_start(args, statement);
    vsnprintf(&buffer[bufferOffset], BUF_LEN, statement, args);
    va_end(args);
    SendUdp((uint8_t *)buffer, strlen(&buffer[TOTAL_UDP_HEADER]));
}

void ConsolePrintfStart(ConsolePrio_t prio, const char *statement, ...)
{
    if (!initialied)
        return;
    sectionPrio = prio;
    if (prio < minPrio || NULL == statement)
        return;
    cbuffer[0] = '\0';
    va_list args;
    va_start(args, statement);
    vsnprintf(&cbuffer[bufferOffset], CBUF_LEN, statement, args);
    va_end(args);
}

void ConsolePrintfContinue(const char *statement, ...)
{
    size_t resultSize;
    if (!initialied)
        return;
    char temp[100];
    if (sectionPrio < minPrio || NULL == statement)
        return;
    va_list args;
    va_start(args, statement);
    vsnprintf(temp, sizeof(temp), statement, args);
    va_end(args);
    resultSize = strlcat(&cbuffer[TOTAL_UDP_HEADER], temp, CBUF_LEN);
    if (resultSize >= CBUF_LEN)
    {
        //cat process truncated string to fit in buffer
        SendUdp((uint8_t *)cbuffer, strlen(&cbuffer[TOTAL_UDP_HEADER]));
        //Copy to rest to the begin of buffer, as it is now ready to be written again
        strcpy(&cbuffer[TOTAL_UDP_HEADER], &temp[resultSize - CBUF_LEN + 1]);
    }
}

void ConsolePrintfExit(const char *statement, ...)
{
    if (!initialied)
        return;
    char temp[100];
    if (sectionPrio < minPrio)
        return;
	if(NULL != statement)
	{
		va_list args;
		va_start(args, statement);
		vsnprintf(temp, sizeof(temp), statement, args);
		va_end(args);
		strlcat(&cbuffer[TOTAL_UDP_HEADER], temp, CBUF_LEN);
	}
	SendUdp((uint8_t *)cbuffer, strlen(&cbuffer[TOTAL_UDP_HEADER]));
    cbuffer[bufferOffset] = '\0';
}

static void SendUdp(uint8_t *pBuf, uint32_t len)
{
#define HB(value)		    ((uint8_t)((uint16_t)(value) >> 8) & 0xFF)
#define LB(value)		    ((uint8_t)(value) & 0xFF)

    uint8_t *buff = pBuf;
    uint16_t ipLen = ETHERNET_HEADER + IP_HEADER + UDP_HEADER + len;
    uint32_t crcSum = 0ul;
    uint8_t crcCount = 0u;
    if (NULL == pBuf || 0 == len)
        return;

    /* ---- Ethernet_HEADER ---- */
    //Destination MAC:
    *buff++ = 0xffu; *buff++ = 0xffu; *buff++ = 0xffu; *buff++ = 0xffu;
    *buff++ = 0xffu; *buff++ = 0xffu;
    //Source MAC:
    *buff++ = 0x02u; *buff++ = 0x00u; *buff++ = 0x00u; *buff++ = 0x01u;
    *buff++ = 0x01u; *buff++ = 0x01u;
    //Type
    *buff++ = 0x08u; *buff++ = 0x00u;

    /* ---- IP_HEADER ---- */
    *buff++ = 0x45u; //Version
    *buff++ = 0x00u; //Service Field
    *buff++ = HB(ipLen - ETHERNET_HEADER);
    *buff++ = LB(ipLen - ETHERNET_HEADER); //Total Length
    *buff++ = 0x00u; *buff++ = 0x0eu; //Identification
    *buff++ = 0x00u; *buff++ = 0x00u; //Flags & Fragment Offset
    *buff++ = 0x40u; //TTL
    *buff++ = 0x11u; //Protocol

    //Header Checksum
    while (crcCount < 10u)
    {
        crcSum += (uint16_t)(pBuf[ETHERNET_HEADER + crcCount++] << 8u);
        crcSum += pBuf[ETHERNET_HEADER + crcCount++];
    }
    crcSum = ~((crcSum & 0xfffful) + (crcSum >> 16u));
    *buff++ = HB(crcSum); *buff++ = LB(crcSum);

    //Source IP
    *buff++ = 0x00u; *buff++ = 0x00u; *buff++ = 0x00u; *buff++ = 0x00u;
    //Destination IP
    *buff++ = 0xffu, *buff++ = 0xffu; *buff++ = 0xffu; *buff++ = 0xffu;
    //UDP_HEADER
    *buff++ = HB(SOURCE_PORT); *buff++ = LB(SOURCE_PORT); //Source Port
    *buff++ = HB(DESTINATION_PORT); *buff++ = LB(DESTINATION_PORT); //Destination Port
    *buff++ = HB(UDP_HEADER + len);
    *buff++ = LB(UDP_HEADER + len); //Length
    *buff++ = 0x00u; *buff++ = 0x00u; //Checksum (optional)

    ConsoleCB_SendDatagram(pBuf, TOTAL_UDP_HEADER + len);
}

static bool CheckTime(void)
{
    char temp[100];
    uint32_t now = GetTicks();
    uint32_t diff = now - lastTime;
    ++callCount;
    if (diff < 1000)
    {
        return (callCount <= SUPPRESS_THRESHOLD);
    }
    else
    {
        if (callCount > SUPPRESS_THRESHOLD)
        {
            snprintf(&temp[TOTAL_UDP_HEADER], sizeof(temp), "! Suppressed %lu lines !\r\n", callCount);
            SendUdp((uint8_t *)temp, strlen(&temp[TOTAL_UDP_HEADER]));
        }
        callCount = 0;
        lastTime = now;
        return true;
    }
}