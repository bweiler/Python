README

This is how to compile the Python library to use the Bluegiga BLE USB Dongle, Part# BLED112.
You can get the Dongle on Amazon or Digikey.

This is compiled on Windows and Linux by typing: "python setup.py build"

On Windows, Python and the mingw64 gcc compiler have to be installed. You can also just
use the library as is, without having to compile it.

The compiler outputs the library at:

./Bluegiga/build/lib.win-amd64-3.7/bluetooth.cp37-win_amd64.pyd

To use it in Python, it should be in the directory you launch Python from, and can be used, for example:

import bluetooth

b = bluetooth
b.sendCMD(0x10)