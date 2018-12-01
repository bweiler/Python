README

Need: Adafruit Feather nRF52 mRF52832, micro USB cable, Python installed
So far tested on Windows.

test_ada.py - python file for communicating over the serial port to the Adafruit Feather nRF52

.\AdafruitFeatherCentral - directory that has Adafruit Feather Arduino code for talking to Skoobot

To use this code:

1) Install Python
2) Install PySerial, MatplotLib, NumPy, (these may installed by default)
-you install from with the pip program like "pip install numpy"
-you can jump ahead and install OpenCV and Tensorflow
3) Download the Arduino code
4) Turn on Skoobot, give it a few seconds to connect
5) run test_ada.py from the command line
"python test_ada.py"
-you may have to pick a serial port that works on your machine, mine is always 'COM14'