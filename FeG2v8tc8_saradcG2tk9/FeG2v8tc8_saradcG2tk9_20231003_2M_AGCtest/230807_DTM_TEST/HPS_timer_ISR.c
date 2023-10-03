#include <stdint.h>
#include <stdbool.h>                         //bool
#include "address_map_arm.h"

#include "hwlib/socal/hps.h"
#include "hwlib/socal/socal.h"
#include "hwlib/socal/alt_tmr.h"

#include "ble_soc.h"

extern volatile int      tick;
       volatile uint32_t tick625us;

/******************************************************************************
 * HPS timer0 interrupt service routine
 *                                                                          
 * This code increments the tick variable, and clears the interrupt
 *****************************************************************************/
/*
void HPS_timer_ISR(void)
{
    ++tick;                                     // used by main program
    ++tick625us;
    alt_read_word(ALT_SPTMR0_TMR1EOI_ADDR);     // Read timer end of interrupt register to clear the interrupt
}
*/

uint32_t tmr625usGetCurrentTicks(void)
{
    return tick625us;
}
bool tmr625usIsExpired(uint32_t offset_ticks, uint32_t Texpire)   //Texpire in unit of 625us
{
    if (((uint32_t)( tick625us + (uint32_t)(0x100000000-offset_ticks)) >= Texpire))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
void delay_unit625us(uint32_t Texpire)
{
    uint32_t offset_ticks;
             offset_ticks = tick625us;//tmr625usGetCurrentTicks()
    while ( tmr625usIsExpired(offset_ticks, Texpire) == 0 )
    {
    }
}
