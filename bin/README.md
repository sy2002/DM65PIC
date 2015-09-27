How to flash the firmware on the DM65PIC
========================================

Get the latest firmware here:

https://github.com/sy2002/DM65PIC/bin

You need to have a ST-LINK/V2 device connected to any USB port of your PC,
MAC, Linux device and you need to the right cable that maps the pins from
the standard ST-LINK/V2 cable to fit to the DM65PIC JTAG interface.

Windows
-------

1. Go to www.st.com

2. Enter   STSW-LINK004   in the search box on the top right

3. Click on the first search result

4. Download, install and start the software

5. Choose "Connect" from the "Target" menu. If this is not working,
   then your STLINK-to-DM65PIC cable is wrong and/or DM65PIC is not powered.

6. The "Connect" takes about 5 seconds

7. Choose "Program & Verify" from the "Target" menu

8. Choose the dm65pic-firmware-rev1.hex file

9. Select "Verify after programming" and make sure that "Reset after
   programming" is checked.

10. Press "Start".

The firmware is now persistently flashed and will start as soon
as the DM65PIC is powered on.


Mac OSX or Linux
----------------

1. Get the tool OpenOCD from here http://openocd.org

2. For OSX users: If you use homebrew (http://brew.sh) then installing
   OpenOCD is very easy. Just enter

   `brew install openocd`

3. Copy the attached "dm65pic.cfg" somewhere, open a terminal and go to
   the folder where you copied dm65pic.cfg

4. Enter:

   `openocd --file dm65pic.cfg`

   If any error occurs, then your cable STLINK-to-DM65PIC is wrong and/or
   DM65PIC is not powered.

5. Executing (4) creates a local telnet server on port 4444, so open another
   terminal window and enter

   `telnet localhost 4444`

6. Enter

   `halt`

7. Enter

   `flash write_image erase <fullpath-to-the-hex-file>`

   In the <fullpath-to-the-hex-file> no things like "~" are allowed.
   Use really the full path starting from the root folder.

8. If an error occurs,  enter:

    ```
    halt
    reset
    halt
    ```

9. And then try (7) again.

10. If it says something like "wrote xyz bytes from file abc" then enter

   `reset`

   (reset starts the firmware)

11. Enter:

    `exit`

12. You can now close all terminal windows, including the OpenOCD server


The firmware is now persistently flashed and will start as soon
as the DM65PIC is powered on.

