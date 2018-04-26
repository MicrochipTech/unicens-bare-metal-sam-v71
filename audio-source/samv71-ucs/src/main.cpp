/*------------------------------------------------------------------------------------------------*/
/* UNICENS Daemon (unicensd) main-loop                                                            */
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "board_init.h"
#include "dim2_lld.h"
#include "ucsi_api.h"
#include "Console.h"
#include "default_config.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          USER ADJUSTABLE                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/*#define LLD_TRACE*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      DEFINES AND LOCAL VARIABLES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

typedef struct
{
    DIM2LLD_ChannelType_t cType;
    DIM2LLD_ChannelDirection_t dir;
    uint8_t instance;
    uint16_t channelAddress;
    uint16_t bufferSize;
    uint16_t subSize;
    uint16_t numberOfBuffers;
    uint16_t bufferOffset;
} DIM2_Setup_t;

typedef struct
{
    uint32_t lastToggle;
    UCSI_Data_t unicens;
    bool unicensRunning;
    uint32_t unicensTimeout;
    bool unicensTrigger;
    bool consoleTrigger;
    bool gmacSendInProgress;
    bool amsReceived;
    bool pongReceived;
    uint32_t audioPos;
} LocalVar_t;

static LocalVar_t m;

static DIM2_Setup_t mlbConfig[] =
{
    {
        .cType = DIM2LLD_ChannelType_Control,
        .dir = DIM2LLD_ChannelDirection_RX,
        .instance = 0,
        .channelAddress = 2,
        .bufferSize = 72,
        .subSize = 0,
        .numberOfBuffers = 8,
        .bufferOffset = 0
    }, {
        .cType = DIM2LLD_ChannelType_Control,
        .dir = DIM2LLD_ChannelDirection_TX,
        .instance = 0,
        .channelAddress = 4,
        .bufferSize = 72,
        .subSize = 0,
        .numberOfBuffers = 8,
        .bufferOffset = 0
    }, {
        .cType = DIM2LLD_ChannelType_Sync,
        .dir = DIM2LLD_ChannelDirection_TX,
        .instance = 0,
        .channelAddress = 10,
        .bufferSize = 512,
        .subSize = 4,
        .numberOfBuffers = 4,
        .bufferOffset = 0
    }
};
static const uint32_t mlbConfigSize = sizeof(mlbConfig) / sizeof(DIM2_Setup_t);

static const uint8_t audioData[] =
{
#include "beat_be.h"
};

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      PRIVATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void ServiceMostCntrlRx();
static void ServiceMostSyncTx();
static void GmacTransferCallback(uint32_t status, void *pTag);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

int main()
{
    BoardInit();
    memset(&m, 0, sizeof(LocalVar_t));
    ConsoleInit();
    ConsolePrintf(PRIO_HIGH, BLUE "------|V71 UNICENS sample start (BUILD %s %s)|------" RESETCOLOR "\r\n", __DATE__, __TIME__);

    // Initialize MOST DIM2 driver
    DIM2LLD_Init();
    Wait(100);
    while (!DIM2LLD_IsMlbLocked())
    {
        ConsolePrintf(PRIO_ERROR, RED "MLB is not locked!" RESETCOLOR "\r\n");
        Wait(1000);
    }
    for (uint32_t i = 0; i < mlbConfigSize; i++)
    {
        if (!DIM2LLD_SetupChannel(mlbConfig[i].cType, mlbConfig[i].dir, mlbConfig[i].instance, mlbConfig[i].channelAddress,
            mlbConfig[i].bufferSize, mlbConfig[i].subSize, mlbConfig[i].numberOfBuffers, mlbConfig[i].bufferOffset))
        {
            ConsolePrintf(PRIO_ERROR, "Failed to MLB channel with address=0x%X\r\n", mlbConfig[i].channelAddress);
            assert(false);
        }
    }

    /* Initialize UNICENS */
    UCSI_Init(&m.unicens, &m);
    if (!UCSI_NewConfig(&m.unicens, PacketBandwidth, AllRoutes, RoutesSize, AllNodes, NodeSize))
    {
        ConsolePrintf(PRIO_ERROR, RED "Could not enqueue new UNICENS config" RESETCOLOR "\r\n");
        assert(false);
        while(true);
    }

    while (1)
    {
        ServiceMostCntrlRx();
        ServiceMostSyncTx();
        uint32_t now = GetTicks();
        /* UNICENS Service */
        if (m.unicensTrigger)
        {
            m.unicensTrigger = false;
            UCSI_Service(&m.unicens);
        }
        if (m.consoleTrigger)
        {
            m.consoleTrigger = false;
            ConsoleService();
        }
        if (0 != m.unicensTimeout && now >= m.unicensTimeout)
        {
            m.unicensTimeout = 0;
            UCSI_Timeout(&m.unicens);
        }
        if (m.amsReceived)
        {
            uint16_t amsId = 0xFFFF;
            uint16_t sourceAddress = 0xFFFF;
            uint8_t *pBuf = NULL;
            uint32_t len = 0;
            m.amsReceived = false;
            if (UCSI_GetAmsMessage(&m.unicens, &amsId, &sourceAddress, &pBuf, &len))
            {
                ConsolePrintf(PRIO_HIGH, "Received AMS, id=0x%X, source=0x%X, len=%lu\r\n", amsId, sourceAddress, len);
                if (0 == amsId)
                    m.pongReceived = true;
                UCSI_ReleaseAmsMessage(&m.unicens);
            }
            else assert(false);
        }
        /* Cyclic LED handling */
        if (now - m.lastToggle >= 333)
        {
            m.lastToggle = now;
            LED_Toggle(0);
        }
    }
    return 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  PRIVATE FUNCTION IMPLEMENTATIONS                    */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void ServiceMostCntrlRx()
{
    uint16_t bufLen;
    uint8_t *pBuf;
    DIM2LLD_Service();
    do
    {
        bufLen = DIM2LLD_GetRxData(DIM2LLD_ChannelType_Control, DIM2LLD_ChannelDirection_RX, 0, 0, &pBuf, NULL, NULL);
        if (0 != bufLen)
        {
            if (m.unicensRunning)
            {
#ifdef LLD_TRACE
                ConsolePrintf( PRIO_HIGH, BLUE "%08lu MSG_RX(%d): ", GetTicks(), bufLen);
                for ( int16_t i = 0; i < bufLen; i++ )
                {
                    ConsolePrintf( PRIO_HIGH, "%02X ", pBuf[i] );
                }
                ConsolePrintf( PRIO_HIGH, RESETCOLOR"\n");
#endif
                if (!UCSI_ProcessRxData(&m.unicens, pBuf, bufLen))
                {
                    ConsolePrintf(PRIO_ERROR, "RX buffer overflow\r\n");
                    /* UNICENS is busy. Try to reactive it, by calling service routine */
                    m.unicensTrigger = true;
                    break;
                }
            }
            DIM2LLD_ReleaseRxData(DIM2LLD_ChannelType_Control, DIM2LLD_ChannelDirection_RX, 0);
        }
    }
    while (0 != bufLen);
}

static void ServiceMostSyncTx()
{
    while(true)
    {
        uint8_t *pBuf;
        uint16_t maxLen = DIM2LLD_GetTxData(DIM2LLD_ChannelType_Sync, DIM2LLD_ChannelDirection_TX, 0, &pBuf);
        if (0 == maxLen)
            break;
        for (uint16_t i = 0; i < maxLen; i++)
        {
            pBuf[i] = audioData[m.audioPos++];
            if (sizeof(audioData) <= m.audioPos)
                m.audioPos = 0;
        }
        DIM2LLD_SendTxData(DIM2LLD_ChannelType_Sync, DIM2LLD_ChannelDirection_TX, 0, maxLen);
    }
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  CALLBACK FUNCTIONS FROM UNICENS                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

uint16_t UCSI_CB_OnGetTime(void *pTag)
{
    return GetTicks();
}

void UCSI_CB_OnSetServiceTimer(void *pTag, uint16_t timeout)
{
    if (0 == timeout)
        m.unicensTimeout = 0;
    else
        m.unicensTimeout = GetTicks() + timeout;
}

void UCSI_CB_OnNetworkState(void *pTag, bool isAvailable, uint16_t packetBandwidth, uint8_t amountOfNodes)
{
    pTag = pTag;
    ConsolePrintf(PRIO_HIGH, YELLOW "Network isAvailable=%s, packetBW=%d, nodeCount=%d" RESETCOLOR "\r\n",
        isAvailable ? "yes" : "no", packetBandwidth, amountOfNodes);
}

void UCSI_CB_OnUserMessage(void *pTag, bool isError, const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    char outbuf[300];
    pTag = pTag;
    va_start(argptr, vargsCnt);
    vsnprintf(outbuf, sizeof(outbuf), format, argptr);
    va_end(argptr);
    if (isError)
        ConsolePrintf(PRIO_ERROR, RED "%s" RESETCOLOR "\r\n", outbuf);
    else
        ConsolePrintf(PRIO_HIGH, "%s\r\n", outbuf);
}

void UCSI_CB_OnServiceRequired(void *pTag)
{
    m.unicensTrigger = true;
}

void UCSI_CB_OnTxRequest(void *pTag,
    const uint8_t *pPayload, uint32_t payloadLen)
{
    pTag = pTag;
    assert(pTag == &m);
    uint8_t *pBuf = NULL;
#ifdef LLD_TRACE
{
    ConsolePrintf( PRIO_HIGH, BLUE "%08lu MSG_TX(%lu): ", GetTicks(), payloadLen);
    for ( uint32_t i = 0; i < payloadLen; i++ )
    {
        ConsolePrintf( PRIO_HIGH, "%02X ", pPayload[i] );
    }
    ConsolePrintf(PRIO_HIGH, RESETCOLOR "\n");
}
#endif
    uint32_t txMaxLen = 0;
    while (0 == txMaxLen)
    {
        txMaxLen = DIM2LLD_GetTxData(DIM2LLD_ChannelType_Control, DIM2LLD_ChannelDirection_TX, 0, &pBuf);
    }
    if (NULL == pBuf || txMaxLen < payloadLen)
    {
        ConsolePrintf(PRIO_ERROR, RED "UCSI_CB_SendMostMessage buffer is too small! %lu < %lu" RESETCOLOR "\r\n", txMaxLen, payloadLen);
        assert(false);
        return;
    }
    memcpy(pBuf, pPayload, payloadLen);
    DIM2LLD_SendTxData(DIM2LLD_ChannelType_Control, DIM2LLD_ChannelDirection_TX, 0, payloadLen);
}

void UCSI_CB_OnStart(void *pTag)
{
    m.unicensRunning = true;
}

void UCSI_CB_OnStop(void *pTag)
{
    m.unicensRunning = false;
}

void UCSI_CB_OnAmsMessageReceived(void *pTag)
{
    m.amsReceived = true;
}

void UCSI_CB_OnRouteResult(void *pTag, uint16_t routeId, bool isActive, uint16_t connectionLabel)
{
    ConsolePrintf(PRIO_HIGH, "Route id=0x%X isActive=%s ConLabel=0x%X\r\n", routeId,
        (isActive ? "true" : "false"), connectionLabel);
}

void UCSI_CB_OnGpioStateChange(void *pTag, uint16_t nodeAddress, uint8_t gpioPinId, bool isHighState)
{
    ConsolePrintf(PRIO_HIGH, "GPIO state changed, nodeAddress=0x%X, gpioPinId=%d, isHighState=%s\r\n",
        nodeAddress, gpioPinId, isHighState ? "yes" : "no");
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