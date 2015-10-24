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

char buf[1024 * 1024];
int main(int argc, char **argv) {
    FT_STATUS status = FT_OK;
    uint32 sizeToTransfer = 0;
	uint32 sizeTransferred=0;
    int filelen;
    int seconds_to_flash;

    if (argc != 3) {
        printf("Usage: %s [A|B] infile\n", argv[0]);
        return 1;
    }

    status = spi_init(argv[1][0] == 'A');
    if (status != FT_OK) {
        printf("Can't find SPI device\n");
        spi_deinit();
        return 1;
    }
    printf("load file: %s\n", argv[2]);
    FILE *fp = fopen(argv[2], "r");

    filelen = fread(buf, 1, 1024 * 1024, fp);
    seconds_to_flash = filelen / 256;
    printf("file read: %d bytes\n",filelen);
    printf("This will take about %d minutes to flash\n",
           (seconds_to_flash + 30) / 60);
    printf("Erase the whole chip...\n");
    WriteEnable();
    ChipErase();
    WaitForWriteDone();
    printf("Erase done\n");

    int blockl = 256;
    int addr = 0;
    while(filelen > 0) {
        WriteEnable();
        wBuffer[0] = 0x02;
        wBuffer[1] = (addr & 0x00ff0000) >> 16;
        wBuffer[2] = (addr & 0x0000ff00) >> 8;
        wBuffer[3] = 0;
        if (filelen < blockl) blockl = filelen;
        memcpy(&wBuffer[4], &buf[addr], blockl);
        sizeToTransfer = blockl + 4;
        sizeTransferred = 0;
        printf("write %d bytes to %d\n", blockl, addr);
        status = ReadWrite(sizeToTransfer, &sizeTransferred);
        APP_CHECK_STATUS(status);

        filelen -= blockl;
        addr += blockl;
        WaitForWriteDone();
    }
    printf("\n");

    printf("%d bytes written. Now read back\n", addr);

    uint32 readaddr = 0;
    uint32 readblk;
    while (addr) {
        wBuffer[0] = 0x03;
        wBuffer[1] = (readaddr & 0x00FF0000) >> 16;
        wBuffer[2] = (readaddr & 0x0000FF00) >> 8;
        wBuffer[3] = 0;
        if (addr > 1024) {
            readblk = 1024;
        }
        else {
            readblk = addr;
        }
        sizeToTransfer = readblk + 4;
        ReadWrite(sizeToTransfer, &sizeTransferred);
        printf("read %d bytes back at 0x%x\n", sizeTransferred - 4, readaddr);
        if (memcmp(&rBuffer[4], &buf[readaddr], readblk)) {
            printf("ERROR!!!!!!\n");
            goto ErrorReturn;
        }
        readaddr += readblk;
        addr -= readblk;
    }
    printf("OK! image verified!\n");
ErrorReturn:
    fclose(fp);
}
