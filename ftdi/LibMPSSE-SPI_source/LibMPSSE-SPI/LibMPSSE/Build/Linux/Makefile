# Makefile to build the libMPSSE.dll
# --- macros
COMMON_INC_DIR = ../../Common/inc
EXTERNAL_INC_DIR = ../../../External/Linux/libftd2xx1.1.12/release
INFRA_INC_DIR = ../../Infra/inc
MIDDLE_INC_DIR = ../../MiddleLayer/inc
#I2C_INC_DIR = ../../TopLayer/I2C/inc
SPI_INC_DIR = ../../TopLayer/SPI/inc

#ALL_INC_DIR = -I$(EXTERNAL_INC_DIR) -I$(INFRA_INC_DIR) -I$(COMMON_INC_DIR) -I$(MIDDLE_INC_DIR)  -I$(I2C_INC_DIR) 
ALL_INC_DIR = -I$(EXTERNAL_INC_DIR) -I$(INFRA_INC_DIR) -I$(COMMON_INC_DIR) -I$(MIDDLE_INC_DIR)  -I$(SPI_INC_DIR)


COMMON_SRC_DIR = ../../Common/src
INFRA_SRC_DIR = ../../Infra/src
MIDDLE_SRC_DIR = ../../MiddleLayer/src
#I2C_SRC_DIR = ../../TopLayer/I2C/src
SPI_SRC_DIR = ../../TopLayer/SPI/src

#ALL_SRC_DIR = -I$(INFRA_SRC_DIR) -I$(COMMON_SRC_DIR) -I$(MIDDLE_SRC_DIR) -I$(I2C_SRC_DIR) 
ALL_SRC_DIR = -I$(INFRA_SRC_DIR) -I$(COMMON_SRC_DIR) -I$(MIDDLE_SRC_DIR) -I$(SPI_SRC_DIR)

#Put macros here
#MACROS = -DINFRA_DEBUG_ENABLE
MACROS = 

CC=gcc
#use only o0 during debugging

AR=ar

#release mode
CFLAGS= -O3 -Wall $(MACROS) $(ALL_INC_DIR)

#without profiling & coverage
#CFLAGS=  -g -O0 -Wall $(MACROS) $(ALL_INC_DIR)

#with profiling & coverage
#CFLAGS=  -g -O0 -Wall -fprofile-arcs -ftest-coverage D$(MACROS) $(ALL_INC_DIR)

#OBJECTS= ftdi_infra.o ftdi_mid.o ftdi_i2c.o 
OBJECTS= ftdi_infra.o ftdi_mid.o ftdi_spi.o

LIBS = -L /MinGW/lib -ldl

# --- targets
all:    libMPSSE
libMPSSE:   $(OBJECTS)
		$(CC)  -o libMPSSE.so -shared $(OBJECTS) $(LIBS)
		$(AR) rcs libMPSSE.a $(OBJECTS)
ftdi_infra.o: $(INFRA_INC_DIR)
		$(CC) $(CFLAGS) -c -fPIC $(INFRA_SRC_DIR)/ftdi_infra.c
		
ftdi_mid.o: $(MIDDLE_INC_DIR)
		$(CC) $(CFLAGS) -c -fPIC $(MIDDLE_SRC_DIR)/ftdi_mid.c
		
#ftdi_i2c.o: $(I2C_INC_DIR)
#		$(CC) $(CFLAGS) -c -fPIC $(I2C_SRC_DIR)/ftdi_i2c.c

ftdi_spi.o: $(SPI_INC_DIR)
		$(CC) $(CFLAGS) -c -fPIC $(SPI_SRC_DIR)/ftdi_spi.c

# --- remove binary and executable files
#clean:
#		del -f tst $(OBJECTS)

clean :
#	del *.i *.o *.exe *.bak *.txt *.dll
	rm *.i *.o *.exe *.bak *.txt *.dll *.so *.a


