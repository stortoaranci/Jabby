# Intro
This is my first project on github and it's also my first attempt to learn how the platform works so the primary goal here is to taste the feeling with the Community.

In my opinion, _JABLOTRON OASIS 80_ and in particular the _JA-83K_ board is a masterpeace in the world of alarm systems. I have installed one several years ago and it's still up-and-running without issue.
I **never** had a false positive so far and i don't intend to substitute this appliance in a short-medium term. However this technology doen't support _Home Assistant_ so i moved on in creating an IoT device to cover this task.

## Roardmap
The time to spend on the project is not so much but in the future i would like to extend the actual implementation to cover the following features ordered by priority:
- **Add security**. MQTT communication is not encrypted and since some of my IoT devices at home don't support MQTT over SSL the next step should be adding [AES support](https://hackaday.com/2017/06/20/practical-iot-cryptography-on-the-espressif-esp8266/) in sending and receiving sensible data over the network. A suitable solution is needed for the console too. The solution maybe in creating a custom client that runs over _nc_ or _PuTTY_ and implementing something similar to AES support.
- **Add a sensor** to extend the functionalities provided by _MQTT Alarm Control Panel support_ of _HA_. In particular it's possible to catch the messages sent by the control panel allowing smarter automations. For instance, it could be possibile to use the alarm sensors to serve other automations like presence detection or human activities in a room.
- **GSM support**. The hardware is able to use AT commands to drive a GSM/GPRS modem. The aim is to access Jabby by phone. The idea behind this feature is because i would like to move one of my appliances in my cellar, where my wine are safety stored :) but there there is no access to any suitable Wi-Fi. This feature is not intended as a subtitution of the _JA-82Y GSM Communicator_. I don't want to reinvent the wheel.
