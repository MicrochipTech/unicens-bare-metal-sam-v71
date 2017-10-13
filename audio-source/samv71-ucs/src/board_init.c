/*------------------------------------------------------------------------------------------------*/
/* Board Init Component                                                                           */
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


#include "board_init.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      FUNCTION PROTOTYPES                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void TCM_StackInit(void);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      DEFINES AND LOCAL VARIABLES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

Twid twid;
sGmacd gGmacd;
GMacb gGmacb;
sXdmad xdma;

/** GMAC power control pin */
#if !defined(BOARD_GMAC_POWER_ALWAYS_ON)
static const Pin gmacPwrDn[]    = {BOARD_GMAC_PIN_PWRDN};
#endif

/** The PINs for GMAC */
static const Pin gmacPins[]     = {BOARD_GMAC_RUN_PINS};
static const Pin gmacResetPin   = BOARD_GMAC_RESET_PIN;

/** The PINs for TWI*/
static const Pin twiPins[]      = PINS_TWI0;
static const Pin twiClkPin[]   = { {PIO_PA4A_TWCK0, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT} };
/** TWI clock frequency in Hz. */
#define TWCK            400000
/** Slave address of twi_eeprom AT24MAC.*/
#define AT24MAC_SERIAL_NUM_ADD  0x5F
/** Page size of an AT24MAC402 chip (in bytes)*/
#define PAGE_SIZE       16
/** Page numbers of an AT24MAC402 chip */
#define EEPROM_PAGES    16
/** EEPROM Pins definition */
#define BOARD_PINS_TWI_EEPROM PINS_TWI0
/** TWI0 peripheral ID for EEPROM device*/
#define BOARD_ID_TWI_EEPROM   ID_TWIHS0
/** TWI0 base address for EEPROM device */
#define BOARD_BASE_TWI_EEPROM TWIHS0

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void BoardInit()
{
    /* Initialize the SAM system */
    WDT_Disable(WDT);
    #ifdef ENABLE_TCM
    TCM_StackInit();
    #endif

    SCB_EnableICache();
    SCB_EnableDCache();	// Please edit __DCACHE_PRESENT in samv71q21.h to en/disable DCache

    // Configure systick for 1 ms
    if (TimeTick_Configure())
    {
        assert(false);
    }

    LED_Configure(0);
    LED_Configure(1);
    LED_Clear(0);
    LED_Clear(1);

    // enable GMAC interrupts
    NVIC_ClearPendingIRQ(GMAC_IRQn);
    NVIC_EnableIRQ(GMAC_IRQn);

    // Clear I2C bus
    PIO_Configure(twiClkPin, ARRAY_SIZE(twiClkPin));
    for (uint8_t pulses = 0; pulses < 20; pulses++)
    {
        PIO_Set(&twiClkPin[0]);
        for (uint32_t dly = 0; dly < 800; dly++) { /* delay */ }
        PIO_Clear(&twiClkPin[0]);
        for (uint32_t dly = 0; dly < 800; dly++) { /* delay */ }
    }
    PIO_Set(&twiClkPin[0]);

    /* Configure TWI pins. */
    PIO_Configure(twiPins, ARRAY_SIZE(twiPins));
    /* Enable TWI */
    PMC_EnablePeripheral(BOARD_ID_TWI_EEPROM);
    TWI_ConfigureMaster(BOARD_BASE_TWI_EEPROM, TWCK, BOARD_MCK);
    TWID_Initialize(&twid, BOARD_BASE_TWI_EEPROM);
    NVIC_ClearPendingIRQ(TWIHS0_IRQn);
    NVIC_EnableIRQ(TWIHS0_IRQn);

    /* Setup and enable XDMAC */
    XDMAD_Initialize(&xdma, 0);
    NVIC_ClearPendingIRQ(XDMAC_IRQn);
    NVIC_SetPriority(XDMAC_IRQn, 1);
    NVIC_EnableIRQ(XDMAC_IRQn);

    /* Initialize the hardware interface */
    init_gmac(&gGmacd);

    /* Setup interrupts */
    GMACB_Init(&gGmacb, &gGmacd, BOARD_GMAC_PHY_ADDR);
    GMACB_ResetPhy(&gGmacb);

    /* PHY initialize */
    if (!GMACB_InitPhy(&gGmacb, BOARD_MCK, &gmacResetPin, 1, gmacPins, PIO_LISTSIZE(gmacPins))) {
        printf("PHY Initialize ERROR!\n\r");
        assert(false);
    }

    if(!GMACB_PhySetSpeed100(&gGmacb, 1u)) {
        printf("Set Ethernet PHY Speed ERROR!\n\r");
        assert(false);
    }
#ifdef DEBUG
    //Phy takes a while until ready, do not wait for it in release variant
    Wait(5000);
#endif
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                            ISR HOOKS                                 */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/** DMA handler **/
void XDMAC_Handler(void)
{
    XDMAD_Handler(&xdma);
}

/** TWI handler **/
void TWIHS0_Handler(void)
{
    TWID_Handler(&twid) ;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      CALLBACK ERROR HOOKS                            */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void SystemHalt(const char *message)
{
    printf("System halted by '%s'\r\n", message);
    while(1);
}
void NMI_Handler(void)
{
    SystemHalt("NMI_Handler");
}
void HardFault_Handler(void)
{
    SystemHalt("HardFault_Handler");
}
void MemManage_Handler(void)
{
    SystemHalt("MemManage_Handler");
}
void BusFault_Handler(void)
{
    SystemHalt("BusFault_Handler");
}
void UsageFault_Handler(void)
{
    SystemHalt("UsageFault_Handler");
}