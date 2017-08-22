#include <stdio.h>
#include <tinyara/spi/spi.h>

typedef struct spi_dev_s SPI_DEV;

#define MAX_LEN 18

#define PCD_IDLE              0x00
#define PCD_AUTHENT           0x0E
#define PCD_RECEIVE           0x08
#define PCD_TRANSMIT          0x04
#define PCD_TRANSCEIVE        0x0C
#define PCD_RESETPHASE        0x0F
#define PCD_CALCCRC           0x03



#define PICC_REQIDL           0x26
#define PICC_REQALL           0x52
#define PICC_ANTICOLL         0x93            
#define PICC_ANTICOLL1        0x93
#define PICC_ANTICOLL2        0x95
#define PICC_AUTHENT1A        0x60
#define PICC_AUTHENT1B        0x61
#define PICC_READ             0x30
#define PICC_WRITE            0xA0
#define PICC_DECREMENT        0xC0
#define PICC_INCREMENT        0xC1
#define PICC_RESTORE          0xC2
#define PICC_TRANSFER         0xB0
#define PICC_HALT             0x50

////////////////
//MF522 FIFO
////////////////
#define DEF_FIFO_LENGTH       64      

// PAGE 0
#define     RFU00                 0x00  
#define     CommandReg            (0x01 << 1)   
#define     CommIEnReg            (0x02 << 1)
#define     DivlEnReg             (0x03 << 1)   
#define     CommIrqReg            (0x04 << 1)   
#define     DivIrqReg             (0x05 << 1)
#define     ErrorReg              (0x06 << 1)   
#define     Status1Reg            (0x07 << 1)   
#define     Status2Reg            (0x08 << 1)   
#define     FIFODataReg           (0x09 << 1)
#define     FIFOLevelReg          (0x0A << 1)
#define     WaterLevelReg         (0x0B << 1)
#define     ControlReg            (0x0C << 1)
#define     BitFramingReg         (0x0D << 1)
#define     CollReg               (0x0E << 1)
#define     RFU0F                 (0x0F << 1)

// PAGE 1    
#define     RFU10                 (0x10 << 1)
#define     ModeReg               (0x11 << 1)
#define     TxModeReg             (0x12 << 1)
#define     RxModeReg             (0x13 << 1)
#define     TxControlReg          (0x14 << 1)
#define     TxAutoReg             (0x15 << 1)
#define     TxSelReg              (0x16 << 1)
#define     RxSelReg              (0x17 << 1)
#define     RxThresholdReg        (0x18 << 1)
#define     DemodReg              (0x19 << 1)
#define     RFU1A                 (0x1A << 1)
#define     RFU1B                 (0x1B << 1)
#define     MifareReg             (0x1C << 1)
#define     RFU1D                 (0x1D << 1)
#define     RFU1E                 (0x1E << 1)
#define     SerialSpeedReg        (0x1F << 1)

// PAGE 2   
#define     RFU20                 (0x20 << 1) 
#define     CRCResultRegM         (0x21 << 1)
#define     CRCResultRegL         (0x22 << 1)
#define     RFU23                 (0x23 << 1)
#define     ModWidthReg           (0x24 << 1)
#define     RFU25                 (0x25 << 1)
#define     RFCfgReg              (0x26 << 1)
#define     GsNReg                (0x27 << 1)
#define     CWGsCfgReg            (0x28 << 1)
#define     ModGsCfgReg           (0x29 << 1)
#define     TModeReg              (0x2A << 1)
#define     TPrescalerReg         (0x2B << 1)
#define     TReloadRegH           (0x2C << 1)
#define     TReloadRegL           (0x2D << 1)
#define     TCounterValueRegH     (0x2E << 1)
#define     TCounterValueRegL     (0x2F << 1)

// PAGE 3     
#define     RFU30                 (0x30 << 1)
#define     TestSel1Reg           (0x31 << 1)
#define     TestSel2Reg           (0x32 << 1)
#define     TestPinEnReg          (0x33 << 1)
#define     TestPinValueReg       (0x34 << 1)
#define     TestBusReg            (0x35 << 1)
#define     AutoTestReg           (0x36 << 1)
#define     VersionReg            (0x37 << 1)
#define     AnalogTestReg         (0x38 << 1)
#define     TestDAC1Reg           (0x39 << 1)
#define     TestDAC2Reg           (0x3A << 1) 
#define     TestADCReg            (0x3B << 1)  
#define     RFU3C                 0x3C  
#define     RFU3D                 0x3D  
#define     RFU3E                 0x3E  
#define     RFU3F    0x3F




#define MI_OK                          0
#define MI_NOTAGERR                    (-1)
#define MI_ERR                         (-2)

void MFRC522_init();

void writeMFRC522(SPI_DEV* spi_dev, unsigned char Address, unsigned char value);


unsigned char readMFRC522(SPI_DEV* spi_dev, unsigned char Address);


void RFID_setBitMask(SPI_DEV* spi_dev, unsigned char reg, unsigned char mask);


void RFID_clearBitMask(SPI_DEV* spi_dev, unsigned char reg, unsigned char mask);

void RFID_antennaOn(SPI_DEV* spi_dev);

void RFID_antennaOff(SPI_DEV* spi_dev);

void RFID_calculateCRC(SPI_DEV* spi_dev, unsigned char *pIndata, unsigned char len, unsigned char *pOutData);


unsigned char RFID_MFRC522ToCard(SPI_DEV* spi_dev, unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen);

unsigned char RFID_findCard(SPI_DEV* spi_dev, unsigned char reqMode, unsigned char *TagType);

unsigned char RFID_anticoll(SPI_DEV* spi_dev, unsigned char *serNum);

unsigned char RFID_auth(SPI_DEV* spi_dev, unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum);

unsigned char RFID_read(SPI_DEV* spi_dev, unsigned char blockAddr, unsigned char *recvData);

unsigned char RFID_write(SPI_DEV* spi_dev, unsigned char blockAddr, unsigned char *writeData);

unsigned char RFID_selectTag(SPI_DEV* spi_dev, unsigned char *serNum);