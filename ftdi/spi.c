#include<stdio.h>
#include<stdlib.h>
#include<string.h>
/* OS specific libraries */
#ifdef _WIN32
#include<windows.h>
#endif

/* Include D2XX header*/
#include "ftd2xx.h"

/* Include libMPSSE header */
#include "libMPSSE_spi.h"
/******************************************************************************/
/*								Macro and type defines							   */
/******************************************************************************/
/* Helper macros */

#define APP_CHECK_STATUS(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);exit(1);}else{;}};
#define CHECK_NULL(exp){if(exp==NULL){printf("%s:%d:%s():  NULL expression \
encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};

/* Application specific macro definations */
#define SPI_DEVICE_BUFFER_SIZE		1024 * 1024
#define SPI_WRITE_COMPLETION_RETRY		10
#define START_ADDRESS_EEPROM 	0x00 /*read/write start address inside the EEPROM*/
#define END_ADDRESS_EEPROM		0x10
#define RETRY_COUNT_EEPROM		10	/* number of retries if read/write fails */
//#define CHANNEL_TO_OPEN			0	/*0 for first available channel, 1 for next... */
#define SPI_SLAVE_0				0
#define SPI_SLAVE_1				1
#define SPI_SLAVE_2				2
#define DATA_OFFSET				4
#define USE_WRITEREAD			0

/******************************************************************************/
/*								Global variables							  	    */
/******************************************************************************/
static FT_HANDLE ftHandle;
static uint8 rBuffer[SPI_DEVICE_BUFFER_SIZE] = {0};
static uint8 wBuffer[SPI_DEVICE_BUFFER_SIZE] = {0};

FT_STATUS ReadWrite(int sizeToTransfer, int *sizeTransferred) {
	FT_STATUS status = FT_OK;
	status = SPI_ReadWrite(ftHandle, rBuffer, wBuffer, sizeToTransfer, sizeTransferred,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE|
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	return status;
}

void dumprBuffer(char *s) {
	printf("%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", s, rBuffer[0], rBuffer[1], rBuffer[2], rBuffer[3], rBuffer[4], rBuffer[5], rBuffer[6], rBuffer[7]);
}

FT_STATUS WriteEnable(void) {
	uint32 sizeToTransfer = 0;
	uint32 sizeTransferred=0;

	wBuffer[0] = 0x06;
	sizeToTransfer = 1;
	sizeTransferred = 0;
	return ReadWrite(sizeToTransfer, &sizeTransferred);
}

void WaitForWriteDone(void) {
	uint32 sizeToTransfer = 0;
	uint32 sizeTransferred=0;

	wBuffer[0] = 0x05;
	sizeToTransfer = 2;
	sizeTransferred = 0;
	while (1) {
		ReadWrite(sizeToTransfer, &sizeTransferred);
        //dumprBuffer("E");
		if ((rBuffer[1] & 0x01) == 0) {
			return;
		}
	}
}

FT_STATUS ChipErase(void) {
	uint32 sizeToTransfer = 0;
	uint32 sizeTransferred=0;

	wBuffer[0] = 0x60;
	sizeToTransfer = 1;
	sizeTransferred = 0;
	return ReadWrite(sizeToTransfer, &sizeTransferred);
}


FT_STATUS spi_init(int channelA) {
    FT_STATUS status = FT_OK;
	FT_DEVICE_LIST_INFO_NODE devList = {0};
	ChannelConfig channelConf = {0};
	uint32 channels = 0;
    uint8 latency = 255;
    uint32 channelToOpen = 0;
    int i;

	channelConf.ClockRate = 3000000;
	channelConf.LatencyTimer = latency;
	channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
	channelConf.Pin = 0x00000000;/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/

	/* init library */
#ifdef _MSC_VER
	Init_libMPSSE();
#endif
	status = SPI_GetNumChannels(&channels);
	APP_CHECK_STATUS(status);
	printf("Number of available SPI channels = %d\n",(int)channels);

	if(channels>0)
	{
		for(i=0;i<channels;i++)
		{
			status = SPI_GetChannelInfo(i,&devList);
			APP_CHECK_STATUS(status);
#if 0            
			printf("Information on channel number %d:\n",i);
			printf("		Flags=0x%x\n",devList.Flags);
			printf("		Type=0x%x\n",devList.Type);
			printf("		ID=0x%x\n",devList.ID);
			printf("		LocId=0x%x\n",devList.LocId);
			printf("		SerialNumber=%s\n",devList.SerialNumber);
			printf("		Description=%s\n",devList.Description);
#endif            
			if (channelA) {
				if (!strcmp(devList.Description, "USB <-> Serial Converter A A")) {
					channelToOpen = i;
				}
			} else {
				if (!strcmp(devList.Description, "USB <-> Serial Converter B A")) {
					channelToOpen = i;
				}
			}
		}
		printf("use channel %d\n", channelToOpen);
		/* Open the first available channel */
		status = SPI_OpenChannel(channelToOpen,&ftHandle);
		APP_CHECK_STATUS(status);
		//printf("\nhandle=0x%x status=0x%x\n",(unsigned int)ftHandle,status);
		status = SPI_InitChannel(ftHandle,&channelConf);
		APP_CHECK_STATUS(status);
    }
}
