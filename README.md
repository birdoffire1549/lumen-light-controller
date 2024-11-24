# Lumen Light Controller

This firmware is intended to be used for a lighting controller I refer
to as the 'Lumen Lighting Controller'. The original intent of this controller is
to simply control some under cabnit white LED lights, for use at my mom's house.
  
This controller remembers the last setting of on/off for the lights. This means if
you turn the lights on, then un-plug the controller. The next time you plug it in
the lights will default to on. This is how my mom originally wanted to control the 
on/off status of the lights. I decided to add an external button that could be used
to turn on/off the lights so she wouldn't be unplugging it all the time. However since
the device was an ESP-8266 I decided not to stop there, so I made a web interface that
can control the lights, and from that interface one can also setup a simple daily timer 
that automatically turns the lights on/off, if the device is connected to a network
with Internet access so it can fetch the time via NTP.

The device remains in AP mode even when connected to a network so that there is an easy to
find way of connecting to the device to change it's settings. Once connected to the AP which
will be an SSID starting with 'Lumen_'. One can open the internally hosted web page by typing
in any HTTP address in a browser. 
  
For example: http://lumen.local

From there all functionality of the device can be leveraged.

Aside from the web interface for controlling the firmware is designed so the device can have an external
on/off button for toggling the lights on/off. Also, a button can be included to act as a factory reset 
button. That button if held down when the device powers up will reset the device instantly. To reset the device 
once it is powered up the button must be held down for 10 seconds or more.

Hardware: ...... ESP8266
Written by: .... Scott Griffis
Date: .......... 11/24/2024
