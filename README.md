# Firmware for the DM65PIC
MEGA65 is an open-source new and open C65-like computer. Hardware designs and
software are open-source (LGPL).
Please visit www.mega65.org to learn more about it.

The DM65PIC is the peripheral interface controller for the MEGA65 computer.
It is based on an STM32 F4 microcontroller. The hardware design was done by
doubleflash using [Eagle](http://www.cadsoftusa.com), it can be found in
the `eagle` folder.

This GitHub repository is containing the firmware of the DM65PIC done by sy2002.

You can see a combination of MEGA65 FPGA prototype (based on a Nexys 4 DDR
board), a DMPIC65 prototype and a very first work-in-progress version of the
firmware in conjuncton with a real Commodore 65 prototype keyboard in action
on YouTube: [https://youtu.be/5PpsEw80j3M](https://youtu.be/5PpsEw80j3M).

## Folder structure
```
00-STM32F4xx_STANDARD_PERIPHERAL_DRIVERS  STM32 CMSIS and standard drivers
00-STM32F429_LIBRARIES                    Tilen Majerle's STM32F4 library
```
