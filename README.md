# atari-debug-cart

(C) 2024 Christian Zietz

An unofficial firmware for the [SidecarTridge Multi-device](https://sidecartridge.com/products/sidecartridge-multidevice-atari-st/) to repurpose it as device for debug output from Atari software.

## How it works

Debug output is transmitted by _reading_ from the cartridge address space at 0xFBxxxx. Characters are encoded into address lines A8-A1. For example the following code snippet sends the character “A”:

```c
#define CARTRIDGE_ROM3 0xFB0000ul
(void)(*((volatile short*)(CARTRIDGE_ROM3 + ('A'<<1))));
```

To receive the debug output, a USB serial port is available at the Raspberry Pi Pico on the SidecarTridge Multi-device. On modern operating systems, no special drivers are required to access it. Any terminal program supporting serial ports can be used. As this is not a physical serial port, you can set any serial port baud rate.

To avoid ground current loops, it is recommended (but not mandatory) to connect a battery powered device such as a laptop computer running on batteries to the USB port of the Pi Pico when the  SidecarTridge Multi-device is plugged into the Atari.

## Benefits

* It is fast. An access to the cartridge port only takes a few CPU cycles. Thus, the timing of the program generating the debug output is much less impacted than with, e.g., MFP serial port output or even on-screen output.
* No hardware initialization needed. Access to the cartridge port is possible without any prior setup.

## How to install

The firmware is available as `debug.uf2` on the [Releases page](https://github.com/czietz/atari-debug-cart/releases).

Connect the Pi Pico installed to the SidecarTridge Multi-device to a computer via USB. Hold down the BOOTSEL button. Briefly press the RESET button, while still holding down the BOOTSEL button. A USB drive named RPI-RP2 should appear on your computer. Copy `debug.uf2` over to this drive. The Pi Pico will automatically reboot and the device will be ready for use.

Note that this procedure will overwrite the official firmware installed on your SidecarTridge Multi-device. Therefore, any functionality normally provided by the SidecarTridge Multi-device will not be available. You can always go back to the official firmware by installing `sidecart-pico_w*.uf2` in a similar way as described above.

## License and disclaimer

This firmware licensed under the [GNU General Public License, Version 3](https://www.gnu.org/licenses/gpl-3.0-standalone.html) (GPLv3). As stated in the license terms, the firmware is provided "AS IS" WITHOUT WARRANTY OF ANY KIND.

This project is not affiliated with the maker of the SidecarTridge, but merely reusing his hardware.

