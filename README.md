MotorMeter_Version_2
====================

This is the source code for the new and improved Motor Meter.  
What's new:
- Much faster response to user inputs thanks to:
  - A video driver update
  - Change from I2C to SPI communications with the display module
- In Set Spd mode you can now toggle the PWM output on and off by pressing the Select button, allowing you to set the desired speed first before enabling the motor
- Modes are now entered AND exited using the Mode button for a more intuitive user interface
- The standard/extended range jumper has been replaced with Select button logic 
  - Hold the Select button for about 1 second to go into extended servo drive range (500ms to 2500ms)  
  - An asterisk appears in the upper left hand corner to indicate extended range is enabled

To prevent library compatability issues the library source files will be included here. The previous motor meter project just pointed to the library locations elsewhere but that meant you might end up using a version of the library that is significantly different from the library used for the production units.

The video library is a custom modification to the Adafruit SSD1306 library based on help from Adafruit's software guy.  This was done to resolve a servo twitch problem caused by interrupt conflicts between the servo.h library and the SSD1306 display library.  Only use this modified library from this project.  Failure to do so will cause random PWM spikes that will cause unexpected speed/direction changes to your motor.
