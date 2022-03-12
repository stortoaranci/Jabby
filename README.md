# Jabby
Home Assistant - MQTT Alarm Control Panel support for JABLOTRON OASIS JA 80 (JA-83K)

## Intro
Jabby is a basic IoT device that allows any [JABLOTRON OASIS 80 (JA-83K)](https://www.jablotron.com/en/about-jablotron/downloads/?level1=2507&level2=2510&_level1=2507&_level2=&_level3=&do=downloadFilterForm-submit) to interact with _Home Assistant_ via _MQTT_.

It is built from inexpensive and easy to find hardware as:
- [WeMos D1 mini](https://www.wemos.cc/en/latest/d1/d1_mini.html) 
- Mini DC-DC Adjustable Step Down Power Supply (12v - 5v) like the [mini360](https://www.google.com/search?q=dc+dc+mini360)
- [MAX485 TTL To RS485 Converter Module Board](https://www.google.com/search?q=MAX485+TTL+To+RS485+Converter+Module+Board)
- [SIM900A](https://simcom.ee/modules/gsm-gprs/sim900/) compatible board for Arduino (optional)

and it can fit inside the alarm's control panel using the power coming from the main board.

Once the device is connected to the RS485 bus of the control panel, it will able to send and recevice data according to the propietary standard implemented by the manufacturer. However just few commands are implemented and even if in the future the idea is to extend the number of supported commands, this project is not intended to substitute the oridinal program _oLink_ designed for managing the control panel via usb on a regular PC. **The sofware is intended for educational and demonstration purposes only**.

## Features
### MQTT
Jabby implements the [MQTT Alarm Control Panel](https://www.home-assistant.io/integrations/alarm_control_panel.mqtt) interface provided by _HA_.

In particular the following commands are supported:
- ARM_AWAY (sets ABC)
- ARM_HOME (sets A only)
- ARM_NIGHT (sets B only)
- DISARM (unsets alarm)
- TRIGGER (triggers the alarm like any standard device normally does)

Configuring the device in _HA_ is easy as adding few rows in the HA **configuration.yaml** file:

```
#alarm example
alarm_control_panel:
  - platform: mqtt
    name: Jabby
    state_topic: "tr1/JabbyXXXXXXXXXXXX/alarm/state"
    command_topic: "tr1/JabbyXXXXXXXXXXXX/alarm/command"
    availability_topic: "tr1/JabbyXXXXXXXXXXXX/online"
    payload_available: "1"
    payload_not_available: "0"
```

where **JabbyXXXXXXXXXXXX** is the name dinamically calculated as unique id of the device.

### Entities
In addition to the _MQTT Alarm Control Panel_ support, Jabby also supports other sensors with states strictly related to the system. Configuring the entities in _HA_ is possibile as regular _MQTT Sensors_. For instance:

```
#alarm-system
  - platform: mqtt
    name: "jabby_message"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/message"
    qos: 0
    icon: mdi:message-alert-outline

  - platform: mqtt
    name: "jabby_device"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/device"
    qos: 0
    icon: mdi:leak

  - platform: mqtt
    name: "jabby_mode"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/mode"
    qos: 0
    icon: mdi:shield-lock-outline

  - platform: mqtt
    name: "jabby_armed"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/armed"
    qos: 0

  - platform: mqtt
    name: "jabby_triggered"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/triggered"
    qos: 0

  - platform: mqtt
    name: "jabby_activated"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/activated"
    qos: 0

  - platform: mqtt
    name: "jabby_delayed"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/delayed"
    qos: 0

  - platform: mqtt
    name: "jabby_warning"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/warning"
    qos: 0
    icon: mdi:alert-outline

  - platform: mqtt
    name: "jabby_battery"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/battery"
    qos: 0
    icon: mdi:car-battery

  - platform: mqtt
    name: "jabby_a"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/a"
    qos: 0
    icon: mdi:alpha-a-circle-outline

  - platform: mqtt
    name: "jabby_b"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/b"
    qos: 0
    icon: mdi:alpha-b-circle-outline

  - platform: mqtt
    name: "jabby_c"
    state_topic: "tr1/JabbyXXXXXXXXXXXX/c"
    qos: 0
    icon: mdi:alpha-c-circle-outline
```
can provide a result similar to [this](images/HomeAssistant_Triggered.jpg).

### Console
Jabby implements a very basic console for management and test purpouse. For instance it can be used to change networking paramters avoiding to reflash the chip or used for testing the RS485 traffic generated on the bus. A complete list of commands is reported in the file [help.txt](Jabby_sketch/data/help.txt).

The console layout is designed for UNIX-like systems but it can be accessed on Windows systems by using [PuTTY](https://www.putty.org/).

On Linux it's possible to reach the console by running the command:

`nc XXX.XXX.XXX.XXX YYYY`

where **XXX.XXX.XXX.XXX** is the IP address assigned to the device and **YYYY** is the TCP port of the service configured. 

On Windows it's possibile to access the console by setting up a **raw** connection in PuTTY with some customizing in the configuration file. In particular the **"Implicit CR in every LF"** option have to be checked under the category: **Terminal**. 

### OTA updates capability
The device can be updated without the need to physically access the control panel once the device is in place by serving a suitable binary file on a web server. Signed binaries are supported as well. To implement OTA updates please refer to the documentation provided by [ESP8266 Arduino Core](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html).

To avoid a possible brick of the device, Jabby checks for new versions of the firmware just after any restart. The update is done if any firmware different form the one installed is available online. The check is done by a string comparison between the filename proposed by the web server and the constant string **VERSION** without the extension suffix.

If the device is not able to access the Wi-Fi network anymore, then an attempt of connection is done by using the variables defined in the **secrets.h** file.

### GSM support
Jabby can be extended by connecting a SIMCOM SIM900 compatible board for future implementations. The idea is to provide remote access to Jabby. Any GPRS modem with TTL@3.3v on the RS232 can be adopted, such as old Nokia mobile phones.

At the moment, only the initial setup is implemented.

## Hardware installation
The easy way to install the device is to use the terminal of the control panel. Power can be provided by the _Backup Power Supply clamps (+U & GND)_. We suggest to take under control the _OVERLOAD_ indicator to avoid possible issue on the main board.

RS485 signals can be reached using the _A_ and _B_ clamps. Since the standard uses the same differential signaling, heaving a common ground is not mandatory so it's possible to place the device outside of the control panel and just connect the two wires on the serial bus. Bear in mind that the control panel doen't have a true heart connection _(this is just my experience)_ so if you connect an USB cable to the WeMos when the device is physically connected to the main power of the control panel, a common ground connection is extablished with the host. In some cases that behaviour is not desiderable.

To avoid possible power issue, a power selection switch is placed between the DC DC converter and the WeMos in order to temporary power the board by the usb port.

By connecting Jabby to any **device** port of the control panel, it's possible to trigger the alarm in two different ways: _device_, _tamper_. If you plan to configure this set up then "be careful" when you power on/off or reset Jabby when it's configured as a triggerable device, because if the connected port is enabled on the control panel, the alarm may trigger if the system is armed. Another way to trigger the alarm that don't need for connecting to any device port is by simulating a _keypad tampering_.

## Compiling the sketch
To compile the software some libraries are required:
- [ESP8266httpUpdate](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266httpUpdate)
- [ESP8266WiFi](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)
- [PubSubClient](https://pubsubclient.knolleary.net/)
- [ArduinoJson](https://arduinojson.org/)
- [LittleFS](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
- [SoftwareSerial](https://github.com/plerup/espsoftwareserial/)

### Customizing
Before flashing the program inside the ESP, copy and rename the files:
```
Jabby_sketch/examples/data.json.example -> Jabby_sketch/data/data.json
Jabby_sketch/examples/secrets.h.example -> Jabby_sketch/secrets.h
```
and customize the content of each file to fit your environment.

### Flashing
To flash the software on the ESP WeMos you can use **Arduino IDE**.

The content of the **Jabby_sketch/data** is intented to be copied inside the ESP flash memory. You can follow the instructions provided by the [LittleFS documentation](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html) to complete the task.

Finally flash the program and setup a suitable Wi-Fi connection. Now you are ready to play.

## Arming & disarming
At the moment Jabby sends the following arming commands to the bus to arm the system:
- `*1 + [code]` to arm in ABC configuration
- `*2 + [code]` to arm in A configuration
- `*3 + [code]` to arm in B configuration

the code is sent only if requested by the system according to the variable **SET_WO_ACCESS_CODE**. In that case the code **ACCESS_CODE** has to be suitable for the selected mode.
**A** and **B** configurations are avaiable only if the system is configured in _splitted mode_.

To disarm the system Jabby uses the code provided in the variable **ACCESS_CODE**. In order to disarm the system the code have to be a valid code for the specific arming level. _Master Code_ and _Service Code_ are not necessary and you should avoid using them in your configuration for security reasons. 

## Security matters
At the moment Jabby is very far to be "secure". As this project grows up, that will be the next pinpoint to accomplish.
