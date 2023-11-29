/*********************************************************************************************************//**
 * @file    advrxq.c
 * @version $Rev:: 100          $
 * @date    $Date:: 2022-02-22 #$
 * @brief   
 *************************************************************************************************************
 * @attention
 *
 * Firmware Disclaimer Information
 *
 * 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, which is supplied by Holtek Semiconductor Inc., (hereinafter referred to as "HOLTEK") is the
 *    proprietary and confidential intellectual property of HOLTEK, and is protected by copyright law and
 *    other intellectual property laws.
 *
 * 2. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, is confidential information belonging to HOLTEK, and must not be disclosed to any third parties
 *    other than HOLTEK and the customer.
 *
 * 3. The program technical documentation, including the code, is provided "as is" and for customer reference
 *    only. After delivery by HOLTEK, the customer shall use the program technical documentation, including
 *    the code, at their own risk. HOLTEK disclaims any expressed, implied or statutory warranties, including
 *    the warranties of merchantability, satisfactory quality and fitness for a particular purpose.
 *
 * <h2><center>Copyright (C) 2022 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdbool.h>                         //bool
#include <stdint.h>                          //uint8_t

#include "pdma.h"                            //pdma_ram_bleRx_read_payload()
#include "usart.h"                           //uart_puts()
#include "advrxq.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"


/* Private function prototypes -----------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/
//uint8_t test_pattern_i[82] ={0x49,0x00,0xB6,0x00,0x49,0x00,0xB7,0x00,0x47,0xB7,0x48,0xB6,0x4A,0xB6,0x49,0xB5,0x4A,0xB5,0x49,0xB7,0x49,0xB5,0x4A,0xB6,0x4A,0xB5,0x4A,0xB6,0x49,0xB6,0x49,0xB6,0x49,0xB6,0x4A,0xB5,0x4A,0xB5,0x49,0xB6,0x4A,0xB6,0x49,0xB6,0x49,0xB6,0x48,0xB7,0x4A,0xB6,0x4A,0xB6,0x48,0xB6,0x49,0xB6,0x49,0xB6,0x49,0xB5,0x49,0xB6,0x49,0xB7,0x48,0xB6,0x4A,0xB6,0x49,0xB6,0x49,0xB6,0x49,0xB7,0x48,0xB6,0x48,0xB6,0x49,0xB6,0x48,0xB6};
//uint8_t test_pattern_q[82] ={0x00,0x47,0x00,0xB6,0x00,0x47,0x00,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t test_pattern_i[82] ={0x4A,0x00,0xB6,0x00,0x4A,0x00,0xB6,0x00,0x47,0xB7,0x48,0xB6,0x4A,0xB6,0x49,0xB5,0x4A,0xB5,0x49,0xB7,0x49,0xB5,0x4A,0xB6,0x4A,0xB5,0x4A,0xB6,0x49,0xB6,0x49,0xB6,0x49,0xB6,0x4A,0xB5,0x4A,0xB5,0x49,0xB6,0x4A,0xB6,0x49,0xB6,0x49,0xB6,0x48,0xB7,0x4A,0xB6,0x4A,0xB6,0x48,0xB6,0x49,0xB6,0x49,0xB6,0x49,0xB5,0x49,0xB6,0x49,0xB7,0x48,0xB6,0x4A,0xB6,0x49,0xB6,0x49,0xB6,0x49,0xB7,0x48,0xB6,0x48,0xB6,0x49,0xB6,0x48,0xB6};
uint8_t test_pattern_q[82] ={0x00,0x4A,0x00,0xB6,0x00,0x4A,0x00,0xB6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
/* Private types -------------------------------------------------------------------------------------------*/


/* Global functions ----------------------------------------------------------------------------------------*/


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
  advRxQ_nodebuffer_TypeDef AdvRxQBUFF[TOTAL_NUM_LE_ADV_PACKETS_RX];

void advRxQBuff_init(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ADV_PACKETS_RX; i++) {
        advRxQBuff_free( AdvRxQBUFF + i );
    }
}

bool irq1isr_advRxQBuff_isAvailable(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ADV_PACKETS_RX; i++) {
        if( AdvRxQBUFF[i].rrrstate == 0 ) { // 0x00:free, 0x01:booked
            return(1);
        }
    }
    //all used already
            return(0);
}
advRxQ_nodebuffer_TypeDef * irq1isr_advRxQBuff_alloc(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ADV_PACKETS_RX; i++) {
        if( AdvRxQBUFF[i].rrrstate == 0 ) { // 0x00:free, 0x01:booked
            AdvRxQBUFF[i].rrrstate = 1;     // 0x00:free, 0x01:booked
            return(AdvRxQBUFF + i);
        }
    }
    //all used already
            return(0);//NULL:fail
}

int sean_abs(signed char num) {
    if (num < 0) {
        return -num;
    } else {
        return num;
    }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
    advRxQ_TypeDef advRxQ;

unsigned char irq1isr_read_rxfifo_to_advRxQ(uint16_t ramStartAddr, uint16_t rxHeader)
{
    ADV_PDU_HDR_TypeDef  header;
    advRxQ_nodebuffer_TypeDef *pBuff;
    header = (ADV_PDU_HDR_TypeDef)rxHeader;
    if( header.field.length == 0 ) {
        return 0;
    }
        pBuff = irq1isr_advRxQBuff_alloc();
    if( pBuff == 0 ) {
uart_puts("rxfifo_to_advRxQ fail____"); uart_putchar_n('\n');
        return 0;
    }
    /////////
    {
        pBuff->advpdu.header.reg = rxHeader ;
        pdma_ram_bleRx_read_payload(ramStartAddr, pBuff->advpdu.header.field.length, (unsigned char *)&(pBuff->advpdu.payload) );
    }
    /////////
    //push this buffer to advRxQ
    if( RINGBUF_isFull(advRxQ, TOTAL_NUM_ELEMENTS_advRxQ ) ) // full Q
    {
        advRxQBuff_free( pBuff );
        return(0);
    }
    else
    {
        RINGBUF_push( advRxQ, pBuff, TOTAL_NUM_ELEMENTS_advRxQ );
        return pBuff->advpdu.header.field.length;
    }
}

void initial_advRxQ(void)
{
    advRxQBuff_init();
    RINGBUF_init( advRxQ );
}
void process_advRxQ(void)
{
    bool done;
    advRxQ_nodebuffer_TypeDef *pBuff;
    if( RINGBUF_isEmpty( advRxQ ) ) // empty q
    {
        return; //exit empty q
    }
        pBuff = (advRxQ_nodebuffer_TypeDef *)( RINGBUF_peek( advRxQ, TOTAL_NUM_ELEMENTS_advRxQ) );
    /////////////////////////
    {
        #if 1 //print debug
        {
            uint16_t i;
            uart_putchar_n('\n');
            uart_puts("advRxQ ");
	        uart_putu16(pBuff->advpdu.header.reg);
            uart_puts(", ");
          for (i=0; i<(pBuff->advpdu.header.field.length); i++) {
	        uart_putu8(pBuff->advpdu.payload[i]);
            uart_puts(" ");
	      }
            uart_putchar_n('\n');
	    }
	    #endif
        /////////////////////////
            /////////////////////////
            done = 1;
            /////////////////////////
        if (done) {
            advRxQBuff_free( pBuff );
            RINGBUF_incrementPopIndex( advRxQ ); // ridx ++   pop Q
        }
        else {
            //return; //quit since this msg fails
        }
    }
}
#if 0
void  format_with_saturation(uint8_t sample_count, unsigned char *pch )
{
	
        uint8_t mask_H,mask_L,test_H,test_L,overflow_H;
        bool sign_Neg;
        int i;
        unsigned char ctepld[164];
	//long int caculate_e;
	signed char caculate_e;
        unsigned char caculate_i,caculate_q;
        uint8_t shift_bits;
        
        
        
        mask_H = 0x3F;
	mask_L = 0x80;
	
	test_H = pch[29];
	test_L = pch[28];
	sign_Neg = test_H&0x80;
	shift_bits = 1;	
	if(sign_Neg)
	{
             ctepld[14]= (((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0)))|(sign_Neg<<7);	  		
	}
	else
	{
               ctepld[14]= ((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0));	     	
	}	
	test_H = pch[31];
	test_L = pch[30];
	if(sign_Neg)
	{
             ctepld[15]= (((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0)))|(sign_Neg<<7);	  		
	}
	else
	{
               ctepld[15]= ((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0));	     	
	}
	caculate_i = ctepld[14];
	caculate_q = ctepld[15];
	//caculate_e = ((signed char)ctepld[14] * (signed char)ctepld[14])+ ((signed char)ctepld[15] * (signed char)ctepld[15]);
	caculate_e = (signed char)ctepld[14];
	caculate_e = sean_abs(caculate_e);
	if(caculate_e > 128){shift_bits = 0;}
	else if(caculate_e > 64){shift_bits=0;}
	else if(caculate_e > 32){shift_bits=0;}
	else if(caculate_e > 16){shift_bits=0;}	
	else if(caculate_e > 8) {shift_bits=1;}	
	else if(caculate_e <= 8){shift_bits=2;}	
	
	mask_H = 0x3F>>shift_bits;
	mask_L = 0xFF<<(7-shift_bits);
	for(i=0;i<sample_count;i++)
	{			
				test_H= pch[i*4+1];
				test_L= pch[i*4];
				
				sign_Neg = test_H&0x80;
				//overflow_H = (test_H&0x3F)&(0x3F<<(6-shift_bits));
		
						if(sign_Neg)
						{
							if(shift_bits==0)
							{
							 overflow_H = 0;
							}
							else 
							{						
							 if((test_H&0x3F)<((0x3F<<(6-shift_bits))&0x3F))
							 {
							   overflow_H = 1;
							 }
							 else
							 {
							   overflow_H = 0;
							 }
							}
							if(overflow_H)
							{
								ctepld[i*2] = 0x80;
		 					}
							else
							{
								ctepld[i*2]= (((test_H&mask_H)<<(shift_bits+1))+((test_L&mask_L)>>(7-shift_bits)))|(sign_Neg<<7);
							}
						}
						else
						{
							overflow_H = (test_H&0x3F)&(0x3F<<(6-shift_bits));							
							if(overflow_H)
							{
								ctepld[i*2]= 0x7F;
							}
							else
							{
								ctepld[i*2]= ((test_H&mask_H)<<(shift_bits+1))+((test_L&mask_L)>>(7-shift_bits));
							}
						}
						
	}					
	for(i=0;i<sample_count;i++)
	{
			  test_H= pch[i*4+3];
			  test_L= pch[i*4+2];
			  sign_Neg = test_H&0x80;
			 // overflow_H = (test_H&0x3F)&(0x3F<<(6-shift_bits));

						if(sign_Neg)
						{	
							if(shift_bits==0)
							{
							 overflow_H = 0;
							}
							else 
							{						
							 if((test_H&0x3F)<((0x3F<<(6-shift_bits))&0x3F))
							 {
							   overflow_H = 1;
							 }
							 else
							 {
							   overflow_H = 0;
							 }
							} 
							//overflow_H = (test_H&0x3F)<(0x3F<<(6-shift_bits));
							if(overflow_H ==1)
							{
								//ctepld[i+sample_count]= 0x7F|(sign_Neg<<7);
								ctepld[i*2+1] = 0x80;
							}
							else
							{
								ctepld[i*2+1]= (((test_H&mask_H)<<(shift_bits+1))+((test_L&mask_L)>>(7-shift_bits)))|(sign_Neg<< 7);
							}
						}
						else
						{
							overflow_H = (test_H&0x3F)&(0x3F<<(6-shift_bits));	
							if(overflow_H)
							{
								ctepld[i*2+1]= 0x7F;
							}
							else
							{
								ctepld[i*2+1]= ((test_H&mask_H)<<(shift_bits+1))+((test_L&mask_L)>>(7-shift_bits));
							}
						}
						
	}
	
	
	for(i=0;i<(sample_count*2);i++){pch[i]=ctepld[i];}
	
	/*
	pch[0]=0x01+(~(ctepld[15]));
	pch[2]=0;
	pch[4]=ctepld[15];
	pch[6]=0;
	pch[8]=0x01+(~(ctepld[15]));
	pch[10]=0;
	pch[12]=ctepld[15];
	pch[14]=0;
	///
	pch[1]  =0 ;
	pch[3]=0x01+(~(ctepld[15]));
	pch[5]=0;
	pch[7]=ctepld[15];
	pch[9]=0;
	pch[11]=0x01+(~(ctepld[15]));
	pch[13]=0;
	pch[15]=ctepld[15];
	*/	
	pch[0]=0;
	pch[2]=0x01+(~(ctepld[14]));
	pch[4]=0;
	pch[6]=ctepld[14];
	pch[8]=0;
	pch[10]=0x01+(~(ctepld[14]));
	pch[12]=0;
	pch[14]=ctepld[14];
	///
	pch[1]=ctepld[14];
	pch[3]=0;
	pch[5]=0x01+(~(ctepld[14]));
	pch[7]=0;
	pch[9]=ctepld[14];
	pch[11]=0;
	pch[13]=0x01+(~(ctepld[14]));
	pch[15]=0;	
	
	/*
	pch[0]=shift_bits;
	pch[1]=caculate_e;
	//pch[2]=caculate_e>>8;
	pch[2]=0xCC;
	pch[3]= caculate_i;
	pch[4]= caculate_q;
	pch[5]= 0xEE;
	*/
	/*
	for(i=16;i<(sample_count*2);i+=2)
	{
	   //pch[i]= ctepld[i];
	  // pch[i+1] = ctepld[sample_count+i+1];
	  // pch[i+1] = 0x00;
	}*/
	
	/*
	for(i=16;i<(sample_count*2);i+=2)
	{
	   pch[i]= 0x80;
	   pch[i+1] = 0x80;
	}*/
	/*
	for(i=8;i<(sample_count);i++)
	{
		if(pch[i] ==0x00){;}
		else if(pch[sample_count+i] ==0x00){;}		
		else{
			if((pch[i]&0x80)== 0x80)
			{
				if((pch[sample_count+i]&0x80) == 0x00)
				{				
					pch[i] =0x80;
					pch[sample_count+i]=0x80;
				}
			}
			else if((pch[i]&0x80) == 0x00)
			{
				if((pch[sample_count+i]&0x80) == 0x80) 
				{				
					pch[i]=0x80;
					pch[sample_count+i]=0x80;
				}
			}
		}
		//pch[i] =0x80;
		//pch[sample_count+i]=0x80;		
	}
	*/
	//for(i=0;i<(sample_count);i++){pch[i]=test_pattern_i[i];}
	//for(i=sample_count+8;i<(sample_count*2);i++){pch[i]=0x00;}
	
} 
#endif

void  format_with_saturation(uint8_t sample_count, unsigned char *pch )
{
	
        uint8_t mask_H,mask_L,test_H,test_L,overflow_H;
        bool sign_Neg;
        int i;
        unsigned char ctepld[164];
        //unsigned char ctepld[1024];
	//long int caculate_e;
	signed char caculate_e;
        unsigned char caculate_i,caculate_q;
        unsigned char shift_bits;

       // ctepld[164]={0xFF,	0xFF,	0x4F,	0xFA,	0x0,	0xAF,	0xAF,	0x1,	0x3,	0x4F,	0x4F,	0xFC,	0xFA,	0xAF,	0xAF,	0xFE,	0xFA,	0x4F,	0x7,	0x8E,	0x0D,	0x4F,	0xFF,	0xAF,	0x0,	0x4F,	0xF8,	0xD8,	0xFB,	0x4F,	0x0,	0xAF,	0xFF,	0x4F,	0xF7,	0xED,	0xF0,	0x4E,	0x0,	0xAF,	0x0,	0x4F,	0x5,	0x8E,	0x5,	0x4F,	0xF7,	0xAF,	0x6,	0x4F,	0xF9,	0xD8,	0xF7,	0x4F,	0x1,	0xAF,	0xF8,	0x4F,	0xF7,	0xED,	0xF4,	0x4F,	0x0,	0xAF,	0x0,	0x4F,	0x0D,	0x8F,	0x0,	0x4F,	0xFA,	0xAF,	0xFE,	0x4F,	0xFA,	0xD7,	0xF9,	0x4F,	0x3,	0xAF,	0xFE,	0x4F,	0xF6,	0xEE,	0xF7,	0x4F,	0x0,	0xAF,	0x1,	0x4F,	0x4,	0x8E,	0x9,	0x4F,	0x0,	0xAF,	0xFD,	0x4F,	0xFC,	0xD7,	0xFA,	0x4F,	0x3,	0xAF,	0xFB,	0x4F,	0xF7,	0xED,	0xF5,	0x4F,	0x1,	0xAF,	0x0,	0x4F,	0x0C,	0x8F,	0x0A,	0x4F,	0xFE,	0xAF,	0xFE,	0x4F,	0xFB,	0xD7,	0xFA,	0x4F,	0xFE,	0xAF,	0xFE,	0x4F,	0xF7,	0xED,	0xF7,	0x4F,	0x6,	0xAF,	0x0,	0x4F,	0xFC,	0x8E,	0x0A,	0x4F,	0xFF,	0xAF,	0xFD,	0x4F,	0xF9,	0xD8,	0xF5,	0x4F,	0x1,	0xAF,	0x1,	0x4F,	0xF6,	0xED,	0xF2,	0x4F,	0x0,	0xAF,	0xFE,	0x4F,	0x0C,	0x8F};

        //shift_bits = 1;
        /*  // 8byte
        mask_H = 0x7F;
	mask_L = 0x80;
	
	test_H = pch[29];
	test_L = pch[28];
	sign_Neg = test_H&0x80;	
	
	if(sign_Neg)
	{
             ctepld[14]= (((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0)))|(sign_Neg<<7);	  		
	}
	else
	{
               ctepld[14]= ((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0));	     	
	}	
	test_H = pch[31];
	test_L = pch[30];
	if(sign_Neg)
	{
             ctepld[15]= (((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0)))|(sign_Neg<<7);	  		
	}
	else
	{
               ctepld[15]= ((test_H&mask_H)<<(0+1))+((test_L&mask_L)>>(7-0));	     	
	}
	*/
	shift_bits = 0;
	//shift_bits = 1;
	mask_H = 0x7F>>shift_bits;
	mask_L = 0xFF<<(8-shift_bits);
	for(i=0;i<sample_count;i++)
	{			
				test_H= pch[i*4+1];
				test_L= pch[i*4];
				
				sign_Neg = test_H&0x80;		
						if(sign_Neg)
						{
							if(shift_bits==0)
							{
							 overflow_H = 0;
							}
							else 
							{						
							 if((test_H&0x7F)<((0x7F<<(7-shift_bits))&0x7F))
							 {
							   overflow_H = 1;
							 }
							 else
							 {
							   overflow_H = 0;
							 }
							}
							if(overflow_H)
							{
								ctepld[i*2] = 0x80;
		 					}
							else
							{
								//ctepld[i*2]= (((test_H&mask_H)<<(shift_bits))+((test_L&mask_L)>>(8-shift_bits)))|(sign_Neg<<7);
								ctepld[i*2]= (((test_H&mask_H)<<(shift_bits))+((test_L&mask_L)>>(8-shift_bits)))|0x80;
							}
						}
						else
						{
							overflow_H = (test_H&0x7F)&(0x7F<<(7-shift_bits));							
							if(overflow_H)
							{
								ctepld[i*2]= 0x7F;
							}
							else
							{
								ctepld[i*2]= ((test_H&mask_H)<<(shift_bits))+((test_L&mask_L)>>(8-shift_bits));
							}
						}
						
	}					
	for(i=0;i<sample_count;i++)
	{
			  test_H= pch[i*4+3];
			  test_L= pch[i*4+2];
			  sign_Neg = test_H&0x80;
						if(sign_Neg)
						{	
							if(shift_bits==0)
							{
							 overflow_H = 0;
							}
							else 
							{						
							 if((test_H&0x7F)<((0x7F<<(7-shift_bits))&0x7F))
							 {
							   overflow_H = 1;
							 }
							 else
							 {
							   overflow_H = 0;
							 }
							} 
							if(overflow_H ==1)
							{
								ctepld[i*2+1] = 0x80;
							}
							else
							{
								ctepld[i*2+1]= (((test_H&mask_H)<<(shift_bits))+((test_L&mask_L)>>(8-shift_bits)))|0x80;
							}
						}
						else
						{
							overflow_H = (test_H&0x7F)&(0x7F<<(7-shift_bits));	
							if(overflow_H)
							{
								ctepld[i*2+1]= 0x7F;
							}
							else
							{
								ctepld[i*2+1]= ((test_H&mask_H)<<(shift_bits))+((test_L&mask_L)>>(8-shift_bits));
							}
						}
						
	}
	
	
	for(i=0;i<(sample_count*2);i++){pch[i]=ctepld[i];}
	

	
	/*
	pch[0]=0;
	pch[2]=0x01+(~(ctepld[14]));
	pch[4]=0;
	pch[6]=ctepld[14];
	pch[8]=0;
	pch[10]=0x01+(~(ctepld[14]));
	pch[12]=0;
	pch[14]=ctepld[14];
	///
	pch[1]=ctepld[14];
	pch[3]=0;
	pch[5]=0x01+(~(ctepld[14]));
	pch[7]=0;
	pch[9]=ctepld[14];
	pch[11]=0;
	pch[13]=0x01+(~(ctepld[14]));
	pch[15]=0;	
	*/
	#if 0
	//pch[0]=0x4F;
	pch[2]=0x00;
	//pch[4]=0xAF;
	pch[6]=0x00;
	//pch[8]=0x4F;
	pch[10]=0x00;
	//pch[12]=0xAF;
	pch[14]=0x00;
	///
	pch[1]=0x00;
	//pch[3]=0xAF;
	pch[5]=0x00;
	//pch[7]=0x4F;
	pch[9]=0x00;
	//pch[11]=0xAF;
	pch[13]=0x00;
	//pch[15]=0x4F;	
	
	for(i=16;i<(sample_count*2);i+=24)
	{
	  pch[i]  =0x00;
	 // pch[i+1]=0x8E;	//A1
	  pch[i+2]=0x00;	
	 // pch[i+3]=0x4F;
	  pch[i+4]=0x00;
	 // pch[i+5]=0xAF;
	  pch[i+6]=0x00;
	 // pch[i+7]=0x4F;
	  
	  pch[i+8] =0x00;
	 // pch[i+9] =0xD7;	//A2
	  pch[i+10]=0x00;	
	  //pch[i+11]=0x4F;
	  pch[i+12]=0x00;
	 // pch[i+13]=0xAF;
	  pch[i+14]=0x00;
	 // pch[i+15]=0x4F;	  

	  pch[i+16]=0x00;
	 // pch[i+17]=0xEE;	//A3
	  pch[i+18]=0x00;	
	 // pch[i+19]=0x4F;
	  pch[i+20]=0x00;
	 // pch[i+21]=0xAF;
	  pch[i+22]=0x00;
	 // pch[i+23]=0x4F;	  
	  
	}
	#endif
	//for(i=0;i<(sample_count*2);i++){pch[i]=test_pattern_iq[i];}
	
 
	
} 
void read_ctepld_and_formatting (uint8_t cte_sample_count,unsigned char* pch)
{

	unsigned char ctepld[328];
	int i;


	//pdma_ch2_bleRx_read_cte_payload(0x0250,(cte_sample_count*4),ctepld+0);
	pdma_ram_bleRx_read_payload(0x0E00,(cte_sample_count*4),ctepld+0);
	//for(i=0;i<(cte_sample_count*4);i++){uart_putchar(ctepld[i]);}
	format_with_saturation(cte_sample_count,ctepld+0);
	
	for(i=0;i<(cte_sample_count*2);i++){pch[i]=ctepld[i];}	
	//ht_memory_copy(pch+0, ctepld+0, sizeof 328);
	
}


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/

