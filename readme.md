## Overview
The idea of this project is to control a garden hose, presumably attached to a sprinkler, with an ESP8266 module over a private Wifi network. 

Once connected to your network, the ESP will serve a webpage that allows you to control the hose in various ways. This page will establish a Web Socket connection with the ESP which will send the page live updates to reflect the state of the system. (Note: I realize this is a bit of an overengineered solution and it stretches the capabilities of the ESP. But it works pretty nicely and it's fun to show off so what the heck...)

The served page allows three basic controls: ON/OFF, countdown timer, and daily timer. The countdown gives you a button that allows you to turn the system on for five minutes, after which it will automatically turn off. Pressing the button while the timer is running will add five minutes to the current timer, so for example you can easily set the timer for 15 minutes by pressing the button three times. The daily timer allows you to set the system to automatically turn on and off at specific times each day. Note that the main on/off switch allows you to override any active timer.

Another included feature is weather checking. To use this feature, you specify a number of days (up to five) and an amount of rain (in mm, up to 255). When the feature is enabled, it modifies the behavior of of the daily timer by disabling it if the forecast calls for the specified amount of rain over the coming specified number of days.

## Hardware
[Here](schematic.png) is the circuit I built, and these are the components I used.

* **NodeMCU ESP8266 module**
* **Solenoid valve** Specifically, [this](https://www.creatroninc.com/product/12v-solenoid-valve-3-4/?search_query=solenoid+valve&results=3) one. A similar one is available from Adafruit, but it's only 1/2", whereas standard garden hose thread is 3/4". In either case, you'll need a couple of adapters to go from the NPT to GHT (i.e. from the threads on the valve to the threads on the hose).
* **1N4001 diode** placed antiparallel across the solenoid.
* **12v power supply** I just used a wall-wart.
* **DC jack** to connect to to your power supply.
* **L7805CV voltage regulator** to step down the voltage for the controller. Technically, the voltage ranges for the NodeMCU (3.3v-10v) and the solenoid (6v-12v) overlap, so it should technically be possible to power them both from one power source. However, I couldn't find a 9v wall-wart with enough current, and the solenoid wouldn't work with the 6v one I tried. 
* **2x 10uF capacitors** across the input and output of the regulator
* **FQP30N06 MOSFET** used to switch the 12v for the solenoid with the ESP's 3.3v logic.
* **10k pull resistor**

Additionally, you'll want some spade connectors for the solenoid, female header for connecting the ESP module, perf/strip board to put it all together. 

## Setup
There are a few things you'll need to set up to use the control system.

### OpenWeatherMap API
The system makes use of the OpenWeatherMap API to obtain the forecast. However, using the API requires an key which can be obtained by creating a free account. You'll also need to determine your city's id. A list of city ids can be obtained [here](http://bulk.openweathermap.org/sample/city.list.json.gz).

### config file
In addition to the code provided, you'll need to create a file called config.h which should contain three variables: `char *ssid`, `char *password`, and `char *weather_url`. The first two are pretty self-explanatory, and the third should look something like this: `"http://api.openweathermap.org/data/2.5/forecast?mode=xml&id=???&APPID=???`. 

### Router
With the ESP module connected to your WiFi network, you'll want to go to your router's settings page to find the IP address of the module, which will be the URN of the HTML page which controls the system. If possible, have your router assign the ESP a reserved IP address.

## Issues
In my code and schematic, pin 10 is used. This is a bit of a problem because pin 10 briefly goes high when the module boots. The pull resistor was supposed to correct any undefined state at boot, but as it turns out, the state *is* defined--only wrong. As a result, the valve briefly opens (maybe .5 sec.) each time the module is powered on. This isn't a huge deal to me, so I haven't bothered to fix it, but an easy fix would be to change the switch pin in the code from pin 10 to pin 4 or pin 5 which are "safe" pins (i.e. they are low at startup), and change the schematic accordingly.