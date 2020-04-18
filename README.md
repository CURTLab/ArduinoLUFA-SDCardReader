# ArduinoLUFA-SDCardReader
USB SD Card Reader using the Arduino Micro and a micro SD card. 

![Preview](https://github.com/CURTLab/ArduinoLUFA-SDCardReader/blob/master/Preview.jpg)

The software is based on the Arduino-Lufa library (https://github.com/Palatis/Arduino-Lufa).
Use the instructions from the Arduino-Lufa library to activate the library. Run the python script ```activate.py``` to override the native USB support from Arduino otherwise it will not work. To have the native USB Serial run the ```deactivate.py``` script.

The Arduino Micro has 5V logic, but SD cards needs 3.3V logic therefore it is imported to use a Level Converter between the Arduino and the SD card.

The SD card driver is based on the Arduino Sd2Card Library (Copyright (C) 2009 by William Greiman) under GNU General Public License and optimized to use as low memory as possible.

I tested it with a Transcend 4GB MicroSDHC card. Others should also work but maybe the SPI speed needs to be adapted (```LUFAConfig.h``` file). I used a USB to Serial adapter to debug the code (Serial1 and enable the ```SDCARD_DRIVER_DEBUG``` define in ```LUFAConfig.h```) since the native Arduino Serial is deactivated. In addition the auto reset routine is deactivated, therefore for flashing the Arduino you need to do it manual using the RST button.


