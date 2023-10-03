/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __BLE_SOC_H
#define __BLE_SOC_H

//===============================================================================
// Base Address
//===============================================================================
#define H2F_ADDRESS  (0XFF200000)
#define F2H_ADDRESS  (0xFFFF0000)
#define IRQ_ADDRESS  (0xFFFED000)
#define SPI_ADDRESS  (H2F_ADDRESS + 0X00000000)
#define RF_ADDRESS   (H2F_ADDRESS + 0X00010000)
#define PDMA_ADDRESS (H2F_ADDRESS + 0X00020000)
#define RAM_ADDRESS  (H2F_ADDRESS + 0X00030000)
#define AES_ADDRESS  (H2F_ADDRESS + 0X00040000)

#define RF_TYPE  0x00
#define RAM_TYPE 0x01
#define MIX_TYPE 0x02

//===============================================================================
// RF Read/Write Command
//===============================================================================
static volatile uint32_t rf_0x1000 = 0xFF;
static inline void RF_WTRB(const uint32_t ADDRESS, const uint32_t DATA){
//  if (rf_0x1000 != DATA)
  ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) ) = DATA;
//      rf_0x1000  = DATA;
}

static inline void RF_WT08(const uint32_t ADDRESS, const uint8_t DATA){
  RF_WTRB (0x1000, 0x0);
  ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void RF_WT16(const uint32_t ADDRESS, const uint16_t DATA){
  RF_WTRB (0x1000, 0x1);
  ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void RF_WT32(const uint32_t ADDRESS, const uint32_t DATA){
  RF_WTRB (0x1000, 0x2);
  ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline uint8_t RF_RD08(const uint32_t ADDRESS){
  RF_WTRB (0x1000, 0x0);
  uint8_t  DATA = ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint16_t RF_RD16(const uint32_t ADDRESS){
  RF_WTRB (0x1000, 0x1);
  uint16_t DATA = ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint32_t RF_RD32(const uint32_t ADDRESS){
  RF_WTRB (0x1000, 0x2);
  uint32_t DATA = ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline void RF_ClearBits08(const uint32_t ADDRESS, const uint8_t BITMAP){
  RF_WTRB (0x1000, 0x0);
  uint8_t  DATA = ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) );
                  ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) ) = (DATA & ~BITMAP);
}

static inline void RF_SetBits08(const uint32_t ADDRESS, const uint8_t BITMAP){
  RF_WTRB (0x1000, 0x0);
  uint8_t  DATA = ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) );
                  ( *((volatile uint32_t *) (RF_ADDRESS + (ADDRESS<<2))) ) = (DATA | BITMAP);
}

//===============================================================================
// PDMA Read/Write Command
//===============================================================================
static inline void PDMA_WTRB(const uint32_t ADDRESS, const uint32_t DATA){
  ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void PDMA_WT08(const uint32_t ADDRESS, const uint8_t DATA){
  PDMA_WTRB (0x1000, 0x0);
  ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void PDMA_WT16(const uint32_t ADDRESS, const uint16_t DATA){
  PDMA_WTRB (0x1000, 0x1);
  ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void PDMA_WT32(const uint32_t ADDRESS, const uint32_t DATA){
  PDMA_WTRB (0x1000, 0x2);
  ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline uint8_t PDMA_RD08(const uint32_t ADDRESS){
  PDMA_WTRB (0x1000, 0x0);
  uint8_t  DATA = ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint16_t PDMA_RD16(const uint32_t ADDRESS){
  PDMA_WTRB (0x1000, 0x1);
  uint16_t DATA = ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint32_t PDMA_RD32(const uint32_t ADDRESS){
  PDMA_WTRB (0x1000, 0x2);
  uint32_t DATA = ( *((volatile uint32_t *) (PDMA_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

//===============================================================================
// RAM Read/Write Command
//===============================================================================
static volatile uint32_t ram_0x1000 = 0xFF;
static inline void RAM_WTRB(const uint32_t ADDRESS, const uint32_t DATA){
//if (ram_0x1000 != DATA)
  ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) ) = DATA;
//    ram_0x1000  = DATA;
}

static inline void RAM_WT08(const uint32_t ADDRESS, const uint8_t DATA){
  RAM_WTRB (0x1000, 0x0);
  ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void RAM_WT16(const uint32_t ADDRESS, const uint16_t DATA){
  RAM_WTRB (0x1000, 0x1);
  ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void RAM_WT32(const uint32_t ADDRESS, const uint32_t DATA){
  RAM_WTRB (0x1000, 0x2);
  ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void RAM_WT32_4aligned(uint32_t ADDRESS, uint32_t dataADDR, uint8_t count) {
  RAM_WTRB (0x1000, 0x2);
  while(count) {
      ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) ) = ( *((uint32_t *) (dataADDR)) );
      ADDRESS += 4;
      dataADDR += 4;
      count --;
  }
}

static inline uint8_t RAM_RD08(const uint32_t ADDRESS){
  RAM_WTRB (0x1000, 0x0);
  uint8_t  DATA = ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint16_t RAM_RD16(const uint32_t ADDRESS){
  RAM_WTRB (0x1000, 0x1);
  uint16_t DATA = ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint32_t RAM_RD32(const uint32_t ADDRESS){
  RAM_WTRB (0x1000, 0x2);
  uint32_t DATA = ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline void RAM_RD32_4aligned(uint32_t dataADDR, uint32_t ADDRESS, uint8_t count){
  RAM_WTRB (0x1000, 0x2);
  while(count) {
      ( *((uint32_t *) (dataADDR)) ) = ( *((volatile uint32_t *) (RAM_ADDRESS + (ADDRESS<<2))) );
      ADDRESS += 4;
      dataADDR += 4;
      count --;
  }
}

//===============================================================================
// AES Read/Write Command
//===============================================================================
static inline void AES_WTRB(uint32_t ADDRESS, uint32_t DATA){
  ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void AES_WT08(uint32_t ADDRESS, uint8_t DATA){
  AES_WTRB (0x1000, 0x0);  
  ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void AES_WT16(uint32_t ADDRESS, uint16_t DATA){
  AES_WTRB (0x1000, 0x1);
  ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline void AES_WT32(uint32_t ADDRESS, uint32_t DATA){
  AES_WTRB (0x1000, 0x2);
  ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) ) = DATA;
}

static inline uint8_t AES_RD08(uint32_t ADDRESS){
  AES_WTRB (0x1000, 0x0);
  uint8_t  DATA = ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint16_t AES_RD16(uint32_t ADDRESS){
  AES_WTRB (0x1000, 0x1);	
  uint16_t DATA = ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

static inline uint32_t AES_RD32(uint32_t ADDRESS){
  AES_WTRB (0x1000, 0x2);
  uint32_t DATA = ( *((volatile uint32_t *) (AES_ADDRESS + (ADDRESS<<2))) );
  return DATA;
}

//===============================================================================
// SPI Read/Write Command
//===============================================================================
static inline void SREG_WT32(const uint32_t ADDRESS, const uint32_t DATA){
  ( *((volatile uint32_t *) (SPI_ADDRESS + ADDRESS)) ) = DATA;
}

static inline uint32_t SREG_RD32(const uint32_t ADDRESS){
  uint32_t DATA = ( *((volatile uint32_t *) (SPI_ADDRESS + ADDRESS)) );
  return DATA;
}

static inline void SPI_CS(const uint32_t ADDRESS){
  
  // Host SPI Chip Selection
  // 2'd0 : RF Frontend
  // 2'd1 : RF ADC
  // 2'd2 : FPGA Baseband
  SREG_WT32	(0x10, ADDRESS);
}

static inline void SPI_WT(const uint32_t ADDRESS, const uint32_t DATA){
  
  SREG_WT32	(0x04, ADDRESS | 0x40);
  SREG_WT32	(0x08, DATA);
  SREG_WT32	(0x00, 0x01);
  while (1){
	if (SREG_RD32(0x00) == 0x00) { break; }
  }
}

static inline uint32_t SPI_RD(const uint32_t ADDRESS){

  uint32_t DATA;
  SREG_WT32	(0x04, ADDRESS | 0xC0);
  SREG_WT32	(0x00, 0x10);
  while (1){
	if (SREG_RD32(0x00) == 0x00) { break; }
  }
  DATA = SREG_RD32(0x0C);
  return DATA;  
}

static inline void SPI_BYTE(const uint32_t BYTE){

  SREG_WT32	(0x04, BYTE);
  SREG_WT32	(0x00, 0x80);
  while (1){
	if (SREG_RD32(0x00) == 0x00) { break; }
  }
}


/* Exported functions --------------------------------------------------------------------------------------*/

#endif /* __BLE_SOC_H */
