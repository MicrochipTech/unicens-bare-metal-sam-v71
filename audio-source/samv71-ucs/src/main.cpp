/*------------------------------------------------------------------------------------------------*/
/* UNICENS Daemon (unicensd) main-loop                                                            */
/* Copyright 2018, Microchip Technology Inc. and its subsidiaries.                                */
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "board_init.h"
#include "Console.h"
#include "task-unicens.h"
#include "task-audio.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          USER ADJUSTABLE                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/* UNICENS daemon version number */
#define UNICENSD_VERSION    ("V4.1.0")

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      DEFINES AND LOCAL VARIABLES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

typedef struct
{
    uint32_t lastToggle;
    bool consoleTrigger;
    bool gmacSendInProgress;
} LocalVar_t;

static LocalVar_t m;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      PRIVATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void GmacTransferCallback(uint32_t status, void *pTag);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

int main()
{
    Board_Init();
    memset(&m, 0, sizeof(LocalVar_t));
    ConsoleInit();
    ConsolePrintf(PRIO_HIGH, BLUE "------|V71 UNICENS sample start %s (BUILD %s %s)|------" RESETCOLOR "\r\n", UNICENSD_VERSION, __DATE__, __TIME__);
    if (!TaskUnicens_Init())
        ConsolePrintf(PRIO_ERROR, RED "Init of Task UNICENS Init Failed" RESETCOLOR "\r\n");
    if (!TaskAudio_Init())
        ConsolePrintf(PRIO_ERROR, RED "Init of Task Audio Failed" RESETCOLOR "\r\n");
    while (1)
    {
        uint32_t now = GetTicks();
        TaskUnicens_Service();
        TaskAudio_Service();
        if (m.consoleTrigger)
        {
            m.consoleTrigger = false;
            ConsoleService();
        }
        if (now - m.lastToggle >= 333)
        {
            m.lastToggle = now;
            LED_Toggle(0);
        }
    }
    return 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                 CALLBACK FUNCTIONS FROM CONSOLE                      */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void ConsoleCB_OnServiceNeeded(void)
{
    m.consoleTrigger = true;
}

bool ConsoleCB_SendDatagram( uint8_t *pEthHeader, uint32_t ethLen, uint8_t *pPayload, uint32_t payloadLen )
{
    sGmacSGList sgl;
    sGmacSG sg[2];
    if (m.gmacSendInProgress)
        return false;
    sg[0].pBuffer = pEthHeader;
    sg[0].size = ethLen;
    sg[1].pBuffer = pPayload;
    sg[1].size = payloadLen;
    sgl.sg = sg;
    sgl.len = 2;
    if (GMACD_OK != GMACD_SendSG(&gGmacd, &sgl, GmacTransferCallback, NULL, GMAC_QUE_0))
        return false;
    m.gmacSendInProgress = true;
    return true;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  CALLBACK FUNCTIONS FROM GMAC                        */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void GmacTransferCallback(uint32_t status, void *pTag)
{
    m.gmacSendInProgress = false;
}