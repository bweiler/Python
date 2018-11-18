README.txt for Bluegiga Bluetooth Python library

Python library to use the Bluegiga BLE USB Dongle, Part# BLED112.
You can get the Dongle on Amazon or Digikey.

You don't have to compile it unless it doesn't work on your system or you want to change it.

Windows: bluetooth.cp37-win_amd64.pyd
Raspberry Pi:

To compile type: "python setup.py build"

Windows compile installs:
1) Python
2) mingw64 gcc compiler
3) create a file called distutils.cfg in this location: (your user home directory)\AppData\Local\Programs\Python\Python37\Lib\distutils
4) Edit the file contents (otherwise it defaults to Visual Studio): 
[build]
compiler=mingw32

Disutils outputs the library at:
./Bluegiga/build/lib.win-amd64-3.7/bluetooth.cp37-win_amd64.pyd

Raspberry Pi compile installs:
1) Python
2) 