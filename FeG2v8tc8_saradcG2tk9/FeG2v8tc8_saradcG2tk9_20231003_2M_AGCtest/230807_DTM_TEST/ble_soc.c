/*
*********************************************************************************
* Project    : BLE_SOC 
* Version    : 1.0
* Date       : 2021/10/12
* Author     : Jack Li
* Description: Software Test for BLE SOC Design
*********************************************************************************
* Revision History :
*  V1.0, 2021/10/12: Initial for BLE SOC Design
*********************************************************************************
*/

//===============================================================================
// Included Files
//===============================================================================
#include <stdio.h>
#include <stdint.h>

//===============================================================================
// FPGA to HPS IRQ Command
//===============================================================================
// f2h_irq0[23: 0] can be read from 0xFFFEDD08 bits [31:8]
// f2h_irq0[31:24] can be read from 0xFFFEDD0C bits [ 7:0]
// f2h_irq1[23: 0] can be read from 0xFFFEDD0C bits [31:8] 
// f2h_irq1[31:24] can be read from 0xFFFEDD10 bits [ 7:0]
//===============================================================================
uint8_t IRQ_RD08(uint32_t ADDRESS){

  RF_WTRB (0x1000, 0x0);
  uint32_t ADDR = IRQ_ADDRESS + ADDRESS;
  uint8_t  DATA = ( *((volatile uint32_t *) ADDR) );
  return DATA;
}

//===============================================================================
// PDMA Initialization Command
//===============================================================================
void PDMA_INIT(uint8_t PDMA_EN){

  printf ("PDMA Status : ");

  PDMA_WT08 (0x3000, PDMA_EN);
  if      (PDMA_EN == 0x01) { printf ("ON \n");  }
  else if (PDMA_EN == 0x00) { printf ("OFF \n"); }
  else                      { printf ("PDMA_INIT Error !! \n"); 
                              PDMA_WT08 (0x3000, 0x00); }
}

//===============================================================================
// PDMA One Channel (CH2) Data Transfer Command
//===============================================================================
void PDMA_CH2_DT(uint32_t PDMA_IRQ_CF,
                 uint32_t PDMA_SRC_ADDR,   uint32_t PDMA_DES_ADDR, 
                 uint32_t PDMA_TRANS_SIZE, uint32_t PDMA_CTRL)
{ 
  printf ("PDMA CH2 Transfer : \n");
  
  PDMA_INIT (0x01);
  PDMA_WT32 (0x0130, PDMA_IRQ_CF);
  PDMA_WT32 (0x0034, PDMA_SRC_ADDR);
  PDMA_WT32 (0x0038, PDMA_DES_ADDR);
  PDMA_WT32 (0x0040, PDMA_TRANS_SIZE); 
  PDMA_WT32 (0x0030, PDMA_CTRL); 
  PDMA_INIT (0x00);
}

//===============================================================================
// Data Comparison
//===============================================================================
// 1 DATA_NUM = 1 bytes
//===============================================================================
void DATA_CMP(uint8_t TYPE, uint32_t ADDR_0, uint32_t ADDR_1, uint32_t DATA_NUM){
   
  int i = 0, flag_ng = 0;
  uint8_t rdata_0 = 0, rdata_1 = 0;
  
  if      (TYPE == 0x00) { printf ("RF Data Comparison : ");   }
  else if (TYPE == 0x01) { printf ("RAM Data Comparison : ");  }
  else if (TYPE == 0x02) { printf ("RF VS RAM Comparison : "); }
  else                   { printf ("DATA_CMP Type Error !! "); }       

  for (i = 0; i < DATA_NUM; i++){
    
    if      (TYPE == 0x00) {
      rdata_0 = RF_RD08 (ADDR_0 + i);
      rdata_1 = RF_RD08 (ADDR_1 + i);
	}
    else if (TYPE == 0x01) {
      rdata_0 = RAM_RD08 (ADDR_0 + i);
      rdata_1 = RAM_RD08 (ADDR_1 + i);
	}
    else if (TYPE == 0x02) {
      rdata_0 = RF_RD08  (ADDR_0 + i);
      rdata_1 = RAM_RD08 (ADDR_1 + i);
	}
    else { break; }	
    
	if (rdata_0 != rdata_1) { flag_ng ++; }
  }
  if ((TYPE == 0x00) || (TYPE == 0x01) || (TYPE == 0x02)){
    if (flag_ng >= 1) { printf ("Inconsistent \n"); }
    else              { printf ("Consistent \n");   }
  }
}

//===============================================================================
// HPS Read Data Command
//===============================================================================
// 1 DATA_NUM = 8 bytes
//===============================================================================
void HPS_RDATA(uint8_t TYPE, uint32_t ADDR, uint32_t DATA_NUM){

  int i = 0;
  int addr_0 = 0, addr_1 = 0, addr_2 = 0, addr_3 = 0;
  int data_0 = 0, data_1 = 0, data_2 = 0, data_3 = 0;  

  if      (TYPE == 0x00) { printf ("HPS Read Data From RF : \n\n");  }
  else if (TYPE == 0x01) { printf ("HPS Read Data From RAM : \n\n"); }
  else                   { printf ("HPS_RDATA Type Error !!");       }   

  for (i = 0; i < DATA_NUM; i++){
 
    addr_0 = (ADDR + 8*i+0);
	addr_1 = (ADDR + 8*i+1);
	addr_2 = (ADDR + 8*i+2);
	addr_3 = (ADDR + 8*i+4);
	
    if      (TYPE == 0x00) {	
      data_0 = RF_RD08 (addr_0);
      data_1 = RF_RD08 (addr_1);
      data_2 = RF_RD16 (addr_2);
      data_3 = RF_RD32 (addr_3);
	}
    else if (TYPE == 0x01) {	
      data_0 = RAM_RD08 (addr_0);
      data_1 = RAM_RD08 (addr_1);
      data_2 = RAM_RD16 (addr_2);
      data_3 = RAM_RD32 (addr_3);
	}
    else { break; }

    printf ("(08, 0x%02X, 0x%02X)\n", addr_0, data_0);
    printf ("(08, 0x%02X, 0x%02X)\n", addr_1, data_1);
    printf ("(16, 0x%02X, 0x%02X)\n", addr_2, data_2);
    printf ("(32, 0x%02X, 0x%02X)\n", addr_3, data_3);    
  }
  printf ("\nUART Check : OK \n\n");
}

//===============================================================================
// Main
//===============================================================================
/*  main by Jack
int main (void)
{ 	
  printf ("|===============================|\n");
  printf ("|      HPS Read/Write RAM       |\n");
  printf ("|===============================|\n\n");
  
  // HPS Read Data From RAM (64 Bytes) 
  HPS_RDATA (RAM_TYPE, 0x0000, 0x0008);
  
  // HPS Write Data To RAM (64 Bytes)
  printf ("HPS Write Data To RAM : ");
  RAM_WT32 (0x0000, 0x13121110);
  RAM_WT16 (0x0004, 0x1514);
  RAM_WT08 (0x0006, 0x16);
  RAM_WT08 (0x0007 ,0x17);
  RAM_WT32 (0x0008 ,0x1B1A1918);
  RAM_WT16 (0x000C, 0x1D1C);
  RAM_WT08 (0x000E, 0x1E);
  RAM_WT08 (0x000F ,0x1F);
  RAM_WT32 (0x0010, 0x23222120);
  RAM_WT16 (0x0014, 0x2524);
  RAM_WT08 (0x0016, 0x26);
  RAM_WT08 (0x0017 ,0x27);
  RAM_WT32 (0x0018 ,0x2B2A2928);
  RAM_WT16 (0x001C, 0x2D2C);
  RAM_WT08 (0x001E, 0x2E);
  RAM_WT08 (0x001F ,0x2F);
  RAM_WT32 (0x0020, 0x33323130);
  RAM_WT16 (0x0024, 0x3534);
  RAM_WT08 (0x0026, 0x36);
  RAM_WT08 (0x0027 ,0x37);
  RAM_WT32 (0x0028 ,0x3B3A3938);
  RAM_WT16 (0x002C, 0x3D3C);
  RAM_WT08 (0x002E, 0x3E);
  RAM_WT08 (0x002F ,0x3F);
  RAM_WT32 (0x0030, 0x43424140);
  RAM_WT16 (0x0034, 0x4544);
  RAM_WT08 (0x0036, 0x46);
  RAM_WT08 (0x0037 ,0x47);
  RAM_WT32 (0x0038 ,0x4B4A4948);
  RAM_WT16 (0x003C, 0x4D4C);
  RAM_WT08 (0x003E, 0x4E);
  RAM_WT08 (0x003F ,0x4F);  
  printf ("Done \n");

  // HPS Read Data From RAM (64 Bytes)
  HPS_RDATA (RAM_TYPE, 0x0000, 0x0008); 

  printf ("|===============================|\n");
  printf ("| PDMA Payload Transfer Setting |\n");
  printf ("|===============================|\n\n");

  PDMA_CH2_DT (0x00002400, 
               0x00030000, 0x00010020, 
               0x00400001, 0x00000721);

  printf ("|===============================|\n");
  printf ("|        RF BLE Setting         |\n");
  printf ("|===============================|\n\n");
  
  // HPS Read Data From RF (32 Bytes) 
  HPS_RDATA (RF_TYPE, 0x0000, 0x0004);

  // BLE Mode
  RF_WT08 (0x0018, 0x88);
  RF_WT08 (0x0019, 0x08);

  // H_S0 = 0x34
  RF_WT08 (0x0014, 0x03); 

  // H_LENGTH = 64 bytes
  RF_WT08 (0x0015, 0x40);

  // H_S1 = 0x35
  RF_WT08 (0x0016, 0x35);

  // P0 Byte 0~4 (BLE used for 4 bytes)
  RF_WT08 (0x0000, 0xAD);
  RF_WT08 (0x0001, 0xFA);
  RF_WT08 (0x0002, 0x24);
  RF_WT08 (0x0003, 0x48);
  RF_WT08 (0x0004, 0x53);

  // FILTEREN (SCAN_REQ)
  RF_WT08 (0x001A, 0x01);

  // Data Rate = 2MHz
  RF_WT08 (0x0042, 0x22);

  // DLY_TXS
  RF_WT08 (0x004C, 0x24);
  
  // CRC
  RF_WT08 (0x0049, 0x03);
  
  // MASK IRQ
  RF_WT08 (0x0033, 0x9E);

  // Light Sleep Mode
  RF_WT08 (0x001C, 0x01);

  printf ("|===============================|\n");
  printf ("|          BLE TX Enable        |\n");
  printf ("|===============================|\n\n");

  // GIOTMS
  RF_WT08 (0x00A6, 0x86);
  
  // TX Strobe Command
  RF_WT08 (0x001E, 0x01);
  
  printf ("\n");
  printf ("\n");

  return 0;
}
*/
