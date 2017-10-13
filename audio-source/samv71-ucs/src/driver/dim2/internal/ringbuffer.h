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

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t dataQueue;
    volatile uint32_t pRx;
    volatile uint32_t pTx;
    volatile uint32_t amountOfEntries;
    volatile uint32_t sizeOfEntry;
    volatile uint32_t rxPos;
    volatile uint32_t txPos;
} RingBuffer_t;

/*----------------------------------------------------------*/
/*! \brief Initializes the given RingBuffer structure
* \note This function must be called before any other functions of this component.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
* \param amountOfEntries - How many entries can be stored in the ring buffer.
* \param workingBuffer - Memory area which is exactly (amountOfEntries * sizeOfEntry) bytes.
*/
/*----------------------------------------------------------*/
void RingBuffer_Init(RingBuffer_t *rb, uint16_t amountOfEntries,
                     uint32_t sizeOfEntry, void *workingBuffer);

/*----------------------------------------------------------*/
/*! \brief Deinitializes the given RingBuffer structure
* \note After this function, all functions, except of RingBuffer_Init, must not be called.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
*/
/*----------------------------------------------------------*/
void RingBuffer_Deinit(RingBuffer_t *rb);

/*----------------------------------------------------------*/
/*! \brief Gets the amount of entries stored.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
* \return The amount of filled RX entries, which may be accessed via RingBuffer_GetReadPtrPos.
*/
/*----------------------------------------------------------*/
uint32_t RingBuffer_GetReadElementCount(RingBuffer_t *rb);

/*----------------------------------------------------------*/
/*! \brief Gets the head data from the ring buffer in order to read, if available.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
* \return Pointer to the oldest enqueued void structure, if data is available, NULL otherwise.
*/
/*----------------------------------------------------------*/
void *RingBuffer_GetReadPtr(RingBuffer_t *rb);

/*----------------------------------------------------------*/
/*! \brief Gets the head data from the ring buffer in order to read, if available.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
* \param pos - The position to read, starting with 0 for the oldest entry. Use RingBuffer_GetReadElementCount to get maximum pos count (max -1).
* \return Pointer to the enqueued void structure on the given position, if data is available, NULL otherwise.
*/
/*----------------------------------------------------------*/
void *RingBuffer_GetReadPtrPos(RingBuffer_t *rb, uint32_t pos);

/*----------------------------------------------------------*/
/*! \brief Marks the oldest available entry as invalid for reading, so it can be reused by TX functions.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
*/
/*----------------------------------------------------------*/
void RingBuffer_PopReadPtr(RingBuffer_t *rb);

/*----------------------------------------------------------*/
/*! \brief Gets the head data from the ring buffer in order to write, if available.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
* \return Pointer to the a free void structure, so user can fill data into.
*/
/*----------------------------------------------------------*/
void *RingBuffer_GetWritePtr(RingBuffer_t *rb);

/*----------------------------------------------------------*/
/*! \brief Marks the packet filled by RingBuffer_GetWritePtr as ready to read.
* \note After this call, the structure, got from RingBuffer_GetWritePtr, must not be written anymore.
* \param rb - Pointer to the RingBuffer_t structure, must not be NULL.
*/
/*----------------------------------------------------------*/
void RingBuffer_PopWritePtr(RingBuffer_t *rb);

#endif /* RINGBUFFER_H_ */