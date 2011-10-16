tlc5940chipkit
================================

This is a TLC5940 Library designed for Digilent's ChipKIT board

This is currently in alpha form

INSTALLATION
If a folder named libraries does not exist in your chipkit sketches folder, create one. Drop the Tlc5940 folder in the libraries folder. Restart mpide and you should be able to select Sketch > Import Library > Tlc5940. 

USAGE
This library is designed to mimic the arduino library for the most part. A pre-instantiated variable named Tlc is included for your use. To begin, call the init function with an initial value for all channels: Tlc.init(0); Next, set each channel value using: Tlc.set(channelNumber, brightnessValue); Lastly, call the update function to send the data to the TLC5940: Tlc.update(); Due to the asynchronous nature of how the data is sent and latched to the TLC5940, the update function may return before the TLC5940 has been completely updated. If you wish to wait until the update has completed you can follow the update function with this code: while (Tlc.updateInProgress()); which will block until the update process is completely finished.

WIRING
With the exception of Vcc, the TLC5940 should be wired to the chipkit in the same way that it would be wired to an arduino. Since the chipkit runs at 3.3V instead of 5V, the TLC5940 should be powered using 3.3V instead of 5V. Like the arduino, the LEDs themselves can be powered using any voltage up to 17V. It is recommended that you supply the LEDs with at least 5V.

POWER CONSIDERATIONS
USB ports can only provide 500mA to the chipkit. That is barely enough to power the chipkit (90mA) and a single TLC5940 (60mA) full of LEDs (16 * 20mA). The chipkit's voltage regulator has a maximum rating of 800mA, so attaching a power source to the chipkit's barrel connector will only allow you to daisy chain one additional TLC5940. For this reason, an external power source (with sufficient amperage) should be used to power the LEDs. See the diagram included in this project: breadboard-chipkit-tlc5940.png or https://github.com/ColinHarrington/tlc5940chipkit/blob/master/breadboard-chipkit-tlc5940.png


