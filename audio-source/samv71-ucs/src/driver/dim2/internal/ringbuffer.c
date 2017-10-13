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

#include <stdlib.h>
#include <assert.h>
#include "ringbuffer.h"

void RingBuffer_Init(RingBuffer_t *rb, uint16_t amountOfEntries,
                     uint32_t sizeOfEntry, void *workingBuffer)
{
    assert(NULL != rb);
    assert(NULL != workingBuffer);
    rb->dataQueue = (uint32_t)workingBuffer;
    rb->pRx = (uint32_t)rb->dataQueue;
    rb->pTx = (uint32_t)rb->dataQueue;
    rb->amountOfEntries = amountOfEntries;
    rb->sizeOfEntry = sizeOfEntry;
    rb->rxPos = 0;
    rb->txPos = 0;
}

void RingBuffer_Deinit(RingBuffer_t *rb)
{
    assert(NULL != rb);
    rb->dataQueue = rb->amountOfEntries = rb->rxPos = rb->txPos = rb->pRx = rb->pTx = 0;
}

uint32_t RingBuffer_GetReadElementCount(RingBuffer_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    return (uint32_t)(rb->txPos - rb->rxPos);
}

void *RingBuffer_GetReadPtr(RingBuffer_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    if (rb->txPos - rb->rxPos > 0)
        return (void *)rb->pRx;
    return NULL;
}

void *RingBuffer_GetReadPtrPos(RingBuffer_t *rb, uint32_t pos)
{
    uint32_t i, t;
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    if (rb->txPos - rb->rxPos <= pos)
        return NULL;
    t = rb->pRx;
    for (i = 0; i < pos; i++) {
        t += rb->sizeOfEntry;
        if (t >= (uint32_t)rb->dataQueue + (rb->amountOfEntries * rb->sizeOfEntry))
            t = rb->dataQueue;
    }
    return (void *)t;
}

void RingBuffer_PopReadPtr(RingBuffer_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    
    rb->pRx += rb->sizeOfEntry;
    if (rb->pRx >= rb->dataQueue + ( rb->amountOfEntries * rb->sizeOfEntry))
        rb->pRx = rb->dataQueue;
    ++rb->rxPos;
    assert(rb->txPos >= rb->rxPos);
}

void *RingBuffer_GetWritePtr(RingBuffer_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    if (rb->txPos - rb->rxPos < rb->amountOfEntries)
        return (void *)rb->pTx;
    return NULL;
}

void RingBuffer_PopWritePtr(RingBuffer_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    rb->pTx += rb->sizeOfEntry;
    if (rb->pTx >= rb->dataQueue + ( rb->amountOfEntries * rb->sizeOfEntry))
        rb->pTx = rb->dataQueue;
    ++rb->txPos;
    assert(rb->txPos >= rb->rxPos);
}