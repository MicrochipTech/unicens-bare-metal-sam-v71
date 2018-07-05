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

#ifndef DIM2_LLD_H_
#define DIM2_LLD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ///MOST Control Channel (ADS, AMS)
    DIM2LLD_ChannelType_Control,
    ///MOST Async Channel (MEP, MAMAC, MHP)
    DIM2LLD_ChannelType_Async,
    ///Synchronous channel (PCM audio)
    DIM2LLD_ChannelType_Sync,
    ///Isochronous channel (TS video/audio multiplex)
    DIM2LLD_ChannelType_Isoc,
    ///Don't use BOUNDARY (internal use)
    DIM2LLD_ChannelType_BOUNDARY
} DIM2LLD_ChannelType_t;

typedef enum {
    ///From EHC to MOST
    DIM2LLD_ChannelDirection_TX,
    ///From MOST to EHC
    DIM2LLD_ChannelDirection_RX,
    ///Don't use BOUNDARY (internal use)
    DIM2LLD_ChannelDirection_BOUNDARY
} DIM2LLD_ChannelDirection_t;

/** \brief Initializes the DIM Low Level Driver
* \return true, if the module could be initialized, false otherwise.
*/
bool DIM2LLD_Init(void);

/** \brief Setup a communication channel
* \param cType - The data type which shall be used for this channel
* \param dir - The direction for this unidirectional channel
* \param instance - For Isoc or Sync channels multiple instances may be used, starting with 0 for the first instance. For Control and Async there is only instance per direction allowed.
* \param channelAddress - The MLB channel address to use. This must be an even value!
* \param bufferSize - The maximum amount of bytes, which may be used by the DIM2 module to buffer data
* \param subSize - This value is only used for Sync and Isoc data types (you may use 0 for Control and Async). It sets the amount of bytes of the smallest data chunk (4 Byte of 16Bit Stereo, 188 Byte for TS).
* \param numberOfBuffers - The maximum amount of messages which is stored in LLD driver (DIM2 uses Ping/Pong Buffer).
* \param bufferOffset - If non zero, the given amount of bytes will be appended to the specific buffer. For RX, this area can be filled for example with header data and passed to different software stacks (TCP/IP e.g.). For TX this value will be ignored.
* \return true, if the channel could be initialized, false otherwise.
*/
bool DIM2LLD_SetupChannel(DIM2LLD_ChannelType_t cType,
                          DIM2LLD_ChannelDirection_t dir, uint8_t instance, uint16_t channelAddress,
                          uint16_t bufferSize, uint16_t subSize, uint16_t numberOfBuffers, uint16_t bufferOffset);


/** \brief Deinitializes the DIM Low Level Driver
*
*/
void DIM2LLD_Deinit(void);


/** \brief Must be called cyclic from task context
*
*/
void DIM2LLD_Service(void);


/** \brief Checks if the MLB and INIC have reached locked state.
*
* \return true, if there is a lock, false otherwise.
*/
bool DIM2LLD_IsMlbLocked(void);


/** \brief Returns the amount of available data buffers (max. "numberOfBuffers" passed with DIM2LLD_SetupChannel), which can be returned by calling DIM2LLD_GetRxData.
* \note This is thought to get health informations of the system (debugging).
* \param cType - The data type which shall be used for this channel
* \param dir - The direction for this unidirectional channel
* \param instance - For Isoc or Sync channels multiple instances may be used, starting with 0 for the first instance. For Control and Async there is only instance per direction allowed.
* \return true, if there is a lock, false otherwise.
*/
uint32_t DIM2LLD_GetQueueElementCount(DIM2LLD_ChannelType_t cType, DIM2LLD_ChannelDirection_t dir, uint8_t instance);


/** \brief Retrieves received data from the given channel, if available.
* \note The payload passed by pBuffer stays valid until the function DIM2LLD_ReleaseRxData is called.
* \param cType - The data type which shall be used for this channel
* \param dir - The direction for this unidirectional channel
* \param instance - For Isoc or Sync channels multiple instances may be used, starting with 0 for the first instance. For Control and Async there is only instance per direction allowed.
* \param pos - The position to read, starting with 0 for the oldest entry. Use DIM2LLD_GetQueueElementCount to get maximum pos count (max -1).
* \param pBuffer - This function will deliver a pointer to the data. It may be used for further processing, don't forget to call DIM2LLD_ReleaseRxData after wise! If there is no data available, this pointer will be set to NULL.
* \param pOffset - To this pointer the offset value will be written. This must be exactly the same value, as the one given with DIM2LLD_SetupChannel, Parameter bufferOffset. The given buffer is extended by this size. 
*                  The user may write into the first pOffset bytes, without destroying any informations. The received data starts after wise. May be left NULL.
* \param  pPacketCounter - To this pointer a unique packet counter will be written. The application can identify this buffer by this value in order to mark it as processed. May be left NULL.
* \return Returns the amount of bytes which can be accessed by the pBuffer.
*/
uint16_t DIM2LLD_GetRxData(DIM2LLD_ChannelType_t cType, DIM2LLD_ChannelDirection_t dir, 
                           uint8_t instance, uint32_t pos, const uint8_t **pBuffer, uint16_t *pOffset, uint8_t *pPacketCounter);


/** \brief Releases the passed data from the DIM2LLD_GetRxData function call. Call DIM2LLD_ReleaseRxData only in case you received valid data from DIM2LLD_GetRxData (Not if returned NULL).
* \param cType - The data type which shall be used for this channel
* \param dir - The direction for this unidirectional channel
* \param instance - For Isoc or Sync channels multiple instances may be used, starting with 0 for the first instance. For Control and Async there is only instance per direction allowed.
*/
void DIM2LLD_ReleaseRxData(DIM2LLD_ChannelType_t cType,
                           DIM2LLD_ChannelDirection_t dir, uint8_t instance);


/** \brief Gives a pointer to a LLD buffer, if available. The user may fill the buffer asynchronously. After wise call DIM2LLD_SendTxData to finally send the data.
* \note The payload passed by pBuffer stays valid until the function DIM2LLD_SendTxData is called.
* \param cType - The data type which shall be used for this channel
* \param dir - The direction for this unidirectional channel
* \param instance - For Isoc or Sync channels multiple instances may be used, starting with 0 for the first instance. For Control and Async there is only instance per direction allowed.
* \param pBuffer - This function will deliver a pointer an empty buffer. It may be used for asynchronous filling with data. If there is no buffers free in the LLD module, this pointer is NULL and the return value is 0.
* \return Returns the amount of bytes which can be filled into pBuffer. ==> Max buffer size
*/
uint16_t DIM2LLD_GetTxData(DIM2LLD_ChannelType_t cType,
                           DIM2LLD_ChannelDirection_t dir, uint8_t instance, uint8_t **pBuffer);


/** \brief Finally sends the passed data from the DIM2LLD_GetTxData function call. Call DIM2LLD_SendTxData only in case you got valid data from DIM2LLD_GetTxData (Not if returned NULL).
* \param cType - The data type which shall be used for this channel
* \param dir - The direction for this unidirectional channel
* \param instance - For Isoc or Sync channels multiple instances may be used, starting with 0 for the first instance. For Control and Async there is only instance per direction allowed.
* \param payloadLength - The length of the data stored in pBuffer returned by DIM2LLD_GetTxData. Make sure that the length is less or equal to the return val of DIM2LLD_GetTxData!
*/
void DIM2LLD_SendTxData(DIM2LLD_ChannelType_t cType,
                        DIM2LLD_ChannelDirection_t dir, uint8_t instance, uint32_t payloadLength);

#ifdef __cplusplus
}
#endif

#endif /* DIM2_LLD_H_ */