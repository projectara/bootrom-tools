# bootrom-tools: Packaging Images for Ara Bridges
This repository contains sofware tool (i.e, Python scripts and C-programs) for
packaging firmware
images into Project Ara's Trusted Firmware Transfer Format (TFTF) and Flash
Format For Firmware (FFFF) file formats. These file formats and the basic
set of tools used with them are described in
[ES3 Bridge ASIC Stage 1 Firmware High Level Design](https://docs.google.com/document/d/1OxjQClSY5cvS370XG9xH7FR9DZgRGv17LXE33EEXSxE/edit) document.

The FFFF file is a filesystem romimage, which is composed of filesystem headers and
TFTF elements. The TFTF elements typically come from the build system, but can also
contain data, certificates, etc.

## Getting Started
**Before you start**: Your environment must be set up
correctly for the tools to function. (See: **Appendix A: Libraries**
and **Appendix B: Environment** below.)


### Automatically Package Your Nuttx Image

Since most Ara development consists of building and testing new nuttx images,
you would use the following command to package your nuttx image (in $NUTTXROOT)
into a flashable image:

    nuttx2ffff

This will create an unsigned (aka "untrusted") image in the same folder as the nuttx executable. If you want to package a specific build image in `$NUTTXROOT/build/<your_build>/images`, use:

    nuttx2ffff --build=your_build


### Manually Packaging Your Nuttx Image
If *nuttx2ffff* doesn't meet your needs, you can manually run through the general process flow, outlined below:

1. Create element content (e.g., compile nuttx)
2. Package the element(s) into TFTF files with create-tftf
3. Optionally Sign the element(s) with sign-tftf (not used in initial
development, but needed for creating trusted images towards the end)
4. Create a new romimage containing the element(s) with create-ffff
5. Re-flash the bridge with the ffff file.

The tools and their parameters are described briefly below, in roughly the order
in which one would use them.

* **create-tftf** Assemble content blobs into a TFTF file
* **display-tftf** Display the contents of a TFTF file
* **sign-tftf** Cryptographically sign the contents of a TFTF file with
the specified private key.
* **create-ffff** Assemble one or more TFTF blobs into an FFFF file
* **display-ffff** Display the contents of an FFFF file

(See *Appendix C: Tools* for details on parameters and
*Appendix D: Related Documents* for additional tools specific
to bootloaer generation)

Each tool will list its parameters if it is called with the flag `--help`.

#### Packaging a [nuttx](https://github.com/projectara/nuttx) ELF binary into a TFTF image

This example packages the ELF format executable ([nuttx](https://github.com/projectara/nuttx)) into a TFTF file: nuttx.tftf.

proceeds in exactly the same way as Example 1, except that instead
of passing raw binary files for the firmware's `.text` and `.data` sections,
necessitating the manual passing of loading offsets, we instead pass a
[nuttx](https://github.com/projectara/nuttx) ELF executable to the `--elf` flag,
and let the script extract the `.text` and `.data` sections and offsets from the
ELF header.

    create-tftf --elf ~/nuttx/nuttx/nuttx \
      --type s2fw \
      --header-size 0x200 \
      --name "NuttX S3FW-as-S2FW" \
      --elf ~/nuttx/nuttx/nuttx \
      --out ~/nuttx/nuttx/nuttx.tftf

Breaking the flags down:

* `--type`: Specifies that this is a Stage-2 Firmware image. [1]
* `--header-size`: Specifies the size of the TFTF header (must be
512 bytes for S2fw).
* `--name`: Specifies the image name. (This is entirely optional, but a
good practice to get into. The name will be truncated if needed to fit
in the header's 48 byte name field.)
* `--elf`: Specifies the filename of the ELF executable to package.
* `--out`: Specifies the output file, which can be anything, anywhere.

**Notes**
1. The normal boot sequence is the stage 1 firmware (aka ROM bootload) loads a more sophisticated stage 2 firmware loader which loads the module (stage 3) firmware. At the time of writing (2015/12/13), a real S2FW loader has not been written, so you can simply specify your S3FW nuttx image as the S2FW.
2. All numerical parameters can be passed in decimal or hexadecimal form, and are here given in hex for convenience.


#### Signing the TFTF
You can skip this step for now - we'll fill in the details later.


#### Packaging the TFTF into an FFFF Romimage
The following command packages the TFTF image created above into an FFFF romimage,
designated for a flashrom with 2 MB (megabytes) of capacity and considered to be a first-generation FFFF header.

The `--flash-capacity` and `--erase-size` parameters take values specific to the hardware for which we are building
firmware.


    create-ffff --flash-capacity 0x200000 --image-length 0x28000 --erase-size 0x1000 \
      --header-size 0x1000 \
      --flash-capacity 0x40000 \
      --image-length 0x40000 \
      --erase-size 0x800 \
      --name "Nuttx test build" \
      --generation 1  \
      --s2f ~/nuttx/nuttx/nuttx.tftf --eloc 0x2000 --eid 1 \
      --out ~/nuttx/nuttx/nuttx.ffff

The flags can be understood as follows:

* `--header-size`: Specifies the size of the FFFF header (must be 4k)
* `--flash-capacity`: The total capacity of the target flashrom, in bytes.
* `--image-length`: The length of the output FFFF image, in bytes. (Must be <= --header_size.)
* `--erase-size`: The length of an erase-block in the target flashrom device, in bytes.
* `--name`: The name being given to the FFFF firmware package.n (As above, this is entirely optional, but good practice.)
* `--generation`: The per-device FFFF generation of the output image.  Used to version firmware images.
* `--s2f`: Specifies the filename of a TFTF package for Ara "*S*tage *2* *F*irmware".
* `--eloc`: The absolute address, within the FFFF image and flashrom address-space, at which the preceding element (here the `--s2f` element) is to be located, in bytes.  `--eloc` should be read as "Element Location".
* `--eid`: The *e*lement *id*entifier, one-indexed.
* `--out`: Specifies the filename to which to write the resultant FFFF image.


#### Resize the FFFF Romimage for Flashing
The flash programmer wants the flashable blob to be the same size as the target device.

    truncate -s 2M ~/nuttx/nuttx/nuttx.ffff


## Additional Examples
### Example 1: Packaging Non-ELF Format Firmware into a TFTF image
The following command packages a nuttx firmware specified in two
raw-binary parts, one of which has a nontrivial linking offset, into
a TFTF image.  Assume that `~/nuttx-es2-debug-apbridgea.text` contains  
the `.text` section of the firmware (at location 0x10000000 in memory),
and that `~/nuttx-es2-debug-apbridgea.data` contains the `.data` section
of the firmware (which begins at an offset of 0x1e6e8 from the start of
the `.text` in memory). The `.text` section's entry-point is at 0x10000ae4.

Note that in this case, an `--elf` flag is *not* supplied, and instead
we must specify the code (text) and data files, where they are loaded
and the starting address. This example also demonstrates specifying
the Unipro MID/PID and Ara VID/PID values, which are specific to the chip on
which we intend to run the firmware (these default to zero when not specified).

    create-tftf \
      --header-size 0x200 \
      --type s2fw \
      --name "NuttX S3FW-as-S2FW" \
      --code ~/nuttx-es2-debug-apbridgea.text \
        --load 0x10000000 \
      --start 0x10000ae4 \
      --data ~/nuttx-es2-debug-apbridgea.data \
        --load 0x1001e6e8 \
      --out ~/nuttx-es2-debug-apbridgea.tftf \
      --unipro-mfg 0x126 --unipro-pid 0x1000 --ara-vid 0x0 --ara-pid 0x1

Most of the flags are covered in *Packaging a [nuttx](https://github.com/projectara/nuttx) ELF binary into a TFTF image* and are
not repeated here. The flags specific to this example follow:

* `--code`: Specifies the filename in which the raw binary for a code section can be found.  This should be the Ara firmware's `.text` section.
* `--data`: Specifies the filename in which the raw binary for a data section can be found.  This should be the Ara firmware's `.data` section.
* `--load`: The absolute memory address to which the firmware sections packaged in the TFTF should be loaded at boot-time, as obtained from a disassembler or linker.
* `--start`: The absolute memory address of the firmware's entry-point, as obtained from a disassembler or linker.
* `--unipro-mfg`: The Unipro Manufacturer ID (MID)/Vendor ID (VID) (these are two different phrasings for talking about the same number).  The specific value is obtained from the relevant hardware.
* `--unipro-pid`: The Unipro Product ID (PID).  The specific value is obtained from the relevant hardware.
* `--ara-vid`: The Project Ara Vendor ID (VID).  The specific value is obtained from the relevant hardware.
* `--ara-pid`: The Project Ara Product ID (PID).  The specific value is obtained from the relevant hardware.

At least one section must be given via `--code`, `--data`, or `--manifest`, and an output filename via `--out` is also mandatory.

### Example 2: packaging a [nuttx](https://github.com/projectara/nuttx) FFFF and a [bootrom](https://github.com/projectara/bootrom) into a "dual image" for ES2 hardware

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


## Appendix A: Required Libraries
### Python
The `create-tftf` and `create-dual-image` scripts require [pyelftools](https://github.com/eliben/pyelftools) to use its `--elf`
flag, which can be installed via:

    sudo pip install pyelftools
    
`sign-tftf` requires [pyopenssl](https://pypi.python.org/pypi/pyOpenSSL) (see [Installation](http://www.pyopenssl.org/en/latest/install.html):

    sudo pip install pyopenssl

### C

Other libraries:

* **libelf** sudo apt-get install libelf-dev (See: https://launchpad.net/ubuntu/+source/libelf)
* **openssl** sudo apt-get install libssl-dev (See: http://stackoverflow.com/questions/3016956/how-do-i-install-the-openssl-c-library-on-ubuntu)



## Appendix B: Environment
The bootrom-tools require the following environment variables be set
in order to function:

Variable | Typical Value | Purpose
-------- | ------------- | -------
BOOTROM_ROOT | ~/bootrom | The bootrom repository
CONFIG_CHIP | es3tsb | The target chip (es2tsb, es3tsb, or fpgatsb)

**PATH**: Because bootrom-tools grew organically, you need to ensure that
your PATH searches for them in the correct order:
${BOOTROM_TOOLS}/bin, ${BOOTROM_TOOLS}/scripts, ${BOOTROM_TOOLS}

## Appendix C: Tools
### create-tftf
**create-tftf**
--out `<tftf-file>`
--start `<num>`
--unipro-mfg `<num>`
--unipro-pid `<num>`
--ara-vid `<num>`
--ara-pid `<num>`
{--name `<string>`}
{--ara-stage `<num>`}
{--header-size `<num>`}
{--map}
{-v}
`<Section>`...

where `<Section>` is defined as:

[[[--elf `<file>`] | [--code `<file>` --load `<num>` --start `<num>`] |
[--data `<file>`] |
[--manifest `<file>`]]
{--class `<num>`} |
{--id `<num>`}]

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
--key `<file>`
--type `<string>`
--suffix `<string>`
--signature-algorithm `<string>`
--format [standard | es3]
{--passin [`pass:<passphrase> | stdin | prompt]`}
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
--flash-capacity `<num>`
--erase-size `<num>`
--length `<num>`
--gen `<num>`
--out <file>
{--header-size `<num>`}
{--name `<string>`}
{-v | --verbose}
{--map}
{--header-size `<num>`}
[`[<element_type> <file>] {<element_option>}]`...

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
`<ffff-file>`...

* `-v`: Verbose mode, in which the script will dump the contents of the FFFF headers, contained TFTF headers and TFTF sections in greater detail
* `--map`: Generate a .map file of the FFFF field offsets
* `<FFFF-file>...`: One or more FFFF files to display

### nuttx2ffff
**nuttx2ffff**
{-v}
`{--build=name}`

* `-v`: Verbose mode, in which the script will dump the contents of the FFFF headers, contained TFTF headers and TFTF sections in greater detail
* `--build`: Which nuttx.bin image to use: ara-bridge-es2-debug-apbridgea | ara-bridge-es2-debug-generic | ara-svc-db3 | ara-bridge-es2-debug-bringup | ara-svc-bdb2a | ara-svc-sdb. (It can also take the full path to any of the above.) It defaults $NUTTXROOT.nuttx when --build is omitted.

Generates *nuttx.ffff* in the same directory as the source *nuttx* file.

## Appendix D: Related Documents
* **README.md** This document.
* **README-Toshiba.md** Tools to create variants of the FFFF and bootrom
images for delivery and test drops. 
* **README-autotest.md** Describes the tools for loading and testing(ES3)
BootRom images with the HAPS-62 board.

