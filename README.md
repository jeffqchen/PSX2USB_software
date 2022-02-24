# PSX2USB_software

PSX2USB_software for https://github.com/jeffqchen/PSX2USB_hardware

Currently I'm using the following libraries for the two sides of the communication:
- https://github.com/MHeironimus/ArduinoJoystickLibrary
- https://github.com/SukkoPera/PsxNewLib

Pin definition:
PS Controller Pin | Arduino Pin
-----------|----------
6 - Attetion|10 - A10
2 - Command|16	- MOSI
1 - Data|14 - MISO
7 - Clock|15 - SCLK
