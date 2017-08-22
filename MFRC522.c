#include <stdio.h>
#include <string.h>
#include <apps/shell/tash.h>

// MFRC522
#include <tinyara/spi/spi.h>
#include <MFRC522.h>

#define MAX_LEN 18
#define MFRC522_VERSION_REG (0x37 << 1)


int port = 0; // SPI0 port
int port2 = 1; // SPI1 port (Arduino Shell) Not used in this code
int bits = 8;
int freq = 5000000;
int conf = 0;

int client_main(int argc, FAR char *argv[]);

static char spi_read(SPI_DEV* spi_dev, int spi_port, int addr)
{
	unsigned char buf[2];
	buf[0] = addr | 0x80;

	SPI_LOCK(spi_dev, true);

	SPI_SELECT(spi_dev, spi_port, true);
	SPI_RECVBLOCK(spi_dev, buf, 2);
	SPI_SELECT(spi_dev, spi_port, false);

	SPI_LOCK(spi_dev, false);

	return buf[1];
}

static void spi_write(SPI_DEV* spi_dev, int spi_port, int addr, char value)
{
	unsigned char buf[2];
	buf[0] = addr;
	buf[1] = value;

	SPI_LOCK(spi_dev, true);

	SPI_SELECT(spi_dev, spi_port, true);
	SPI_SNDBLOCK(spi_dev, buf, 2);
	SPI_SELECT(spi_dev, spi_port, false);

	SPI_LOCK(spi_dev, false);
}

int MFRC522_init_youngje(SPI_DEV* spi_dev, int spi_port){

	uint8_t fwver;
	uint8_t registerVal;

	SPI_LOCK(spi_dev, true);
	SPI_SETFREQUENCY(spi_dev, freq);
	SPI_SETBITS(spi_dev, bits);
	SPI_SETMODE(spi_dev, conf);
	SPI_LOCK(spi_dev, false);

	fwver = spi_read(spi_dev, spi_port, MFRC522_VERSION_REG);

	if (fwver != 0x90 && fwver != 0x91 && fwver != 0x92 && fwver != 0x88 && fwver != 0x12)  {
		printf("It is not MFRC522 '%02X'\n", fwver);
		return -1;
	}
	else{
		printf("MFRC522 firmware v%02X\n", fwver);
	}

	spi_write(spi_dev, spi_port, CommandReg, PCD_RESETPHASE);
	spi_write(spi_dev, spi_port, TModeReg, 0x8D);   
	spi_write(spi_dev, spi_port, TPrescalerReg, 0x3E);  
	spi_write(spi_dev, spi_port, TReloadRegL, 30);
	spi_write(spi_dev, spi_port, TReloadRegH, 0);
	spi_write(spi_dev, spi_port, TxAutoReg, 0x40);   
	spi_write(spi_dev, spi_port, ModeReg, 0x3D);    

	RFID_antennaOn(spi_dev);

	return 0;
}

int main(int argc, FAR char *argv[]){
	tash_cmd_install("rfid", client_main, TASH_EXECMD_SYNC);
	return 0;
}

int client_main(int argc, FAR char *argv[]){

	int i;
	unsigned char s;
	unsigned char id[10] = { 0, };
	int result;

	static struct spi_dev_s *spi_dev;

	// MFRC522 read
	spi_dev = up_spiinitialize(port);

	if (MFRC522_init_youngje(spi_dev, port) < 0){ printf("port : %d Initialize ERROR!\n", port); }


	while (1){
		result = RFID_findCard(spi_dev, 0x52, &s); 
		if (result == MI_OK){
			if (RFID_anticoll(spi_dev, id) == MI_OK){
				for (i = 0; i < 5; ++i){
					printf("%d ", id[i]);
					printf("\n");
				}
			}
			else {
				printf("anticoll ERR!\n");
			}
		}

		return 0;
	}

void RFID_setBitMask(SPI_DEV* spi_dev, unsigned char reg, unsigned char mask)
{
	unsigned char tmp;
	tmp = spi_read(spi_dev, port, reg);
	spi_write(spi_dev, port, reg, tmp | mask);  // set bit mask
}

void RFID_clearBitMask(SPI_DEV* spi_dev, unsigned char reg, unsigned char mask)
{
	unsigned char tmp;
	tmp = spi_read(spi_dev, port, reg);
	spi_write(spi_dev, port, reg, tmp & (~mask));  // clear bit mask
}

void RFID_antennaOn(SPI_DEV* spi_dev)
{
	unsigned char temp;

	temp = spi_read(spi_dev, port, TxControlReg);
	if (!(temp & 0x03))
	{
		RFID_setBitMask(spi_dev, TxControlReg, 0x03);
	}
}

void RFID_antennaOff(SPI_DEV* spi_dev)
{
	unsigned char temp;

	temp = spi_read(spi_dev, port, TxControlReg);
	if (!(temp & 0x03))
	{
		RFID_clearBitMask(spi_dev, TxControlReg, 0x03);
	}
}

void RFID_calculateCRC(SPI_DEV* spi_dev, unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
	unsigned char i, n;

	RFID_clearBitMask(spi_dev, DivIrqReg, 0x04);     
	RFID_setBitMask(spi_dev, FIFOLevelReg, 0x80);     


	for (i = 0; i<len; i++)
		spi_write(spi_dev, port, FIFODataReg, *(pIndata + i));
	spi_write(spi_dev, port, CommandReg, PCD_CALCCRC);


	i = 0xFF;
	do
	{
		n = spi_read(spi_dev, port, DivIrqReg);
		i--;
	} while ((i != 0) && !(n & 0x04));     

	pOutData[0] = spi_read(spi_dev, port, CRCResultRegL);
	pOutData[1] = spi_read(spi_dev, port, CRCResultRegM);
}

unsigned char RFID_MFRC522ToCard(SPI_DEV* spi_dev, unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen)
{
	unsigned char status = MI_ERR;
	unsigned char irqEn = 0x00;
	unsigned char waitIRq = 0x00;
	unsigned char lastBits;
	unsigned char n;
	unsigned int i;

	switch (command)
	{
	case PCD_AUTHENT:
	{
		irqEn = 0x12;
		waitIRq = 0x10;
		break;
	}
	case PCD_TRANSCEIVE: 
	{
		irqEn = 0x77;
		waitIRq = 0x30;
		break;
	}
	default:
		break;
	}

	spi_write(spi_dev, port, CommIEnReg, irqEn | 0x80);
	RFID_clearBitMask(spi_dev, CommIrqReg, 0x80);
	RFID_setBitMask(spi_dev, FIFOLevelReg, 0x80);    

	spi_write(spi_dev, port, CommandReg, PCD_IDLE);

	for (i = 0; i<sendLen; i++)
		spi_write(spi_dev, port, FIFODataReg, sendData[i]);

	spi_write(spi_dev, port, CommandReg, command);
	if (command == PCD_TRANSCEIVE)
		RFID_setBitMask(spi_dev, BitFramingReg, 0x80);  


	i = 2000; 
	do
	{
		n = spi_read(spi_dev, port, CommIrqReg);
		i--;
	} while ((i != 0) && !(n & 0x01) && !(n&waitIRq));

	RFID_clearBitMask(spi_dev, BitFramingReg, 0x80);   

	if (i != 0)
	{
		if (!(spi_read(spi_dev, port, ErrorReg) & 0x1B)) 
		{
			status = MI_OK;
			if (n & irqEn & 0x01)
				status = MI_NOTAGERR;   

			if (command == PCD_TRANSCEIVE)
			{
				n = spi_read(spi_dev, port, FIFOLevelReg);
				lastBits = spi_read(spi_dev, port, ControlReg) & 0x07;
				if (lastBits)
					*backLen = (n - 1) * 8 + lastBits;
				else
					*backLen = n * 8;

				if (n == 0)
					n = 1;
				if (n > MAX_LEN)
					n = MAX_LEN;

				for (i = 0; i<n; i++)
					backData[i] = spi_read(spi_dev, port, FIFODataReg);
			}
		}
		else
			status = MI_ERR;
	}

	return status;
}

unsigned char RFID_findCard(SPI_DEV* spi_dev, unsigned char reqMode, unsigned char *TagType)
{
	unsigned char status;
	unsigned int backBits;

	spi_write(spi_dev, port, BitFramingReg, 0x07);  

	TagType[0] = reqMode;
	status = RFID_MFRC522ToCard(spi_dev, PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
		status = MI_ERR;

	return status;
}

unsigned char RFID_anticoll(SPI_DEV* spi_dev, unsigned char *serNum)
{
	unsigned char status;
	unsigned char i;
	unsigned char serNumCheck = 0;
	unsigned int unLen;

	RFID_clearBitMask(spi_dev, Status2Reg, 0x08);   
	RFID_clearBitMask(spi_dev, CollReg, 0x80);     
	spi_write(spi_dev, port, BitFramingReg, 0x00);    

	serNum[0] = PICC_ANTICOLL;
	serNum[1] = 0x20;

	status = RFID_MFRC522ToCard(spi_dev, PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

	if (status == MI_OK)
	{
		for (i = 0; i<4; i++){
			*(serNum + i) = serNum[i];
			serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i]){
			status = MI_ERR;
		}
	}

	RFID_setBitMask(spi_dev, CollReg, 0x80); 

	return status;
}

unsigned char RFID_auth(SPI_DEV* spi_dev, unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
	unsigned char status;
	unsigned int recvBits;
	unsigned char i;
	unsigned char buff[12];


	buff[0] = authMode;
	buff[1] = BlockAddr;
	for (i = 0; i<6; i++)
		buff[i + 2] = *(Sectorkey + i);
	for (i = 0; i<4; i++)
		buff[i + 8] = *(serNum + i);

	status = RFID_MFRC522ToCard(spi_dev, PCD_AUTHENT, buff, 12, buff, &recvBits);
	if ((status != MI_OK) || (!(spi_read(spi_dev, port, Status2Reg) & 0x08)))
		status = MI_ERR;

	return status;
}

unsigned char RFID_read(SPI_DEV* spi_dev, unsigned char blockAddr, unsigned char *recvData)
{
	unsigned char status;
	unsigned int unLen;

	recvData[0] = PICC_READ;
	recvData[1] = blockAddr;
	RFID_calculateCRC(spi_dev, recvData, 2, &recvData[2]);
	status = RFID_MFRC522ToCard(spi_dev, PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

	if ((status != MI_OK) || (unLen != 0x90))
		status = MI_ERR;

	return status;
}

unsigned char RFID_write(SPI_DEV* spi_dev, unsigned char blockAddr, unsigned char *writeData)
{
	unsigned char status;
	unsigned int recvBits;
	unsigned char i;
	unsigned char buff[18];

	buff[0] = PICC_WRITE;
	buff[1] = blockAddr;
	RFID_calculateCRC(spi_dev, buff, 2, &buff[2]);
	status = RFID_MFRC522ToCard(spi_dev, PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

	if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
		status = MI_ERR;

	if (status == MI_OK)
	{
		for (i = 0; i<16; i++)    
			buff[i] = *(writeData + i);

		RFID_calculateCRC(spi_dev, buff, 16, &buff[16]);
		status = RFID_MFRC522ToCard(spi_dev, PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
			status = MI_ERR;
	}

	return status;
}

unsigned char RFID_selectTag(SPI_DEV* spi_dev, unsigned char *serNum)
{
	unsigned char i;
	unsigned char status;
	unsigned char size;
	unsigned int recvBits;
	unsigned char buffer[9];

	buffer[0] = PICC_ANTICOLL1;
	buffer[1] = 0x70;

	for (i = 0; i<5; i++)
		buffer[i + 2] = *(serNum + i);

	RFID_calculateCRC(spi_dev, buffer, 7, &buffer[7]);

	status = RFID_MFRC522ToCard(spi_dev, PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
	if ((status == MI_OK) && (recvBits == 0x18))
		size = buffer[i];
	else
		size = 0;
	return size;
}

int RFID_compare(unsigned char* id1, unsigned char* id2){
	if (id1[0] == id2[0] && id1[1] == id2[1] && id1[2] == id2[2] && id1[3] == id2[3] && id1[4] == id2[4]){
		return true;
	}
	else{
		return false;
	}
}
