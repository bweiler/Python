README

Need - 
1) Adafruit Feather nRF52 mRF52832
2) micro USB cable
3) Adafruit Feather libraries installed
4) Python installed
5) Github code in "Python" repository

So far tested on Windows.

Github code:
1) test_ada.py - python file for communicating over the serial port to the Adafruit Feather nRF52
2) .\AdafruitFeatherCentral - directory that has Adafruit Feather Arduino code for talking to Skoobot

To use this code:

1) Install Python
2) Install PySerial, MatplotLib, NumPy, (these may installed by default)
-you install from with the pip program like "pip install numpy"
-you can jump ahead and install OpenCV and Tensorflow
3) Plug in Adafruit Feather nRF52
4) Follow Adafruit's directions to paste their Json file into Arduino IDE to load their libraries
5) Download the Arduino code and Python code form my Github
6) Load the Arduino code on the Feather
7) Turn on Skoobot, give it a few seconds to connect
8) run test_ada.py from the command line, I use Windows Powershell, but dos window is great
"python test_ada.py"
-you may have to find the serial port that installed on your machine and update it in the code, mine is always 'COM14'

Gotchas:
1) If Arduino or Python have the serial port open, the other can't open it. If you open Arduino Console, can't run Python. Kind of tricky until you get used to it.
2) Feather is programmed to connect to Skoobot when Skoobot is turned on, very fast.
3) Python serial port open is kind of slow, like 3-5 seconds. I have 5 second delay with timer.sleep after calling open.
4) If you have problems, retry it, or power cycle dongle, or press reset button on Feather and then power cycle Skoobot.
