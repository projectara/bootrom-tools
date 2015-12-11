# bootrom-tools
## Building images using the Trusted Firmware Transfer Format and Flash Format For Firmware files
This repository contains sofware tool (i.e, Python scripts and C-programs) for
packaging firmware
images into Project Ara's Trusted Firmware Transfer Format (TFTF) and Flash
Format For Firmware (FFFF) file formats. These file formats and the basic
set of tools used with them are described in [ES3 Bridge ASIC Stage 1 Firmware High Level Design](https://docs.google.com/document/d/1OxjQClSY5cvS370XG9xH7FR9DZgRGv17LXE33EEXSxE/edit)
document.

The FFFF file is a filesystem romimage, which is composed of TFTF elements. The
TFTF elements typically come from the build system, but can also consist of data,
certificates, etc. The general process flow is outlined below

<center>Create element content -> create-tftf -> sign-tftf -> create-ffff</center>

Each tool will list its parameters if it is called with the flag `--help`.

## Tools
The tools and their parameters are described briefly below, in roughly the order
in which one would use them.

* **create-tftf** Assemble content blobs into a TFTF file
* **display-tftf** Display the contents of a TFTF file
* **sign-tftf** Cryptographically sign the contents of a TFTF file with the specified private key.
* **create-ffff** Assemble one or more TFTF blobs into an FFFF file
* **display-ffff** Display the contents of an FFFF file
* **nuttx2ffff** Package the nuttx.bin into an FFFF file

### create-tftf
**create-tftf**
--out <tftf-file>
--start <num>
--unipro-mfg <num>
--unipro-pid <num>
--ara-vid <num>
--ara-pid <num>
{--name <string>}
{--ara-stage <num>}
{--header-size <num>}
{--map}
{-v}
{Section}...

where each [Section] is composed of:

{--elf <file> | [--code <file> --load <num> --start <num>]}...
{--data <file>}...
{--manifest <file>}...
{--class <num>}
{--id <num>}

The flags can be understood as follows:

* `--out`: Specifies the filename to which the TFTF image should be written.
* `--start <num>`: The absolute memory address of the firmware's entry-point, as obtained from a disassembler or linker. Note that while there can be multiple code sections, there is only one --start, which must reference an address in one of the code sections.
* `--unipro-mfg <num>`: The Unipro Manufacturer ID (MID)/Vendor ID (VID) (these are two different phrasings for talking about the same number).  The specific value is obtained from the relevant hardware.
* `--unipro-pid <num>`: The Unipro Product ID (PID).  The specific value is obtained from the relevant hardware.
* `--ara-vid <num>`: The Project Ara Vendor ID (VID).  The specific value is obtained from the relevant hardware.
* `--ara-pid <num>`: The Project Ara Product ID (PID).  The specific value is obtained from the relevant hardware.
* `--name <string>`: The name of the module
* `--ara-stage <num>`: (ES3-bringup only) Specify the ARA boot stage.
* `--header-size <num>`: The size in byts of the TFTF header (defaults to 512).
* `--map`: Generate a .map file of the TFTF field offsets
* `-v`: Verbose mode, in which the script will dump the contents of the resultant TFTF headers to stdout when finished.

The section flags are similarly described:

* `--elf <file>`: Specifies a code section from an elf file. it extracts the --code, --load and --start parameters from the elf file.
* `--code`: Specifies the filename in which the raw binary for a code section can be found.  This should be the Ara firmware's `.text` section.
* `--load <num>`: The absolute address in memory to which the --code section will be loaded
boot-time, as obtained from a disassembler or linker.
* `--data  <file>`: Specifies the filename in which the raw binary for a data section can be found.  This should be the Ara firmware's `.data` section.
* `--manifest  <file>`:
section (which must be one of: `--code`, `--data`, or `--manifest`) when loading its contents to memory at boot-time.
* `--class <num>`: Specifies the TFTF section class
* `--id <num>`: Specifies the TFTF section ID

At least one section must be given via `--code`/`--elf`, `--data`, or `--manifest`, and an output filename via `--out` is also mandatory.

### display-tftf
**display-tftf**
{-v}
{--map}
`<tftf-file>`...

* `-v`: Verbose mode, in which the script will dump the contents of the TFTF headers in greater detail
* `--map`: Generate a .map file of the TFTF field offsets
* `<tftf-file>...`: One or more TFTF files to display

### sign-tftf
**sign-tftf**
--key <file>
--type <string>
--suffix <string>
--signature-algorithm <string>
--format [standard | es3]
{--passin [pass:<passphrase> | stdin | prompt]}
{--retry}
{--check}
{-v}
`<tftf-file>`...

* `--key <file>`: The private .pem key file
* `--type <string>`: The type of the key file (e.g., s2fsk)
* `--suffix <string>`: The right hand part of the key name (keys.projecatara.com)
* `--signature-algorithm <string>`: The name of the signing algorithm (e.g., rsa2048-sha256)
* `--format <string>`: The naming format for keys (standard | es3)
* `--passin pass:<string>`: Use the specified string as the passphrase
* `--passin stdin`: Read the passphrase from standard input
* `--passin prompt`: Prompt for password (this is default behaviour). You can use ^C to exit this if needed
* `--retry`: If `-passin prompt' is specified, exit with an error status if the password is invalid. If not specified, then it will re-prompt for a valid password.
* `--check`:
* `-v`: Verbose mode
* `<tftf-file>...`: One or more TFTF files to display


### create-ffff
**create-ffff**
--flash-capacity <num>
--erase-size <num>
--length <num>
--gen <num>
--out <file>
{--header-size <num>}
{--name <string>}
{-v | --verbose}
{--map}
{--header-size <num>}
[[<element_type> <file>] {<element_option>}]...

* `--flash-capacity <num>`: The capacity of the Flash drive, in bytes.
* `--erase-size <num>`: The Flash erase block granularity, in bytes.
* `--length <num>`: The size of the image, in bytes.
* `--gen <num>`: The header generation number (must be bigger than the generation number of what is currently on the Flash).
* `--out <file>`: Specifies the output file.
* `--header-size <num>`: The size of the generated FFFF header, in bytes (default is 4096).
* `--name <string>`: Flash image name.
* `-v | --verbose`: Display the FFFF header and a synopsis of each FFFF section
* `--map`: Create a map file of the FFFF headers and each FFFF sections
* `--header-size <num>`:

Each element (`[<element_type> <file>] {<element_option>}`)is described with an element type and options.

**Element types:**

* `--s2f <file>`: Stage 2 Firmware file.
* `--s3f <file>`: Stage 3 Firmware file.
* `--ims <file>`: Internal Master Secret (IMS) certificate file
* `--cms <file>`: CMS certificate file.
* `--data <file>`: Generic data file.

**Element Options:**

* `--element-class`: The element's ID number.
* `--element-id`: The element's ID number.
* `--element-generation`: The element's generation number.
* `--element-location`: The element's absolute location in Flash (must be a multiple of --erase-size).
* `--element-length`: (Optional) The element's length. If ommitted, the length is extracted from the file.

### display-ffff
**display-ffff**
{-v}
{--map}
{--explode}
`<ffff-file>`...

* `-v`: Verbose mode, in which the script will dump the contents of the FFFF headers, contained TFTF headers and TFTF sections in greater detail
* `--map`: Generate a .map file of the FFFF field offsets
* `--explode`: (Deprecated) Saves FFFF elements in separate files with the same root name
* `<FFFF-file>...`: One or more FFFF files to display

### nuttx2ffff
**nuttx2ffff**
{-v}
{--build=name}

* `-v`: Verbose mode, in which the script will dump the contents of the FFFF headers, contained TFTF headers and TFTF sections in greater detail
* `--build`: Which nuttx image to use: ara-bridge-es2-debug-apbridgea | ara-bridge-es2-debug-generic | (ara-svc-db3) | ara-bridge-es2-debug-bringup | ara-svc-bdb2a | ara-svc-sdb

## Libraries you must load before use

### Python
The `create-dual-image` script requires [pyelftools](https://github.com/eliben/pyelftools) to use its `--elf`
flag, which can be installed via:

    sudo pip install pyelftools

### C
FTDI libraries
(See: https://learn.adafruit.com/adafruit-ft232h-breakout/mpsse-setup for an explanation and links to the installation scripts)

* **D2xx** drivers/ibraries (see: FTDI [AN 220](http://www.ftdichip.com/Support/Documents/AppNotes/AN_220_FTDI_Drivers_Installation_Guide_for_Linux%20.pdf) for download and install instructions)
* **LibMPSSE_SPI** There doesn't appear to be a place from which one can install the libraries. So, the current approach is to download the source from the [FTDI site](http://www.ftdichip.com/Support/SoftwareExamples/MPSSE/LibMPSSE-SPI.htm) and rebuild the libs. Follow the link at the bottom of the text "The source code for the LibMPSSE-SPI library is available for download here", and and unzip the dowloaded file to your home directory. Once unzipped, cd ~/LibMPSSE-SPI_source/LibMPSSE-SPI/LibMPSSE/Build/Linux and type "make"

# Examples

## Example 1: packaging a [nuttx](https://github.com/projectara/nuttx) firmware into a TFTF image
The following command packages a nuttx firmware specified in two raw-binary parts,
one of which has a nontrivial linking offset, into a TFTF image.  Assume that `~/nuttx-es2-debug-apbridgea.text`
contains the `.text` section of the firmware (with an offset of 0), and that `~/nuttx-es2-debug-apbridgea.data`
contains the `.data` section of the firmware (which begins 0x1e6e8 after the base loading address in memory).  We want to load the firmware as a whole to the base address 0x10000000, and the `.text` section's entry-point is at 0x10000ae4.

The Unipro and Ara VID/PID values are specific to the chip on which we intend to run the firmware.

All numerical parameters can be passed in decimal or hexadecimal form, and are here given in hex for convenience.

    ./create-tftf -v --code ~/nuttx-es2-debug-apbridgea.text --out ~/nuttx-es2-debug-apbridgea.tftf \
    --data ~/nuttx-es2-debug-apbridgea.data --offset 0x1e6e8 \
    --load 0x10000000 --start 0x10000ae4 \
    --unipro-mfg 0x126 --unipro-pid 0x1000 --ara-vid 0x0 --ara-pid 0x1

The flags can be understood as follows:

* `-v`: Verbose mode, in which the script will dump the contents of the resultant TFTF headers to stdout when finished.
* `--code`: Specifies the filename in which the raw binary for a code section can be found.  This should be
the Ara firmware's `.text` section.
* `--out`: Specifies the filename to which the TFTF image should be written.
* `--data`: Specifies the filename in which the raw binary for a data section can be found.  This should be
the Ara firmware's `.data` section.
* `--offset`: An offset to be added to the loading address (see below) to find where to load the immediately preceding
section (which must be one of: `--code`, `--data`, or `--manifest`) when loading its contents to memory at boot-time.
* `--load`: The base address in memory to which the firmware sections packaged in the TFTF should be loaded at
boot-time, as obtained from a disassembler or linker.
* `--start`: The absolute memory address of the firmware's entry-point, as obtained from a disassembler or linker.
* `--unipro-mfg`: The Unipro Manufacturer ID (MID)/Vendor ID (VID) (these are two different phrasings for talking about the same number).  The specific value is obtained from the relevant hardware.
* `--unipro-pid`: The Unipro Product ID (PID).  The specific value is obtained from the relevant hardware.
* `--ara-vid`: The Project Ara Vendor ID (VID).  The specific value is obtained from the relevant hardware.
* `--ara-pid`: The Project Ara Product ID (PID).  The specific value is obtained from the relevant hardware.

At least one section must be given via `--code`, `--data`, or `--manifest`, and an output filename via `--out` is also mandatory.

## Example 2: packaging [nuttx](https://github.com/projectara/nuttx) TFTF into an FFFF image
The following command packages the TFTF image from Example 1 into an FFFF image,
designated for a flashrom with 2 MB (megabytes) of capacity and considered to be a first-generation FFFF header.

The `--flash-capacity` and `--erase-size` parameters take values specific to the hardware for which we are building
firmware.

All numerical parameters can be passed in decimal or hexadecimal form, and are here given in hex for convenience.

    ./create-ffff -v --flash-capacity 0x200000 --image-length 0x28000 --erase-size 0x1000 \
    --name "nuttx" --generation 0x1 \
    --s2f ~/nuttx-es2-debug-apbridgea.tftf --eloc 0x2000 --eid 0x1 \
    --out ~/nuttx-es2-debug-apbridgea.ffff

The flags can be understood as follows:

* `-v`: Verbose mode, in which the script will dump the contents of the resultant FFFF and TFTF headers when finished.
* `--flash-capacity`: The total capacity of the target flashrom, in bytes.
* `--image-length`: The length of the output FFFF image, in bytes.
* `--erase-size`: The length of an erase-block in the target flashrom device, in bytes.
* `--name`: The name being given to the FFFF firmware package.
* `--generation`: The per-device FFFF generation of the output image.  Used to version firmware images.
* `--s2f`: Specifies the filename of a TFTF package for Ara "*S*tage *2* *F*irmware".
* `--eloc`: The absolute address, within the FFFF image and flashrom address-space, at which the preceding element (here the `--s2f` element) is to be located, in bytes.  `--eloc` should be read as "Element Location".
* `--eid`: The *e*lement *id*entifier, one-indexed.
* `--out`: Specifies the filename to which to write the resultant FFFF image.

## Example 3: packaging a [nuttx](https://github.com/projectara/nuttx) ELF binary into a TFTF image

This example proceeds in exactly the same way as Example 1, except that instead
of passing raw binary files for the firmware's `.text` and `.data` sections,
necessitating the manual passing of loading offsets, we instead pass a
[nuttx](https://github.com/projectara/nuttx) ELF executable to the `--elf` flag,
and let the script extract the `.text` and `.data` sections and offsets from the
ELF header.

    ./create-tftf -v --elf ~/nuttx-es2-debug-apbridgea \
    --start 0x10000ae4 \
    --out ~/nuttx-es2-debug-apbridgea.tftf \
    --unipro-mfg 0x126 --unipro-pid 0x1000 --ara-vid 0x0 --ara-pid 0x1

The flags differing from Example 1 can be understood as follows:

* `--elf`: Specifies the filename in which an ELF executable can be found.

Note that in this case, a `--load` flag is *not* supplied, and the loading
address is thus thus taken from the `.text` section's ELF section header.
Likewise, the value of the `--start` flag can also be replaced with the ELF
header's specified entry point, although we *have* supplied `--start` here since
we wish to specify a non-default entry point.

## Example 4: packaging a [nuttx](https://github.com/projectara/nuttx) FFFF and a [bootrom](https://github.com/projectara/bootrom) into a "dual image" for ES2 hardware

In this example, we hack the FFFF specification to package a
[bootrom](https://github.com/projectara/bootrom) *alongside* our FFFF image,
in the early portion of the binary.  This works only because the FFFF
specification expects to see the first FFFF header element at
address 0 in the flashrom.  If we instead place the
[bootrom](https://github.com/projectara/bootrom) there, the FFFF loader will
just assume the first FFFF header was corrupted and search for the "second" FFFF
header in progressively higher addresses in flashrom.  It will then find the
actual *first* FFFF header of our image (`~/nuttx-es2-debug-apbridge.ffff` in
this example), and load in accordance with that header.

This basically **only** exists for testing purposes, and should **never** be
done in any kind of production environment, as it subverts the spirit of FFFF
generation numbering.  Unfortunately, this inevitably means someone will try
it :-).  *C'est la vie.*

    ./create-dual-image --ffff ~/nuttx-es2-debug-apbridgea.ffff \
    --bootrom ~/bootrom/build/bootrom.bin \
    --out ~/es2-apbridgea-dual

The flags can be understood as follows:

* `--ffff`: Specifies the filename of the FFFF image to corrupt.
* `--bootrom`: Specifies the filename of the raw
[bootrom](https://github.com/projectara/bootrom) to insert into the low
addresses of the image.
* `--out`: Specifies the filename into which the output hack-image should be
written for testing purposes.

# Scripts for Building and Packaging Drops to Toshiba
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
* **makef4** (called from makeall) compiles the L2FW and L3FW with the desired
  configuration and creates the FFFF.bin. It has an option to compile only
  L3FW and load it as L2FW to speed simulation.
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
**CONFIG_CHIP**, which should be one of these, without the leading dash.
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
* **-stby** Enable standby-mode tesing
* **-stby-wait-svr** Will wait for the server in standby mode
* **-stby-gbboot** Enable GBBoot server standby mode
* **-spec `<num>`** Compile special test number `<num>`.
(This appears to the code as the _SPECIAL_TEST define, the value
* **-342** Substitute the L3FW for L2FW to speed simulation.

### Parameter Sets by Tool
* **makedrop** {-v} {-342}
* **makeall** {-es2tsb | -es3tsb | -fpgatsb } {-v} {-justboot} {-zip}
{-nocrypto} {-gearchange} {-debugmsg} {-handshake}
{-stby} {-stby-wait-svr} {-stby-gbboot} {-spec `<num>`} {-342}
{-justboot}
* **makeboot** {-es2tsb | -es3tsb | -fpgatsb } {-v} {-dbg} {-prod}
{-nocrypto} {-gearchange} {-debugmsg} {-handshake}
{-stby} {-stby-wait-svr} {-stby-gbboot} {-spec `<num>`} {-342}
* **makef4** {-es2tsb | -es3tsb | -fpgatsb } {-sign} {-v} {-dbg} {-prod}
{-nocrypto} {-gearchange} {-debugmsg} {-handshake}
{-stby} {-stby-wait-svr} {-stby-gbboot} {-spec `<num>`} {-342}
* **makef4norm** {-es2tsb | -es3tsb | -fpgatsb } {-sign} {-v}
{-handshake} {-stby} {-stby-wait-svr} {-stby-gbboot} {-spec `<num>`} {-342}
* **make3sim** {-es2tsb | -es3tsb | -fpgatsb } {-sign} {-dbg} {-v} {-prod}
{-spec `<num>`} {-342}


## Environment Variables
The test-drop and delivery tools (found in .../bootrom-tools/scripts)
require the following environment variables be set in order to function:

Variable | Nominal Value | Purpose
-------- | ------------- | -------
BOOTROM_ROOT | ~/work/bootrom | The bootrom repository
KEY_DIR | ~/bin | Your local source of .pem files (used by makef4 to sign the TFTF)
DROP_ASSEMBLY_DIR | ~/bootrom-tools/es3-test | Where test drop components are created
TEST_DROP_DIR | ~/es3-test | The target folder for a given test drop
DELIVERY_NAME | es3-bootrom-delivery | The name of a delivery (also used in creating the delivery folder)
DELIVERY_ROOT | ~ | Where the delivery folder will reside
