/*
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spirom_common.h"

/******************************************************************************/
/*								Global variables							  	    */
/******************************************************************************/
FT_HANDLE ftHandle;
uint8 rBuffer[SPI_DEVICE_BUFFER_SIZE] = {0};
uint8 wBuffer[SPI_DEVICE_BUFFER_SIZE] = {0};

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
	if (channels == 0)
	{
		status = FT_DEVICE_NOT_FOUND;
		APP_CHECK_STATUS(status);
	}

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
			printf("		ftHandle=0x%p\n",devList.ftHandle);/*is 0 unless open*/
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
		printf("\nhandle=0x%p status=0x%x\n",ftHandle,status);
		status = SPI_InitChannel(ftHandle,&channelConf);
		APP_CHECK_STATUS(status);
	}
	return status;
}

FT_STATUS spi_deinit(void) {
    /* deinit library */
#ifdef _MSC_VER
    Cleanup_libMPSSE();
#endif
}
