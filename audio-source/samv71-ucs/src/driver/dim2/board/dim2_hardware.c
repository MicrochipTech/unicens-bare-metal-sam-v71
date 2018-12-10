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

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "samv71.h"
#include "board.h"
#include "dim2_hardware.h"
#include "Console.h"

void enable_mlb_clock(void)
{
    volatile Pmc *pmc = PMC;
    pmc->PMC_PCR = PMC_PCR_EN |
                   PMC_PCR_CMD |
                   PMC_PCR_DIV_PERIPH_DIV_MCK |
                   PMC_PCR_PID(ID_MLB);
    pmc->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN;
    pmc->PMC_PCER1 = PMC_PCER1_PID53;
    pmc->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD;
}

//Warning: Using the MLB pins will disable the virtual UART over USB :-(
void initialize_mlb_pins(void)
{
#define PIN_MLBCLK  {PIO_PB4, PIOB, ID_PIOB, PIO_PERIPH_C, PIO_DEFAULT}
#define PIN_MLBDAT  {PIO_PB5, PIOB, ID_PIOB, PIO_PERIPH_C, PIO_DEFAULT}
#define PIN_MLBSIG  {PIO_PD10, PIOD, ID_PIOD, PIO_PERIPH_D, PIO_DEFAULT}

    static const Pin pinsMlb[] = {PIN_MLBCLK, PIN_MLBDAT, PIN_MLBSIG};
    PIO_Configure(pinsMlb, 3);

    MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;
    MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO5;
}

void enable_mlb_interrupt(void)
{
    NVIC_EnableIRQ(AHB0_INT_IRQn);
    NVIC_EnableIRQ(MLB_INT_IRQn);
}

void disable_mlb_interrupt(void)
{
    NVIC_DisableIRQ(MLB_INT_IRQn);
    NVIC_DisableIRQ(AHB0_INT_IRQn);
}

uint32_t dimcb_io_read(uint32_t  *ptr32)
{
    assert(NULL != ptr32);
    return *ptr32; //No MMU, so it's easy
}

void dimcb_io_write(uint32_t  *ptr32, uint32_t value)
{
    assert(NULL != ptr32);
    *ptr32 = value; //No MMU, so it's easy
}

void dimcb_on_error(uint8_t error_id, const char *error_message)
{
    ConsolePrintf(PRIO_ERROR, RED "dim2-hal error:%d, '%s'" RESETCOLOR"\r\n", error_id, error_message);
}

void mlb_int_handler(void)
{
    irqflags_t flags = cpu_irq_save();

    on_mlb_int_isr();
    cpu_irq_restore(flags);
}

void ahb0_int_handler(void)
{
    irqflags_t flags = cpu_irq_save();

    on_ahb0_int_isr();
    cpu_irq_restore(flags);
}
