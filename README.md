
This is a modified version of u-boot specifically used for installing fread.ink onto 5th generation Kindle devices. It is based on the u-boot 2009.08 found in the [5.1.2 Kindle GPL release](https://s3.amazonaws.com/kindle/Kindle_src_5.1.2_1679530004.tar.gz) on the official [Amazon GPL download page](https://www.amazon.com/gp/help/customer/display.html?nodeId=200203720) but it has been stripped down to only provide basic [fastboot](https://en.wikipedia.org/wiki/Android_software_development#Fastboot) functionality while still fitting into the 72 kB of OCRAM available during early boot (before external RAM initialization).

This is based on a partial fastboot patch [provided by Eureka on the mobileread forums](https://www.mobileread.com/forums/showthread.php?p=2272493).

# Compiling

Make sure you have the appropriate compiler and tools installed:

```
sudo apt install build-essential gcc-4.8-arm-linux-gnueabihf 
```

To compile:

```
TYPE=prod # see u-boot-2009.08/board/imx50_yoshi/config.mk
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf- # change this if you want to use a different cross-compiler

make imx50_yoshi_config
make all
```

The output file you want will be called `u-boot.bin`.

## Recompiling

If you change anything, before recompiling do:

```
make clean
make distclean
rm -rf u-boot.*
```

# Precompiled binary

You can find a precompiled version under [Releases](https://github.com/fread-ink/fread-installer-u-boot/releases).

# Testing using USB loader

You can run this u-boot without flashing it onto your device.

Hold down the power button on the Kindle for 20 seconds. Note that the light next to the power button will turn off at _15 seconds_. Now press and hold the magic button for USB download mode. For the Kindle 5th generation non-touch this is the down button on the five-way controller. Release the power button, wait one or more seconds, then release the down button.

Nothing will change on the screen nor will you see any output via the serial terminal but if you connect via USB and run `lsusb` you will see it show up as a Freescale device rather than the usual Amazon/Kindle device.

Now install the [imx_usb_loader](https://github.com/fread-ink/imx_usb_loader) tool from Boundary Devices:

```
sudo apt install libusb-1.0-0-dev
git clone https://github.com/fread-ink/imx_usb_loader
cd imx_usb_loader/
make
```

Load the compiled `u-boot.bin` using:

```
sudo ./imx_usb ../path/to/u-boot.bin
```

If you have a serial console for your Kindle, you will now see output from u-boot as it initializes.

# Using fastboot

Once this u-boot is loaded you can use [a special version of the fastboot client](https://github.com/fread-ink/Fastboot-Kindle) made by [yifanlu](https://github.com/yifanlu).

Download and compile:

```
git clone https://github.com/fread-ink/fread-installer-fastboot
cd fread-installer-fastboot/
make linux
```

Check that the fastboot client can talk to the fastboot server on the device:

```
sudo ./fastboot getvar serial
```

This should print the serial number of you Kindle.

Now you can flash a file onto a partition using:

```
sudo ./fastboot flash <partition> <filename>
```

# Original readme

The original readme that shipped with this version of u-boot can be found in README.uboot

# License

GPLv2

# Disclaimer

Kindle is a registered trademarks of Amazon Inc. 

E Ink is a registered trademark of the E Ink Corporation. 

None of these organizations are in any way affiliated with fread nor this git project nor any of the authors of this project and neither fread nor this git project is in any way endorsed by these corporations.
