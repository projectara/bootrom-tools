/*!
 * \file sample-static.c
 *
 * \author FTDI
 * \date 20110512
 *
 * Copyright � 2000-2014 Future Technology Devices International Limited
 *
 *
 * THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Project: libMPSSE
 * Module: SPI Sample Application - Interfacing 94LC56B SPI EEPROM
 *
 * Rivision History:
 * 0.1  - 20110512 - Initial version
 * 0.2  - 20110801 - Changed LatencyTimer to 255
 * 					 Attempt to open channel only if available
 *					 Added & modified macros
 *					 Included stdlib.h
 * 0.3  - 20111212 - Added comments
 * 0.41 - 20140903 - Fixed compilation warnings
 *					 Added testing of SPI_ReadWrite()
 */

/******************************************************************************/
/* 							 Include files										   */
/******************************************************************************/
/* Standard C libraries */
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
#define SPI_DEVICE_BUFFER_SIZE		1024
#define SPI_WRITE_COMPLETION_RETRY		10
#define START_ADDRESS_EEPROM 	0x00 /*read/write start address inside the EEPROM*/
#define END_ADDRESS_EEPROM		0x10
#define RETRY_COUNT_EEPROM		10	/* number of retries if read/write fails */
#define CHANNEL_TO_OPEN			0	/*0 for first available channel, 1 for next... */
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
		if ((rBuffer[1] & 0x01) == 0) {
			return;
		}
	}
}

/******************************************************************************/
/*						Public function definitions						  		   */
/******************************************************************************/
/*!
 * \brief Main function / Entry point to the sample application
 *
 * This function is the entry point to the sample application. It opens the channel, writes to the
 * EEPROM and reads back.
 *
 * \param[in] none
 * \return Returns 0 for success
 * \sa
 * \note
 * \warning
 */
int main()
{
	FT_STATUS status = FT_OK;
	FT_DEVICE_LIST_INFO_NODE devList = {0};
	ChannelConfig channelConf = {0};
	uint8 address = 0;
	uint32 channels = 0;
	uint16 data = 0;
	uint8 i = 0;
	uint8 latency = 255;
	uint32 sizeToTransfer = 0;
	uint32 sizeTransferred=0;
	uint8 writeComplete=0;
	uint32 retry=0;
	int x;

	channelConf.ClockRate = 500000;
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
			printf("Information on channel number %d:\n",i);
			/* print the dev info */
			printf("		Flags=0x%x\n",devList.Flags);
			printf("		Type=0x%x\n",devList.Type);
			printf("		ID=0x%x\n",devList.ID);
			printf("		LocId=0x%x\n",devList.LocId);
			printf("		SerialNumber=%s\n",devList.SerialNumber);
			printf("		Description=%s\n",devList.Description);
			printf("		ftHandle=0x%x\n",(unsigned int)devList.ftHandle);/*is 0 unless open*/
		}
		/* Open the first available channel */
		status = SPI_OpenChannel(CHANNEL_TO_OPEN,&ftHandle);
		APP_CHECK_STATUS(status);
		printf("\nhandle=0x%x status=0x%x\n",(unsigned int)ftHandle,status);
		status = SPI_InitChannel(ftHandle,&channelConf);
		APP_CHECK_STATUS(status);

		{
			wBuffer[0] = 0x9F;
			sizeToTransfer = 4;
			sizeTransferred = 0;
			status = ReadWrite(sizeToTransfer, &sizeTransferred);
			APP_CHECK_STATUS(status);
			dumprBuffer("ID");

			status = WriteEnable();
			APP_CHECK_STATUS(status);

			wBuffer[0] = 0x02;
			wBuffer[1] = 0;
			wBuffer[2] = 0;
			wBuffer[3] = 0;
			for (x = 0; x < 256; x++) {
				wBuffer[4 + x] = x;
			}
			sizeToTransfer = 260;
			sizeTransferred = 0;
			status = ReadWrite(sizeToTransfer, &sizeTransferred);
			APP_CHECK_STATUS(status);

			WaitForWriteDone();

			wBuffer[0] = 0x03;
			wBuffer[1] = 0;
			wBuffer[2] = 0;
			wBuffer[3] = 0;
			sizeToTransfer = 20;
			sizeTransferred = 0;
			status = ReadWrite(sizeToTransfer, &sizeTransferred);
			APP_CHECK_STATUS(status);

			printf("%d: %02X %02X %02X %02X %02X %02X %02X %02X\n", sizeTransferred, rBuffer[0], rBuffer[1], rBuffer[2], rBuffer[3], rBuffer[4], rBuffer[5], rBuffer[6], rBuffer[7]);
		}

		//status = SPI_CloseChannel(ftHandle);
		printf("-----\r\n");
	}

#ifdef _MSC_VER
	//Cleanup_libMPSSE();
#endif

#ifndef __linux__
	system("pause");
#endif
	return 0;
}
