/* This files provides address values that exist in the system */

#define BOARD						"DE1-SoC"

/* Memory */
#define DDR_START					0x00000000
#define DDR_END						0x3FFFFFFF
#define A9_ONCHIP_START				0xFFFF0000
#define A9_ONCHIP_END				0xFFFFFFFF
#define SDRAM_START					0xC0000000
#define SDRAM_END					0xC3FFFFFF
#define FPGA_ONCHIP_START			0xC8000000
#define FPGA_ONCHIP_END				0xC8003FFF
#define FPGA_CHAR_START				0xC9000000
#define FPGA_CHAR_END				0xC9001FFF

/* Cyclone V FPGA devices */
//efine LEDR_BASE					0xFF200000
//efine HEX3_HEX0_BASE				0xFF200020
//efine HEX5_HEX4_BASE				0xFF200030
//efine SW_BASE						0xFF200040
//efine KEY_BASE					0xFF200050
//efine JP1_BASE					0xFF200060
//efine JP2_BASE					0xFF200070
//efine PS2_BASE					0xFF200100
//efine PS2_DUAL_BASE				0xFF200108
//efine JTAG_UART_BASE				0xFF201000
//efine JTAG_UART_2_BASE			0xFF201008
//efine IrDA_BASE					0xFF201020
//efine TIMER_BASE					0xFF202000
//efine AV_CONFIG_BASE				0xFF203000
//efine PIXEL_BUF_CTRL_BASE			0xFF203020
//efine CHAR_BUF_CTRL_BASE			0xFF203030
//efine AUDIO_BASE					0xFF203040
//efine VIDEO_IN_BASE			    0xFF203060
//efine EDGE_DETECTION_ROUTER_BASE	0xFF203070
//efine ADC_BASE				    0xFF204000

/* Cyclone V HPS devices */
#define HPS_GPIO1_BASE				0xFF709000
#define HPS_TIMER0_BASE				0xFFC08000
#define HPS_TIMER1_BASE				0xFFC09000
#define HPS_TIMER2_BASE				0xFFD00000
#define HPS_TIMER3_BASE				0xFFD01000
#define FPGA_BRIDGE					0xFFD0501C

/* ARM A9 MPCORE devices */
#define   PERIPH_BASE				0xFFFEC000    // base address of peripheral devices
#define   MPCORE_PRIV_TIMER			0xFFFEC600    // PERIPH_BASE + 0x0600

/* Interrupt controller (GIC) CPU interface(s) */
#define MPCORE_GIC_CPUIF			0xFFFEC100    // PERIPH_BASE + 0x100
#define ICCICR						0x00          // offset to CPU interface control reg
#define ICCPMR						0x04          // offset to interrupt priority mask reg
#define ICCIAR						0x0C          // offset to interrupt acknowledge reg
#define ICCEOIR						0x10          // offset to end of interrupt reg
/* Interrupt controller (GIC) distributor interface(s) */
#define MPCORE_GIC_DIST				0xFFFED000    // PERIPH_BASE + 0x1000
#define ICDDCR						0x00          // offset to distributor control reg
#define ICDISER						0x100         // offset to interrupt set-enable regs
#define ICDICER						0x180         // offset to interrupt clear-enable regs
#define ICDIPTR						0x800         // offset to interrupt processor targets regs
#define ICDICFR						0xC00         // offset to interrupt configuration regs
