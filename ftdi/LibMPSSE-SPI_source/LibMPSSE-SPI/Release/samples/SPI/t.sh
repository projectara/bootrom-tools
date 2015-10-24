pushd ../../../LibMPSSE/Build/Linux/
#make clean
#make
popd
pushd SPI
gcc -g -I. -o sample-static.o sample-static.c ../../../../LibMPSSE/Build/Linux/libMPSSE.a -ldl && ./sample-static.o
popd
