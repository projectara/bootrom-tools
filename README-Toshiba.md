# Scripts for Building and Packaging Drops to Toshiba
**Before you start**: Your environment must be set up
correctly for the tools to function. (See: **Environment** below)

The bootrom-toools/scripts folder contains a number of tools to build
variants of the FFFF and bootrom images, with the bootrom
image names being ornamented with the build mode.

* **makedelivery** builds a **delivery** drop consisting of the source and
  tools trees, and the production compilation of the bootrom. The latter
  includes the .bin, .bitcount (count of 1's in .bin), and .dat (special
  hex dump of .bin) for the AP and GP bridges. Both .bin files are padded
  with 0xffffffff to a fixed 16k length, and the last 8 bytes are a
  serial number indicating the creation date and an AG/GP bridge flag.
* **makedrop** builds a **test** drop. It builds all the variants, generates the
  test scripts, all FFFF variants, etc. and packages it all into a zipped
  tar.
* **makeall** (called from makedrop) compiles the server, FFFF and bootrom for
  a specific configuration.
  *The minimal command line to create an ES3 bringup sample is:*
  
> `makeall -es3tsb --rev==HEAD`

* **makef4** (called from makeall) compiles and optionally signs the L2FW
  and L3FW with the desired configuration, creating FFFF.bin. (This is
  useful for es3 bootloader development and es3 bringup because it is a
  canonical 3-stage bootloader equivalent of "hello world".) It has an
  option to compile only S3FW and load it as S2FW to speed simulation.
* **makeboot** (called from makeall) compiles the bootrom with the desired
  configuration, creating an ornamented bootrom-xxx.bin
* **bootsuffix** is a utility which generates the ornamentation string, used
in various places in the above tools

In addition, there are a few extra scripts which can simplify building
stock versions for development testing. These tend to be wrappers around
some of the above-mentioned tools, with most of the boilerplate parameters
hardwired.

* **make3** left over from the ad-hoc days, but still useful for building
test images, this builds the FFFF and the bootrom in the desired
configuration.
* **make3sim** A wrapper for make3 which builds an fpga image equivalent to
  that generated with the old _SIMULATION flag.
* **makef4norm** is a wrapper for makef4, which generates an FFFF.bin
akin to that obtained by the current/previous _SIMULATION flag.

All of the above tools share a common set of parameters, with each tool
using a large subset of the whole.

##  Parameters
### General Parameters
* **-es2tsb, -es3tsb, -fpgatsb** Select the build target chip.
If omitted, it will try to use the environment variable
**CONFIG_CHIP**, which should be one of these without the leading dash.
If CONFIG_CHIP is not defined, it will try to fall back to
**DEFAULT_CONFIG_CHIP**.
* **-sign** Sign the L2FW and L3FW (generates "ffff-sgn.bin" instead of
"ffff.bin")
* **-dbg** Compile with debugging symbols and compiler options
* **-v** Verbose mode on TFTF creation
* **-prod** Enable _PRODUCTION mode for all 3 firmware levels.
This disables all test parameters and forces a non-debug build.()
* **-justboot** Normally, makeall will clear the drop folder before
updating it with the server, bootrom, FFFF images and all the rest of
the test suite files. Since the server and FFFF images need only be
built once, and because makeall is called by makedrop for each of the
bootrom images, -justboot is set on all but the first invocation, which
speeds the generation of the drop and prevents the bootrom builds from
unnecessary recompilation.
* **-zip** Tells makeall to generate a zip archive of the (interim) drop folder.

### TESTING parameters
* **-nocrypto** Substitute fake cryptographic routines to speed simulation
* **-gearchange"** Test for changing UniPro gear change* **
* **-debugmsg** Allow debug serial output
* **-handshake** GPIO handshake with simulation/test controller
* **-342** Substitute the L3FW for L2FW to speed simulation.
* **--spec=`<num>`** Compile special test number `<num>`.
(This appears to the code as the _SPECIAL_TEST define with the value of `<num>`.)

#### Special Test values
* **1** Spec_StandbyTest - Allow 3rd stage FW to try to put the chip
into standby. 
* **2** Spec_GBBootSrvStandbyTest - STANDBY_TEST plus S3FW waits for the
server to put UniPro to hibern8 mode before suspend
* **3** Spec_GearChangeTest - Run the SpiRom at different gear speeds

### Parameter Sets by Tool
* **makedrop** {-v} {-342}
* **makeall** {-es2tsb | -es3tsb | -fpgatsb } {-v} {-justboot} {-zip}
{-nocrypto} {-gearchange} {-debugmsg} {-handshake}
{-spec `<num>`} {-342}
{-justboot}
* **makeboot** {-es2tsb | -es3tsb | -fpgatsb } {-v} {-dbg} {-prod}
{-nocrypto} {-gearchange} {-debugmsg} {-handshake}
{-spec `<num>`} {-342}
* **makef4** {-es2tsb | -es3tsb | -fpgatsb } {-sign} {-v} {-dbg} {-prod}
{-nocrypto} {-gearchange} {-debugmsg} {-handshake}
{-spec `<num>`} {-342}
* **makef4norm** {-es2tsb | -es3tsb | -fpgatsb } {-sign} {-v}
{-handshake} {-spec `<num>`} {-342}
* **make3sim** {-es2tsb | -es3tsb | -fpgatsb } {-sign} {-dbg} {-v} {-prod}
{-spec `<num>`} {-342}


## Appendix A: Environment
The test-drop and delivery tools (found in .../bootrom-tools/scripts)
require the following environment variables be set in order to function:

Variable | Typical Value | Purpose
-------- | ------------- | -------
BOOTROM_ROOT | ~/work/bootrom | The bootrom repository
CONFIG_CHIP | es3tsb | The target chip (es2tsb, es3tsb, or fpgatsb)
DEFAULT_CONFIG_CHIP | es3tsb | Optional fallback default value for CONFIG_CHIP
DELIVERY_NAME | es3-bootrom-delivery | The name of a delivery (also used in creating the delivery folder)
DELIVERY_ROOT | ~ | Where the delivery folder will reside
DROP_ASSEMBLY_DIR | ~/bootrom-tools/es3-test | Where test drop components are created
KEY_DIR | ~/bin | Your local source of .pem files (used by makef4 to sign the TFTF)
TEST_DROP_DIR | ~/es3-test | The target folder for a given test drop

Because bootrom-tools grew organically, you need to ensure that
your PATH searches for them in the correct order:
${BOOTROM_TOOLS}/bin, ${BOOTROM_TOOLS}/scripts, ${BOOTROM_TOOLS}

## Appendix B: Related Documents
* **README.md** Describes the core Ara module packaging tools.
* **README-Toshiba.md** This document.
* **README-autotest.md** Describes the tools for loading and testing(ES3)
BootRom images with the HAPS-62 board.
