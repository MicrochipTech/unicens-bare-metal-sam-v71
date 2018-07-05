/*------------------------------------------------------------------------------------------------*/
/* Audio Processing Task Implementation                                                           */
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

#include <string.h>
#include <assert.h>
#include "Console.h"
#include "dim2_lld.h"
#include "task-audio.h"

/* #define ENABLE_AUDIO_RX */

struct TaskAudioVars
{
    bool initialized;
    uint32_t audioPos;
};
static struct TaskAudioVars m = { 0 };
static const uint8_t audioData[] =
{
    #include "beat_be.h"
};


static bool ProcessStreamingData(const uint8_t *pRxBuf, uint32_t rxLen, uint8_t *pTxBuf, uint32_t txLen);

bool TaskAudio_Init(void)
{
    memset(&m, 0, sizeof(m));
    m.initialized = true;
    return true;
}

void TaskAudio_Service(void)
{
    while(true)
    {
        uint8_t *pTxBuf = NULL;
        const uint8_t *pRxBuf = NULL;
        uint16_t rxLen = 0;
        uint16_t txLen = 0;
#if ENABLE_AUDIO_RX
        rxLen = DIM2LLD_GetRxData(DIM2LLD_ChannelType_Sync, DIM2LLD_ChannelDirection_RX, 0, 0, &pRxBuf, NULL, NULL);
        if (0 == rxLen)
            break;
#endif
        txLen = DIM2LLD_GetTxData(DIM2LLD_ChannelType_Sync, DIM2LLD_ChannelDirection_TX, 0, &pTxBuf);
        if (0 == txLen)
            break;
        if (ProcessStreamingData(pRxBuf, rxLen, pTxBuf, txLen))
        {
#if ENABLE_AUDIO_RX
            DIM2LLD_ReleaseRxData(DIM2LLD_ChannelType_Sync, DIM2LLD_ChannelDirection_RX, 0);
#endif
            DIM2LLD_SendTxData(DIM2LLD_ChannelType_Sync, DIM2LLD_ChannelDirection_TX, 0, txLen);
        }
        else break;
    }
}

static bool ProcessStreamingData(const uint8_t *pRxBuf, uint32_t rxLen, uint8_t *pTxBuf, uint32_t txLen)
{
    uint32_t i;
    if (!m.initialized)
        return false;
    for (i = 0; i < txLen; i++)
    {
        pTxBuf[i] = audioData[m.audioPos++];
        if (sizeof(audioData) <= m.audioPos)
            m.audioPos = 0;
    }
    return true;
}