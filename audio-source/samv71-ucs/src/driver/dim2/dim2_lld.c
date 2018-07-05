/*------------------------------------------------------------------------------------------------*/
/* DIM2 LOW LEVEL DRIVER                                                                          */
/* (c) 2017 Microchip Technology Inc. and its subsidiaries.                                       */
/*                                                                                                */
/* You may use this software and any derivatives exclusively with Microchip products.             */
/*                                                                                                */
/* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR    */
/* STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,       */
/* MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP       */
/* PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.                      */
/*                                                                                                */
/* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR        */
/* CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE,    */
/* HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE       */
/* FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS   */
/* IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE  */
/* PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.                                                  */
/*                                                                                                */
/* MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE TERMS.            */
/*------------------------------------------------------------------------------------------------*/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ringbuffer.h"
#include "dim2_hal.h"
#include "dim2_lld.h"
#include "dim2_hardware.h"

//USE CASE SPECIFIC:
//Depending from this value, different buffer sizes must be used for synchronous streaming (ask for helper tool):
#define FCNT_VAL                        (5)

//How many RX and TX pairs available for sync / isoc use case
#define MAX_CHANNEL_INSTANCES           (4)

//Enable to debug
//#define LLD_TRACE


//Fixed values:
#define DMA_CHANNELS (32 - 1)  /* channel 0 is a system channel */

typedef struct {
    bool hwEnqueued;
    int16_t payloadLen;
    int16_t maxPayloadLen;
    uint16_t offset;
    uint8_t *buffer;
    uint8_t packetCounter;
} QueueEntry_t;

typedef struct {
    bool channelUsed;
    RingBuffer_t *ringBuffer;
    struct dim_channel *dimChannel;
    uint16_t amountOfEntries;
    QueueEntry_t *workingStruct;
    DIM2LLD_ChannelType_t cType;
    DIM2LLD_ChannelDirection_t dir;
    uint8_t lastPacketCount;
} ChannelContext_t;

typedef struct {
    bool initialized;
    ChannelContext_t controlLookupTable[DIM2LLD_ChannelDirection_BOUNDARY];
    ChannelContext_t asyncLookupTable[DIM2LLD_ChannelDirection_BOUNDARY];
    ChannelContext_t
    syncLookupTable[DIM2LLD_ChannelDirection_BOUNDARY][MAX_CHANNEL_INSTANCES];
    ChannelContext_t
    isocLookupTable[DIM2LLD_ChannelDirection_BOUNDARY][MAX_CHANNEL_INSTANCES];
    ///Zero terminated list of all dim channels. This used by the ISR routine.
    struct dim_channel *allChannels[DMA_CHANNELS];
} LocalVar_t;

static LocalVar_t lc = { 0 };

//This must be adapted use case specific
static ChannelContext_t *GetDimContext(DIM2LLD_ChannelType_t cType,
                                       DIM2LLD_ChannelDirection_t dir,
                                       uint8_t instance)
{
    assert(cType < DIM2LLD_ChannelType_BOUNDARY);
    assert(dir < DIM2LLD_ChannelDirection_BOUNDARY);
    assert(instance < MAX_CHANNEL_INSTANCES);
    if (cType >= DIM2LLD_ChannelType_BOUNDARY
        || dir >= DIM2LLD_ChannelDirection_BOUNDARY
        || instance >= MAX_CHANNEL_INSTANCES)
        return NULL;

    switch (cType) {
    case DIM2LLD_ChannelType_Control:
        if (0 != instance)
            return NULL;
        return &lc.controlLookupTable[dir];
    case DIM2LLD_ChannelType_Async:
        if (0 != instance)
            return NULL;
        return &lc.asyncLookupTable[dir];
    case DIM2LLD_ChannelType_Sync:
        return &lc.syncLookupTable[dir][instance];
    case DIM2LLD_ChannelType_Isoc:
        return &lc.isocLookupTable[dir][instance];
    default:
        break;
    }
    return NULL;
}

static void CleanUpContext(ChannelContext_t *context)
{
    uint16_t i;
    assert(NULL != context);
    if (!context->channelUsed)
        return;
    context->channelUsed = false;
    context->cType = DIM2LLD_ChannelType_BOUNDARY;
    context->dir = DIM2LLD_ChannelDirection_BOUNDARY;
    if (NULL != context->ringBuffer)
        free(context->ringBuffer);
    if (NULL != context->dimChannel)
        free(context->dimChannel);
    if (NULL != context->ringBuffer)
        RingBuffer_Deinit(context->ringBuffer);
    if (NULL != context->workingStruct) {
        for (i = 0; i < context->amountOfEntries; i++)
            if (NULL != context->workingStruct[i].buffer)
                free(context->workingStruct[i].buffer);
        free(context->workingStruct);
    }
}

static bool AddDimChannelToIsrList(struct dim_channel *ch)
{
    uint8_t i;
    bool added = false;
    assert(NULL != ch);
    for (i = 0; i < DMA_CHANNELS; i++) {
        if (lc.allChannels[i] != NULL)
            continue;
        lc.allChannels[i] = ch;
        added = true;
        break;
    }
    assert(added);
    return added;
}

bool DIM2LLD_Init(void)
{
    assert(!lc.initialized);
    memset(&lc, 0, sizeof(lc));
    lc.initialized = true;
    enable_mlb_clock();
    initialize_mlb_pins();
    disable_mlb_interrupt();
    if (DIM_NO_ERROR != dim_startup(DIM2_BASE_ADDRESS, DIM2_MLB_SPEED, FCNT_VAL))
        return false;
    enable_mlb_interrupt();

    return true;
}

bool DIM2LLD_SetupChannel(DIM2LLD_ChannelType_t cType,
                          DIM2LLD_ChannelDirection_t dir, uint8_t instance, uint16_t channelAddress,
                          uint16_t bufferSize, uint16_t subSize, uint16_t numberOfBuffers, uint16_t bufferOffset)
{
    uint8_t result;
    uint16_t i;
    ChannelContext_t *context;
    assert(lc.initialized);
    if (!lc.initialized)
        return false;
    if (DIM2LLD_ChannelDirection_TX == dir)
        bufferOffset = 0;
    context = GetDimContext(cType, dir, instance);
    if (NULL == context)
        return false;
    CleanUpContext(context);
    context->channelUsed = true;
    context->amountOfEntries = numberOfBuffers;
    switch (cType) {
    case DIM2LLD_ChannelType_Control:
    case DIM2LLD_ChannelType_Async:
        bufferSize = dim_norm_ctrl_async_buffer_size(bufferSize);
        break;
    case DIM2LLD_ChannelType_Sync:
        bufferSize = dim_norm_sync_buffer_size(bufferSize, subSize);
        break;
    case DIM2LLD_ChannelType_Isoc:
        bufferSize = dim_norm_isoc_buffer_size(bufferSize, subSize);
        break;
    default:
        assert(false);
        return false;
    }
    if (0 == bufferSize)
        return false;
    context->cType = cType;
    context->dir = dir;
    context->workingStruct = (QueueEntry_t *)calloc(numberOfBuffers,
                             sizeof(QueueEntry_t));
    for (i = 0; i < numberOfBuffers; i++) {
        context->workingStruct[i].offset = bufferOffset;
        context->workingStruct[i].maxPayloadLen = bufferSize;
        context->workingStruct[i].buffer = (uint8_t *)calloc(bufferSize + bufferOffset,
                                           sizeof(uint8_t));
        assert(NULL != context->workingStruct[i].buffer);
    }
    context->ringBuffer = (RingBuffer_t *)calloc(1, sizeof(RingBuffer_t));
    RingBuffer_Init(context->ringBuffer, numberOfBuffers, sizeof(QueueEntry_t),
                    context->workingStruct);
    context->dimChannel = calloc(1, sizeof(struct dim_channel));
    assert(NULL != context->workingStruct &&
           NULL != context->ringBuffer &&
           NULL != context->dimChannel);
    AddDimChannelToIsrList(context->dimChannel);
    disable_mlb_interrupt();
    switch (cType) {
    case DIM2LLD_ChannelType_Control:
        result = dim_init_control(context->dimChannel,
                                  (DIM2LLD_ChannelDirection_TX == dir), channelAddress, bufferSize);
        enable_mlb_interrupt();
        return (DIM_NO_ERROR == result);
    case DIM2LLD_ChannelType_Async:
        result = dim_init_async(context->dimChannel,
                                (DIM2LLD_ChannelDirection_TX == dir), channelAddress, bufferSize);
        enable_mlb_interrupt();
        return (DIM_NO_ERROR == result);
    case DIM2LLD_ChannelType_Sync:
        result = dim_init_sync(context->dimChannel,
                               (DIM2LLD_ChannelDirection_TX == dir), channelAddress, subSize);
        enable_mlb_interrupt();
        return (DIM_NO_ERROR == result);
    case DIM2LLD_ChannelType_Isoc:
        result = dim_init_isoc(context->dimChannel,
                               (DIM2LLD_ChannelDirection_TX == dir), channelAddress, subSize);
        enable_mlb_interrupt();
        return (DIM_NO_ERROR == result);
    default:
        enable_mlb_interrupt();
        assert(false);
        return false;
    }
}

void DIM2LLD_Deinit(void)
{
    uint16_t i;
    assert(lc.initialized);
    if (!lc.initialized)
        return;
    for (i = 0; i < (sizeof(lc.controlLookupTable) / sizeof(ChannelContext_t)); i++)
        CleanUpContext(&lc.controlLookupTable[i]);
    for (i = 0; i < (sizeof(lc.asyncLookupTable) / sizeof(ChannelContext_t)); i++)
        CleanUpContext(&lc.asyncLookupTable[i]);
    for (i = 0; i < (sizeof(lc.syncLookupTable) / sizeof(ChannelContext_t)); i++)
        CleanUpContext((ChannelContext_t *)&lc.syncLookupTable[i]);
    for (i = 0; i < (sizeof(lc.isocLookupTable) / sizeof(ChannelContext_t)); i++)
        CleanUpContext((ChannelContext_t *)&lc.isocLookupTable[i]);
    disable_mlb_interrupt();
    dim_shutdown();
    lc.initialized = false;
}

static void ServiceTxChannel(ChannelContext_t *context)
{
    uint32_t amountTx, i;
    uint16_t done_buffers;
    struct dim_ch_state_t st = { 0 };
    QueueEntry_t *entry;
    assert(lc.initialized);
    if (!lc.initialized)
        return;
    if (NULL == context || !context->channelUsed)
        return;
    assert(NULL != context->ringBuffer);
    assert(NULL != context->dimChannel);
    assert(DIM2LLD_ChannelDirection_TX == context->dir);
    disable_mlb_interrupt();
    dim_service_channel(context->dimChannel);
    enable_mlb_interrupt();

    //Try to release elements from hardware buffer:
    done_buffers = dim_get_channel_state(context->dimChannel, &st)->done_buffers;
    if (0 != done_buffers) {
        disable_mlb_interrupt();
        dim_detach_buffers(context->dimChannel, done_buffers);
        enable_mlb_interrupt();
    }
    for (i = 0; i < done_buffers; i++)
        RingBuffer_PopReadPtr(context->ringBuffer);

    //Try to enqueue new elements into hardware buffer:
    amountTx = RingBuffer_GetReadElementCount(context->ringBuffer);
    for (i = 0; i < amountTx; i++) {
        if (!dim_get_channel_state(context->dimChannel, &st)->ready)
            break;
        entry = (QueueEntry_t *)RingBuffer_GetReadPtrPos(context->ringBuffer, i);
        assert(NULL != entry);
        if (NULL == entry || 0 == entry->payloadLen || entry->hwEnqueued)
            continue;
#ifdef LLD_TRACE
        {
            int16_t l;
            printf("DIM2LLD_TX: [");
            for (l = 0; l < entry->payloadLen; l++) {
                printf(" %02X", entry->buffer[l]);
            }
            printf(" ]\r\n");
        }
#endif
        disable_mlb_interrupt();
        if (dim_dbr_space(context->dimChannel) < entry->payloadLen) {
            enable_mlb_interrupt();
            break;
        }
        if (dim_enqueue_buffer(context->dimChannel, (uint32_t)entry->buffer,
                               entry->payloadLen)) {
            enable_mlb_interrupt();
            entry->hwEnqueued = true;
        } else {
            enable_mlb_interrupt();
            break;
        }
    }
}

static void ServiceRxChannel(ChannelContext_t *context)
{
    int32_t amountRx, i;
    uint16_t done_buffers;
    struct dim_ch_state_t st = { 0 };
    QueueEntry_t *entry;
    assert(lc.initialized);
    if (!lc.initialized)
        return;
    if (NULL == context || !context->channelUsed)
        return;
    assert(DIM2LLD_ChannelDirection_RX == context->dir);

    disable_mlb_interrupt();
    dim_service_channel(context->dimChannel);
    enable_mlb_interrupt();

    //Enqueue empty buffers into hardware
    while (NULL != (entry = (QueueEntry_t *)RingBuffer_GetWritePtr(
                                context->ringBuffer))) {
        if (!dim_get_channel_state(context->dimChannel, &st)->ready)
            break;

        disable_mlb_interrupt();
        if (dim_enqueue_buffer(context->dimChannel, (uint32_t)&entry->buffer[entry->offset],
                               entry->maxPayloadLen)) {
            enable_mlb_interrupt();
            entry->hwEnqueued = true;
            entry->packetCounter = context->lastPacketCount++;
            RingBuffer_PopWritePtr(context->ringBuffer);
        } else {
            enable_mlb_interrupt();
            break;
        }
    }

    //Handle filled RX buffers
    done_buffers = dim_get_channel_state(context->dimChannel, &st)->done_buffers;
    if (0 != done_buffers) {
        disable_mlb_interrupt();
        dim_detach_buffers(context->dimChannel, done_buffers);
        enable_mlb_interrupt();

        amountRx = RingBuffer_GetReadElementCount(context->ringBuffer);
        assert(done_buffers <= amountRx);
        for (i = 0; i < done_buffers && i < amountRx; i++) {
            entry = (QueueEntry_t *)RingBuffer_GetReadPtrPos(context->ringBuffer, i);
            assert(NULL != entry);
            if (NULL == entry || !entry->hwEnqueued)
                continue;
            if (DIM2LLD_ChannelType_Control == context->cType ||
                DIM2LLD_ChannelType_Async == context->cType)
                entry->payloadLen = (uint16_t)entry->buffer[entry->offset] * 256 + entry->buffer[entry->offset + 1] + 2;
            else
                entry->payloadLen = entry->maxPayloadLen;
            assert(entry->payloadLen <= entry->maxPayloadLen);
#ifdef LLD_TRACE
            {
                int16_t l;
                printf("DIM2LLD_RX: [");
                for (l = 0; l < entry->payloadLen; l++) {
                    printf(" %02X", entry->buffer[entry->offset + l]);
                }
                printf(" ]\r\n");
            }
#endif
            entry->hwEnqueued = false;
        }
    }
}

void DIM2LLD_Service(void)
{
    uint16_t i;
    assert(lc.initialized);
    if (!lc.initialized)
        return;

    //Handle TX channels
    ServiceTxChannel(&lc.controlLookupTable[DIM2LLD_ChannelDirection_TX]);
    ServiceTxChannel(&lc.asyncLookupTable[DIM2LLD_ChannelDirection_TX]);
    for (i = 0; i < MAX_CHANNEL_INSTANCES; i++) {
        ServiceTxChannel(&lc.syncLookupTable[DIM2LLD_ChannelDirection_TX][i]);
        ServiceTxChannel(&lc.isocLookupTable[DIM2LLD_ChannelDirection_TX][i]);
    }

    //Handle RX channels
    ServiceRxChannel(&lc.controlLookupTable[DIM2LLD_ChannelDirection_RX]);
    ServiceRxChannel(&lc.asyncLookupTable[DIM2LLD_ChannelDirection_RX]);
    for (i = 0; i < MAX_CHANNEL_INSTANCES; i++) {
        ServiceRxChannel(&lc.syncLookupTable[DIM2LLD_ChannelDirection_RX][i]);
        ServiceRxChannel(&lc.isocLookupTable[DIM2LLD_ChannelDirection_RX][i]);
    }
}

bool DIM2LLD_IsMlbLocked(void)
{
    assert(lc.initialized);
    if (!lc.initialized)
        return false;
    return dim_get_lock_state();
}

uint32_t DIM2LLD_GetQueueElementCount(DIM2LLD_ChannelType_t cType, DIM2LLD_ChannelDirection_t dir, uint8_t instance)
{
    ChannelContext_t *context;
    if (!lc.initialized)
        return 0;
    context = GetDimContext(cType, dir, instance);
    if (NULL == context || !context->channelUsed)
        return 0;
    return RingBuffer_GetReadElementCount(context->ringBuffer);
}

uint16_t DIM2LLD_GetRxData(DIM2LLD_ChannelType_t cType, DIM2LLD_ChannelDirection_t dir, 
                           uint8_t instance, uint32_t pos, const uint8_t **pBuffer, uint16_t *pOffset, uint8_t *pPacketCounter)
{
    ChannelContext_t *context;
    QueueEntry_t *entry;
    if (!lc.initialized)
        return 0;
    if (NULL == pBuffer)
        return 0;
    *pBuffer = NULL;
    if (NULL != pOffset)
        *pOffset = 0;
    if (NULL != pPacketCounter)
        *pPacketCounter = 0;        
    context = GetDimContext(cType, dir, instance);
    if (NULL == context || !context->channelUsed)
        return 0;
    assert(context->cType == cType && context->dir == dir && NULL != context->ringBuffer);
    entry = (QueueEntry_t *)RingBuffer_GetReadPtrPos(context->ringBuffer, pos);
    if (NULL == entry || entry->hwEnqueued)
        return 0;
    *pBuffer = entry->buffer;
    if (NULL != pOffset)
        *pOffset = entry->offset;
    if (NULL != pPacketCounter)
        *pPacketCounter = entry->packetCounter;
    return entry->payloadLen;
}

void DIM2LLD_ReleaseRxData(DIM2LLD_ChannelType_t cType,
                           DIM2LLD_ChannelDirection_t dir, uint8_t instance)
{
    ChannelContext_t *context;
    assert(lc.initialized);
    if (!lc.initialized)
        return;
    context = GetDimContext(cType, dir, instance);
    if (NULL == context)
    {
        assert(false);
        return;
    }        
    RingBuffer_PopReadPtr(context->ringBuffer);
}

uint16_t DIM2LLD_GetTxData(DIM2LLD_ChannelType_t cType,
                           DIM2LLD_ChannelDirection_t dir, uint8_t instance, uint8_t **pBuffer)
{
    ChannelContext_t *context;
    QueueEntry_t *entry;
    if (!lc.initialized)
        return 0;
    if (NULL == pBuffer)
        return 0;
    *pBuffer = NULL;
    context = GetDimContext(cType, dir, instance);
    if (NULL == context || !context->channelUsed)
        return 0;
    assert(context->cType == cType && context->dir == dir && NULL != context->ringBuffer);        
    entry = (QueueEntry_t *)RingBuffer_GetWritePtr(context->ringBuffer);
    if (NULL == entry)
        return 0;
    *pBuffer = entry->buffer;
    return entry->maxPayloadLen;
}

void DIM2LLD_SendTxData(DIM2LLD_ChannelType_t cType,
                        DIM2LLD_ChannelDirection_t dir, uint8_t instance, uint32_t payloadLength)
{
    ChannelContext_t *context;
    QueueEntry_t *entry;
    assert(lc.initialized);
    assert(0 != payloadLength);
    if (!lc.initialized || 0 == payloadLength)
        return;
    context = GetDimContext(cType, dir, instance);
    if (NULL == context)
        return;
    assert(DIM2LLD_ChannelDirection_TX == context->dir) ;
    entry = (QueueEntry_t *)RingBuffer_GetWritePtr(context->ringBuffer);
    assert(NULL != entry);
    if (NULL == entry)
        return;
    entry->payloadLen = payloadLength;
    entry->hwEnqueued = false;
    RingBuffer_PopWritePtr(context->ringBuffer);
}

void on_mlb_int_isr(void)
{
    assert(lc.initialized);
    if (!lc.initialized)
        return;

    dim_service_mlb_int_irq();
}

void on_ahb0_int_isr(void)
{
    assert(lc.initialized);
    if (!lc.initialized)
        return;

    dim_service_ahb_int_irq(lc.allChannels);
}
