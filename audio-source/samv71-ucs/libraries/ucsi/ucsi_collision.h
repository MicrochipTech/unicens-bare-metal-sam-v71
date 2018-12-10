/*------------------------------------------------------------------------------------------------*/
/* UNICENS Node and MAC Address Collision Solver Component                                        */
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
#ifndef UCSI_COLLISION_H_
#define UCSI_COLLISION_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ucs_api.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                            Public API                                */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Initializes the collision resolver module
 * \note Do not use any other function, before calling this method.
 *
 * \param pPriv - private data section of this instance
 */
void UCSICollision_Init(void);

/**
 * \brief Sets any pointer as parameter of the callback function UCSICollision_CB_OnProgramIdentString.
 *
 * \param userPtr - Any pointer allowed
 */
void UCSICollision_SetUserPtr(void *userPtr);

/**
 * \brief Sets the expected amount of network nodes in the ring (MPR). 
 *        Programming will only take place, if expected node count is like the found node count.
 * \note This value is given by the user
 * \note Values over 64 will be silently ignored
 *
 * \param nodeCount - private data section of this instance
 */
void UCSICollision_SetExpectedNodeCount(uint8_t nodeCount);

/**
 * \brief Sets the found amount of network nodes in the ring (MPR). 
 *        Programming will only take place, if expected node count is like the found node count.
 * \note This value is given by the network
 * \note Values over 64 will be silently ignored
 *
 * \param nodeCount - private data section of this instance
 */
void UCSICollision_SetFoundNodeCount(uint8_t nodeCount);

/**
 * \brief Stores a found network node (by the UNICENS manager) into a lookup table.
 *
 * \param signature - The signature of the node (containing at least valid node address and MAC address)
 * \param collisionDetected - true, if UNICENS manager detected state which is not normal. false, node is valid
 */
void UCSICollision_StoreSignature(const Ucs_Signature_t *signature, bool collisionDetected);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                        CALLBACK SECTION                              */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Callback when the implementer needs to program the INIC
 *
 * \param signature - The signature of the node to be programmed
 * \param newIdentString - The data to be programmed
 * \param userPtr - The pointer given by UCSICollision_SetUserPtr function, otherwise NULL.
 */
extern void UCSICollision_CB_OnProgramIdentString(const Ucs_Signature_t *signature, 
    const Ucs_IdentString_t *newIdentString, void *userPtr);

/**
 * \brief Callback when the implementer needs to do final cleanup after flashing
 *
 * \param userPtr - The pointer given by UCSICollision_SetUserPtr function, otherwise NULL.
 */
extern void UCSICollision_CB_OnProgramDone(void *userPtr);

/**
 * \brief Callback to inform the implementer that there was no flashing required. The collision resolving finished without any changes.
 *
 * \param userPtr - The pointer given by UCSICollision_SetUserPtr function, otherwise NULL.
 */
extern void UCSICollision_CB_FinishedWithoutChanges(void *userPtr);

#endif /* UCSI_COLLISION_H_ */