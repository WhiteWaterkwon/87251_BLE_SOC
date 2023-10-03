#include <stdio.h>

#include "hwlib/socal/hps.h"                        //ALT_SPTMR0_TMR1EOI_ADDR
#include "hwlib/socal/socal.h"
#include "hwlib/socal/alt_tmr.h"

#include "address_map_arm.h"
#include "interrupt_ID.h"
#include "defines.h"

#include "usart.h"

/* This file:
 * 1. defines exception vectors for the A9 processor
 * 2. provides code that sets the IRQ mode stack, and that dis/enables interrupts
 * 3. provides code that initializes the generic interrupt controller
*/
void HPS_UART0_ISR (void);
void HPS_GPIO1_ISR (void);
void BC5602_IRQ1_ISR(void);

extern volatile int      tick;
extern volatile uint32_t tick625us;
static inline void HPS_timer_ISR(void)
{
    ++tick;                                     // used by main program
    ++tick625us;
    alt_read_word(ALT_SPTMR0_TMR1EOI_ADDR);     // Read timer end of interrupt register to clear the interrupt
}

// Define the IRQ exception handler
void __attribute__ ((interrupt)) __cs3_isr_irq (void)
{
    // Read the ICCIAR from the processor interface 
    int int_ID = *((int *) (MPCORE_GIC_CPUIF + ICCIAR)); 
   
    if      (int_ID == FPGA_5602_IRQ) {             // fpga 5602 IRQ1
        BC5602_IRQ1_ISR();
    }
    else if (int_ID == HPS_UART0_IRQ) {             // check if interrupt is from the HPS UART0
        HPS_UART0_ISR ();
    }
    else if (int_ID == HPS_TIMER0_IRQ) {            // check if interrupt is from the HPS timer
        HPS_timer_ISR();
    }
  //else if (int_ID == HPS_GPIO1_IRQ) {             // HPS_GPIO1
  //    HPS_GPIO1_ISR();
  //}
    else {
//uart_puts("int_ID="); uart_putu16(int_ID); uart_putchar_n('\n');
        while (1);                                    // if unexpected, then stay here
    }

    // Write to the End of Interrupt Register (ICCEOIR)
    *((int *) (MPCORE_GIC_CPUIF + ICCEOIR)) = int_ID;
} 

// Define the remaining exception handlers
void __attribute__ ((interrupt)) __cs3_reset (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_undef (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_swi (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_pabort (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_dabort (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_fiq (void)
{
    while(1);
}

/* 
 * Turn off interrupts in the ARM processor
*/
void disable_A9_interrupts(void)
{
    int status = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

/* 
 * Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack(void)
{
    int stack, mode;
    stack = A9_ONCHIP_END - 7;        // top of A9 onchip memory, aligned to 8 bytes
    /* change processor to IRQ mode with interrupts disabled */
    mode = INT_DISABLE | IRQ_MODE;
    asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
    /* set banked stack pointer */
    asm("mov sp, %[ps]" : : [ps] "r" (stack));

    /* go back to SVC mode before executing subroutine return! */
    mode = INT_DISABLE | SVC_MODE;
    asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}

/* 
 * Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void)
{
    int status = SVC_MODE | INT_ENABLE;
    asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

/* 
 * Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void)
{
    int address;    // used to calculate register addresses

     // configure the HPS timer interrupt
     alt_write_byte(0xFFFED800+HPS_UART0_IRQ,  0x01);   // ICDIPTR  0x01: bit[0]=1 to select target CPU 0
     alt_write_byte(0xFFFED800+HPS_GPIO1_IRQ,  0x01);   // ICDIPTR  0x01: bit[0]=1 to select target CPU 0
     alt_write_byte(0xFFFED800+HPS_TIMER0_IRQ, 0x01);   // ICDIPTR  0x01: bit[0]=1 to select target CPU 0
//    *((int *) 0xFFFED8C0) = 0x00010000;               // ICDIPTR      to target CPU,        0xc0=192,192*1=192,192+2=194:interruptID of HPS_UART0_IRQ    00:high priority
//    *((int *) 0xFFFED8C4) = 0x01000000;               // ICDIPTR      to target CPU,        0xc4=196,196*1=196,196+3=199:interruptID of HPS_TIMER0_IRQ

    *((int *)(0xFFFED100+24)) = 0x000000A4;             // ICDISER    Set-enable bits,        0x18= 24, 24*8=192,192+7=199:interruptID of HPS_TIMER0_IRQ
                                                        //                                                       192+5=197:interruptID of HPS_GPIO1_IRQ
                                                        //                                                       192+2=194:interruptID of HPS_UART0_IRQ
    alt_write_byte(0xFFFED400+FPGA_5602_IRQ,      0x00);// ICDIPR   0x00000000 the highest priority
    alt_write_byte(0xFFFED400+HPS_UART0_IRQ,      0x01);// ICDIPR   0x00000000 the highest priority
    alt_write_byte(0xFFFED400+HPS_TIMER0_IRQ,     0x02);// ICDIPR   0x00000000 the highest priority
    alt_write_byte(0xFFFED400+HPS_GPIO1_IRQ,      0x03);// ICDIPR   0x00000000 the highest priority

  #if 1 // FPGA interrupt
//  alt_write_byte(0xFFFEDC00+(6*4),         0x02|0x55);// ICDICFR  fpga interrupts start at 72, FPGA-to-HPS irq bits 24 through 31 (f2h_irq0[31:24]) can be read from 0xFFDE_DD0C bits [7:0]

    alt_write_byte(0xFFFED800+FPGA_5602_IRQ,      0x01);// ICDIPTR  fpga interrupts start at 72, FPGA-to-HPS irq bits 24 through 31 (f2h_irq0[31:24]) can be read from 0xFFDE_DD0C bits [7:0]
  //alt_write_byte(0xFFFED800+INTERVAL_TIMER_IRQ, 0x01);// ICDIPTR  
  //alt_write_byte(0xFFFED800+KEYS_IRQ,           0x01);// ICDIPTR  
  //*((int *) 0xFFFED848) = 0x00000101;                 // ICDIPTR      to target CPU,        0x48=72,72*1=72,72+0=72:interruptID of FPGA_Interval_Timer
                                                        //                                                   ,72+1=73:interruptID of FPGA_Pushbutton_KEY0123

    alt_write_byte(0xFFFED100+12, 0x01);                // ICDISER    Set-enable bits,             12*8=96,96+0= 96    :interruptID of RFIRQ 5602
  //*((int *) 0xFFFED108) = 0x00000300;                 // ICDISER    Set-enable bits,        0x8=8,8*8=64,64+8= 72    :interruptID of FPGA_Interval_Timer
                                                        //                                                ,64+9= 73    :interruptID of FPGA_Pushbutton_KEY0123
  #endif

      // Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all priorities 
    address = MPCORE_GIC_CPUIF + ICCPMR;
      *((int *) address) = 0xFFFF;

      // Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
    address = MPCORE_GIC_CPUIF + ICCICR;
    *((int *) address) = ENABLE;

    // Configure the Distributor Control Register (ICDDCR) to send pending interrupts to CPUs 
    address = MPCORE_GIC_DIST + ICDDCR;
    *((int *) address) = ENABLE;
}

