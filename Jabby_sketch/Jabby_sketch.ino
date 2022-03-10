#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
#include <ESPAsyncTCP.h>
#include <vector>
#include "secrets.h"
#include "config.h"
#include <unistd.h>
#include <SoftwareSerial.h>


//Configuration
char strMQTTServer[50] =  "\0"; //MQTT_SERVER;
int intMQTTPort = MQTT_PORT;
uint8_t intGSMEnable = GSM_ENABLE;
uint8_t intSetWOAccessCode = SET_WO_ACCESS_CODE;
char strMQTTUser[50] = "\0"; //MQTT_USER;
char strMQTTPassword[50] =  "\0"; //MQTT_PASSWORD;
//char strOTAPassword[50]=  "\0"; //OTA_PASSWORD;
char strOTAWebServer[50] =  "\0"; //OTA_WEB_SERVER;
int intOTAWebPort = OTA_WEB_PORT;
//char strOTAWebUser[50] =  "\0"; //OTA_WEB_USER;
//char strOTAWebPassword[50] =  NULL; //OTA_WEB_PASSWORD;
char strOTAWebPage[255] =  "\0"; //OTA_WEB_PAGE;
char strWiFiPassword[100] =  "\0"; //WIFI_PASSWORD;
char strWiFiSSID[50] =  "\0"; //WIFI_SSID;
//char strTCPUser[50] =  NULL; //TCP_USER;
char strTCPPassword[50] =  "\0"; //TCP_PASSWORD;
int intTCPPort = TCP_PORT;
char strAccessCode[5] = "\0"; //ACCESS_CODE

//TCP
struct TCPMessage {
  int terminal = -1; //slot for incoming messages and terminal for outgoing messages;
  char data[MAX_TCP_MESSAGE_LEN];
  int length = -1;
  //bool active = true;
};

struct TCPSlot {

  byte terminal = 0;
  bool authenticated = false;
  byte authAttempts = MAX_TCP_AUTH_ATTEMPTS;
  bool active = false;
  bool busy = false;
  bool hasData = false;
  char command[5] = "";
  char parameter[20] = "";
  char value[MAX_TCP_MESSAGE_LEN - 2] = ""; //take in to consideration the command (2 chars at least)
};

//Console
TCPSlot slots[MAX_TCP_CONNECTIONS];
//static std::vector<AsyncClient*> clients[MAX_TCP_CONNECTIONS]; // a list to hold all clients
static AsyncClient* clients[MAX_TCP_CONNECTIONS];
std::vector<TCPMessage*> inMessages;    // a list of incoming messages from clients
std::vector<TCPMessage*> outMessages;   // a list of outgoing messages from clients
int intBroadcast = 0;                   //holds the broadcast for all the slots
int intSub = 0;                         //holds the terminals that want to receive broadcast messages from server;
int intSubRS485 = 0;                    //holds the terminals that want to receive RS485 packets from server;
int intSubGSM = 0;                      //holds the terminals that want to receive GSM packets from server;
uint8_t intTerminals = 0;               //number of connected clients

//Wi-Fi
WiFiClient espClient;

//MQTT
PubSubClient mqttClient(espClient);
long lngLastReconnectAttempt = 0;
long lngLastPublishAttempt = 0;
long lngLastLoopCall = 0;

String strMQTTCommandTopic;
String strMQTTCommandTopicTR1;

//Serial
uint8_t serInput[MAX_SERIAL_PK_LEN];
uint8_t serInputLen = 0;
long lngLastSerialUpdateTime = 0;
int intSerialInput = 0;
uint8_t busStatus = 0;
long lngLastCommandSent = 0;


struct serMessage {
  uint8_t data[MAX_SERIAL_MSG_LEN];
  serMessageType type = serMessageCommand;
  int length = 0;
  int pointer = 0;
};
std::vector<serMessage*> serMessages;   // a list of outgoing RS485 messages

//Led
bool ledBlink = HIGH;
long lngLedBlinkTimer = 0;

//Time
long m = 0;

//GSM
SoftwareSerial ssGSM;
int intGSMInput = 0;
gsmState gsmStatus = gsmStateUnk;
long lngGSMTimeout = 0;
long lngGSMResetTime = 0;
uint8_t gsmRetry = GSM_RETRY;
uint8_t gsmMessage[MAX_SERIAL_PK_LEN];
uint8_t gsmPacketLen = 0;
long lngLastGSMUpdateTime = 0;

//Control Panel
uint8_t cpStatus = 0xFF; //last known CP status
uint8_t cpUpToDate = 0;  // the state of the CP is up-to-date. 0 = not updated; 1 = updated; 2 = force mqtt refresh
uint8_t cpInfo =   0xFF; //last known CP info
bool isCPInfo = false;
uint8_t cpDevice =   0xFF; //last device triggered (51 = Control Panel 0x33; 52 = Key Pad 0x34)
bool isCPDevice = false;
uint8_t cpMessage =   0xFF; //last displayed
bool isCPMessage = false;

long lngLastCPUpdateTime = 0;   //last update time

bool setStringVariable(int terminal, char* variable, const char* value, size_t size, charRestriction r) {
  size_t l = strlen(value) + 1;

  if (l > size) {
    if (terminal >= 0) {
      createMessage (terminal, ERR_VALUE_TOO_LARGE, strlen(ERR_VALUE_TOO_LARGE));
    }
    return false;
  } else {
    if (r >= charAllowDigits) {
      const char *p = value;
      for (size_t i = 0; i < l - 1; i++) {
        if ( (!isdigit(*p) && r == charAllowDigits) || (r == charAllowKeypad && (!isdigit(*p) && *p != '*' && *p != '#' ))) {
          if (terminal >= 0) {
            createMessage (terminal, ERR_WRONG_VALUE, strlen(ERR_WRONG_VALUE));
          }
          return false;
        }
        p++;
      }
    }
  }
  strcpy(variable, value);
  return true;
}

bool createMessage (int terminal, const char* data, size_t len ) {

  const char* ptrData = data;
  size_t sizeData = len;
  while (sizeData) {
    outMessages.push_back (new TCPMessage);
    outMessages.back()->terminal = terminal;
    if (sizeData > MAX_TCP_MESSAGE_LEN) { //send messages in chunk
      memcpy(outMessages.back()->data, ptrData, MAX_TCP_MESSAGE_LEN);
      outMessages.back()->length = MAX_TCP_MESSAGE_LEN;
      ptrData += MAX_TCP_MESSAGE_LEN;
      sizeData -= MAX_TCP_MESSAGE_LEN;
    } else {
      memcpy(outMessages.back()->data, ptrData, sizeData);
      outMessages.back()->length = sizeData;
      sizeData = 0;
    }
  }
  return true;
}

bool addDataToMessage(AsyncClient* client, const char* data, size_t len ) {
  if (client->space() > len) {
    client->add(data, len);
    return true;
  }
  return false;
}

bool sendMessage(AsyncClient* client) {
  if (client->canSend()) {
    client->send();
    return true;
  }
  return false;
}

/*
  ----------------------------------------------
  MQTT FUCTIONS
  ----------------------------------------------
*/
void mqttRefreshInfo() {

  String t;
  String s;

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_INFO_WARNING;
  //s = String((cpInfo & SERIAL_PK_WARNING) == SERIAL_PK_WARNING);
  s = cpWarningToString();
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_INFO_BATTERY;
  //s = String((cpInfo & SERIAL_PK_BATTERY) == SERIAL_PK_BATTERY);
  s = cpBatteryToString();
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_INFO_A;
  //s = String((cpInfo & SERIAL_PK_A) == SERIAL_PK_A);
  s = cpAToString();
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_INFO_B;
  //s = String((cpInfo & SERIAL_PK_B) == SERIAL_PK_B);
  s = cpBToString();
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_INFO_C;
  //s = String((cpInfo & SERIAL_PK_C) == SERIAL_PK_C);
  s = cpCToString();
  mqttClient.publish(t.c_str(), s.c_str());
}

void mqttRefreshDevice() {

  String t;
  String s;

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_DEVICE;
  s = String(cpDevice);
  mqttClient.publish(t.c_str(), s.c_str());
}

void mqttRefreshMessage() {

  String t;
  String s;

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_MESSAGE;
  s = cpMessageToString();
  mqttClient.publish(t.c_str(), s.c_str());
}

void mqttRefreshAlarm() {

  String t;
  String s;

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_ONLINE;
  s = String(cpUpToDate != 0);
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_ALARM + MQTT_STATE;
  s = mqttStateToString();
  mqttClient.publish(t.c_str(), s.c_str());

  //sensors
  t = MQTT_PREFIX + WiFi.hostname() + MQTT_MODE;
  s = String(cpModeToString());
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_ARMED;
  s = String(cpArmedToString());
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_TRIGGERED;
  //s = String((cpStatus & SERIAL_PK_TRIGGERED)== SERIAL_PK_TRIGGERED);
  s = cpTriggredToString();
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_ACTIVATED;
  //s = String((cpStatus & SERIAL_PK_TRIGGERED) == SERIAL_PK_TRIGGERED);
  s = cpActivatedToString();
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_DELAYED;
  //s = String((cpStatus & SERIAL_PK_DELAYED) == SERIAL_PK_DELAYED);
  s = cpDelayedToString();
  mqttClient.publish(t.c_str(), s.c_str());

}

boolean mqttReconnect() {

  if (mqttClient.connect(WiFi.hostname().c_str(), strMQTTUser, strMQTTPassword)) {

    mqttClient.subscribe(strMQTTCommandTopic.c_str());
    mqttClient.subscribe(strMQTTCommandTopicTR1.c_str());
  }
  if (intSub) {
    String s = F("MQTT: Connect (\'") + WiFi.hostname() + F("\', \'") + String(strMQTTUser) + F("\', \'") +  String(strMQTTPassword) + F("\') result was ") + String(mqttClient.connected()) + STR_N;
    createMessage(intSub, s.c_str(), s.length());
  }
  return mqttClient.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  if (intSub) {
    String s = F("MQTT Message [") + String(topic) + F("]\n");
    createMessage(intSub, s.c_str(), s.length());
    createMessage(intSub, (char*) payload, len);
  }

  if (!strcmp(strMQTTCommandTopicTR1.c_str(), topic)) {
    char p[len + 1];
    memset(p, 0, sizeof(p));
    memcpy(p, (char*)payload, len);

    if (!strcmp(MQTT_PAYLOAD_ANNOUNCE, p)) {
      mqttRefreshAlarm();
      mqttRefreshInfo();
    }
  } else {
    if (cpUpToDate) {
      if (!strcmp(strMQTTCommandTopic.c_str(), topic)) {
        char p[len + 1];
        memset(p, 0, sizeof(p));
        memcpy(p, (char*)payload, len);

        if (!strcmp(MQTT_PAYLOAD_ARM_AWAY, p)) {
          setAlarm(-1, armABC);
        } else if (!strcmp(MQTT_PAYLOAD_ARM_HOME, p)) {
          setAlarm(-1, armA);
        } else if (!strcmp(MQTT_PAYLOAD_ARM_NIGHT, p)) {
          setAlarm(-1, armB);
        } else if (!strcmp(MQTT_PAYLOAD_DISARM, p)) {
          //disarm
          setAlarm(-1, armNone);
        } else if (!strcmp(MQTT_PAYLOAD_TRIGGER, p) ) {
          //trigger device
          triggerAlarm(1);
          delay (TRIGGER_INTERVAL_TIMER);
          triggerAlarm(0);
        } else if (!strcmp(MQTT_PAYLOAD_TRIGGER_2, p) ) {
          //trigger device
          triggerAlarm(2);
          delay (TRIGGER_INTERVAL_TIMER);
          triggerAlarm(0);
        } else if (!strcmp(MQTT_PAYLOAD_TRIGGER_3, p) ) {
          //trigger device
          if (!triggerAlarm(3)) {
            if (intSub) {
              createMessage (intSub, ERR_TRIGGER, strlen(ERR_TRIGGER));
            }
          }
        }

      } // !strcmp(strMQTTCommandTopic.c_str(), topic)

    } //cpUpToDate
  } //!strcmp(MQTT_PAYLOAD_ANNOUNCE,p)

}

/* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error) {

  if (intSub) {
    String s = F("Connection error ") + String(client->errorToString(error)) + STR_N + client->remoteIP().toString() + STR_N;
    createMessage(intSub, s.c_str(), s.length());
  }
}

static void handleDisconnect(void* arg, AsyncClient* client) {

  if (intSub) {
    String s = F("Client ") + String(client->remoteIP().toString()) + F(" disconnected\n");
    createMessage(intSub, s.c_str(), s.length());
  }

  for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    if (clients[i] == client) {
      slots[i].active = false;
      slots[i].busy = false;
      slots[i].authenticated = false;
      slots[i].hasData = false;
      strcpy(slots[i].command, "");
      strcpy(slots[i].parameter, "");
      strcpy(slots[i].value, "");
      intTerminals--;
      intSub = intSub & (intBroadcast - slots[i].terminal);
      break;
    }
  }

  delete client;
}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
  //Nothing to do here
}

static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {

  //allow max MAX_TCP_MESSAGES_LEN bytes
  if (len > MAX_TCP_MESSAGE_LEN) {
    if (addDataToMessage (client, ERR_MESSAGE_TOO_BIG, strlen(ERR_MESSAGE_TOO_BIG))) {
      sendMessage(client);
    }
    return;
  }

  //check message space availability
  if (inMessages.size() < MAX_TCP_MESSAGES) {
    int intSlot = -1;

    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
      if (clients[i] == client) {
        intSlot = i;
        break;
      }
    }

    // get incoming message
    if (intSlot >= 0) {

      inMessages.push_back (new TCPMessage);
      inMessages.back()->terminal = intSlot; //slots[intSlot].terminal;
      inMessages.back()->length = len;
      memcpy(inMessages.back()->data, (char*)data, len);
    }
  } else {

    if (addDataToMessage(client, ERR_TOO_MANY_MESSAGES, strlen(ERR_TOO_MANY_MESSAGES))) {
      sendMessage (client);
    }
    return;
  }
}

/* server events */
static void handleNewClient(void* arg, AsyncClient* client) {

  if (intSub) {
    String s = F("New client has been connected (") + client->remoteIP().toString() + F(")\n");
    createMessage(intSub, s.c_str(), s.length());
  }

  // register events
  client->onError(&handleError, NULL);
  client->onDisconnect(&handleDisconnect, NULL);
  client->onTimeout(&handleTimeOut, NULL);

  //check for a free slot
  int intSlot = -1;
  for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    if (!slots[i].active) {
      intSlot = i;
      break;
    }
  }

  if (intSlot >= 0) {
    intTerminals++;
    clients[intSlot] = client;

    //register events
    client->onData(&handleData, NULL);

    slots[intSlot].active = true;
    slots[intSlot].authenticated = false;

    //create reply
    String s = F("Connected to terminal ") + String(intSlot) + STR_N;
    createMessage (slots[intSlot].terminal, s.c_str(), s.length());

  } else {

    if (addDataToMessage(client, ERR_NO_ROOM_FOR_CONNECTION, strlen(ERR_NO_ROOM_FOR_CONNECTION))) {
      sendMessage (client);
      client->close();
      //delete client;
    }
  }
}

String mqttStateToString () {
  if (cpStatus & SERIAL_PK_TRIGGERED) {
    return STR_MQTT_TRIGGERED;
  }
  if (cpStatus & SERIAL_PK_ACTIVATED) {
    return STR_MQTT_DISARMING;
  }
  if (cpStatus & SERIAL_PK_DELAYED) {
    return STR_MQTT_ARMING;
  }
  if ((cpStatus & SERIAL_PK_ARMED_MASK) == SERIAL_PK_ARMED_ABC) {
    return STR_MQTT_ARMED_AWAY;
  }
  if ((cpStatus & SERIAL_PK_ARMED_MASK) == SERIAL_PK_ARMED_A) {
    return STR_MQTT_ARMED_HOME;
  }
  if ((cpStatus & SERIAL_PK_ARMED_MASK) == SERIAL_PK_ARMED_B) {
    return STR_MQTT_ARMED_NIGHT;
  }
  return STR_MQTT_DISARMED;

};

String gsmStatusToString () {

  switch (gsmStatus) {
    case gsmStateAck: {
        return STR_ACKNOWLEDGMENT;
      }
    case gsmStateRst: {
        return STR_RESET;
      }
    case gsmStateOk: {
        return STR_OK;
      }
    default: {
        return STR_UNKNOWN;
      }
  }
}

String cpMessageToString() {
  switch (cpMessage) {
    case 0:
      return STR_0;
    case 2:
      return SER_MSG_02;
    case 5:
      return SER_MSG_05;
    case 0x11:
      return SER_MSG_11;
    case 0x14:
      return SER_MSG_14;
    case 0x33:
      return SER_MSG_33;
    case 0x34:
      return SER_MSG_34;
    case 0x36:
      return SER_MSG_36;
    case 0x38:
      return SER_MSG_38;
    case 0x39:
      return SER_MSG_39;
    case 0x3A:
      return SER_MSG_3A;
    case 0x3F:
      return SER_MSG_3F;
  }
  return String(cpMessage);
}

String cpModeToString() {
  switch (cpStatus & SERIAL_PK_MODE_MASK) {

    case SERIAL_PK_MODE_NORMAL_SPLITTED:
      return STR_MODE_NORMAL_SPLITTED;
      break;

    case SERIAL_PK_MODE_NORMAL_UNSPLITTED:
      return STR_MODE_NORMAL_UNSPLITTED;
      break;

    case SERIAL_PK_MODE_MASTER:
      return STR_MODE_MASTER;
      break;

    default:
      return STR_MODE_SERVICE;
  }
}

String cpArmedToString() {
  switch (cpStatus & SERIAL_PK_ARMED_MASK) {

    case SERIAL_PK_ARMED_ABC:
      return STR_ARMED_ABC;
      break;

    case SERIAL_PK_ARMED_A:
      return STR_ARMED_A;
      break;

    case SERIAL_PK_ARMED_B:
      return STR_ARMED_B;
      break;

    default:
      return STR_ARMED_NONE;

  }
}

String cpUpToDateToString() {
  return (cpUpToDate) ? STR_ON : STR_OFF;
}

String cpTriggredToString() {
  return (cpStatus & SERIAL_PK_TRIGGERED ) ? STR_ON : STR_OFF;
}

String cpActivatedToString() {
  return (cpStatus & SERIAL_PK_ACTIVATED) ? STR_ON : STR_OFF;
}

String cpDelayedToString() {
  return (cpStatus & SERIAL_PK_DELAYED) ? STR_ON : STR_OFF;
}

String cpWarningToString() {
  return (cpInfo & SERIAL_PK_WARNING ) ? STR_ON : STR_OFF;
}

String cpBatteryToString() {
  return (cpInfo & SERIAL_PK_BATTERY ) ? STR_ON : STR_OFF;
}

String cpAToString() {
  return (cpInfo & SERIAL_PK_A ) ? STR_ON : STR_OFF;
}

String cpBToString() {
  return (cpInfo & SERIAL_PK_B ) ? STR_ON : STR_OFF;
}

String cpCToString() {
  return (cpInfo & SERIAL_PK_C ) ? STR_ON : STR_OFF;
}

void simulateKPTrigger() {
  //uint8_t* p=pgm_read_byte_near(SER_MSG_TAMPER_KEYPAD);
  uint8_t s[2]={0xB2,0xFF};
  //uint8_t s[2];
  //memcpy_P(s,SER_MSG_TAMPER_KEYPAD,2);
  //createStream((uint8_t *)pgm_read_byte(SER_MSG_TAMPER_KEYPAD), 2);
  createStream(s, 2);
}

bool createStream(const uint8_t* data, size_t len) {
  const uint8_t* ptrData = data;
  size_t sizeData = len;

  while (sizeData) {
    serMessages.push_back (new serMessage);
    serMessages.back()->type = serMessageStream;
    
    if (sizeData > MAX_SERIAL_MSG_LEN) { //send messages in chunk
      memcpy(serMessages.back()->data, ptrData, MAX_SERIAL_MSG_LEN);
  
      serMessages.back()->length = MAX_SERIAL_MSG_LEN;
      ptrData += MAX_SERIAL_MSG_LEN;
      sizeData -= MAX_SERIAL_MSG_LEN;
    } else {
      memcpy(serMessages.back()->data, ptrData, sizeData);
      serMessages.back()->length = sizeData;
      sizeData = 0;
    }
  }
  return true;
}

/*
  ----------------------------------------------
  CRC
  ----------------------------------------------
*/
int calculateCRC (const uint8_t* message, size_t len ) {
  if (len > 2) {
    int magicNumber = 0x7F;
    const uint8_t *p = message;
    for (int j = 0; j < len; j++) {
      int c = *(p + j);
      for (int k = 8; k > 0; k--) {
        magicNumber += magicNumber;
        if (c & SERIAL_PK_BEGIN_MASK) {
          magicNumber += 1;
        }
        if (magicNumber & SERIAL_PK_BEGIN_MASK) {
          magicNumber ^= CRC_POLY;
        }
        c += c;
      } // for k
    } // for i
    return magicNumber;
  }
  return 0;
}

void setCRC(uint8_t* message, size_t len) {
  if (len > 2) {
    int magicNumber = calculateCRC(message, len - 2);
    uint8_t *p = message;
    int c = CRC_POLY;
    for (int k = 8; k > 0; k--) {
      magicNumber += magicNumber;
      if (c & SERIAL_PK_BEGIN_MASK) {
        magicNumber += 1;
      }
      if (magicNumber & SERIAL_PK_BEGIN_MASK) {
        magicNumber ^= CRC_POLY;
      }
      c += c;
    } // for k
    *(p + len - 1) = magicNumber;
  }
}

void emptySerialOutput(){
  if (serMessages.size() > 0) {
    for (int i = serMessages.size() - 1; i >= 0; i--) {
      delete serMessages[i];
      serMessages.erase(serMessages.begin() + i);
    }
  }

  serMessages.clear();
  serMessages.reserve (MAX_SER_MESSAGES);
}

/*
  ----------------------------------------------
  INSTRUCTIONS
  ----------------------------------------------
*/
bool triggerAlarm (uint8_t t) {

  switch (t) {
    case 1:
      analogWrite(TRIGGER_PIN, TRIGGER_DEVICE);
      return true;
    case 2:
      analogWrite(TRIGGER_PIN, TRIGGER_TAMPER);
      return true;
    case 3:
      simulateKPTrigger();
      return true;
    default:
      analogWrite(TRIGGER_PIN, TRIGGER_NONE);
      return true;
  }
  return false;
}

void resetGSM() {

  lngGSMResetTime = m;
  //pinMode(GSM_RST_PIN, OUTPUT);
  digitalWrite(GSM_RST_PIN, HIGH);
  gsmStatus = gsmStateRst;
}

bool loadConfig() {
  File configFile = LittleFS.open(DATA_FILENAME, "r");
  if (!configFile) {
    if (intSub) {
      String s = F("Failed to open config file.\n");
      createMessage(intSub, s.c_str(), s.length());
    }
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    if (intSub) {
      String s = F("Config file size is too large.\n");
      createMessage(intSub, s.c_str(), s.length());
    }
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  configFile.close();

  StaticJsonDocument<512> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    if (intSub) {
      String s = F("Failed to parse config file.\n");
      createMessage(intSub, s.c_str(), s.length());
    }
    return false;
  }
  strncpy(strMQTTServer, !doc[LBL_MQTT_SERVER].isNull() ? doc[LBL_MQTT_SERVER] : MQTT_SERVER, sizeof(strMQTTServer));
  intMQTTPort = !doc[LBL_MQTT_PORT].isNull() ? doc[LBL_MQTT_PORT] : MQTT_PORT;
  intGSMEnable = !doc[LBL_GSM_ENABLE].isNull() ? doc[LBL_GSM_ENABLE] : GSM_ENABLE;
  intSetWOAccessCode = !doc[LBL_SET_WO_ACCESS_CODE].isNull() ? doc[LBL_SET_WO_ACCESS_CODE] : SET_WO_ACCESS_CODE;
  strncpy(strMQTTUser, !doc[LBL_MQTT_USER].isNull() ? doc[LBL_MQTT_USER] : MQTT_USER , sizeof(strMQTTUser));
  strncpy(strMQTTPassword, !doc[LBL_MQTT_PASSWORD].isNull() ? doc[LBL_MQTT_PASSWORD] : MQTT_PASSWORD, sizeof(strMQTTPassword));
  //  if (!doc[LBL_OTA_PASSWORD].isNull()) {strncpy(strOTAPassword, doc[LBL_OTA_PASSWORD], sizeof(strOTAPassword));}
  strncpy(strOTAWebServer, !doc[LBL_OTA_WEB_SERVER].isNull() ? doc[LBL_OTA_WEB_SERVER] : OTA_WEB_SERVER, sizeof(strOTAWebServer));
  intOTAWebPort = !doc[LBL_OTA_WEB_PORT].isNull() ? doc[LBL_OTA_WEB_PORT] : OTA_WEB_PORT;
  //  if (!doc[LBL_OTA_WEB_PORT].isNull()) {intOTAWebPort = doc[LBL_OTA_WEB_PORT];} else {intOTAWebPort = OTA_WEB_PORT};
  //  if (!doc[LBL_OTA_WEB_USER].isNull()) {strncpy(strOTAWebUser, doc[LBL_OTA_WEB_USER], sizeof(strOTAWebUser));}
  //  if (!doc[LBL_OTA_WEB_PASSWORD].isNull()) {strncpy(strOTAWebPassword, doc[LBL_OTA_WEB_PASSWORD], sizeof(strOTAWebPassword));}
  strncpy(strOTAWebPage, !doc[LBL_OTA_WEB_PAGE].isNull() ? doc[LBL_OTA_WEB_PAGE] : OTA_WEB_PAGE, sizeof(strOTAWebPage));
  strncpy(strWiFiPassword, !doc[LBL_WIFI_PASSWORD].isNull() ? doc[LBL_WIFI_PASSWORD] : WIFI_PASSWORD, sizeof(strWiFiPassword));
  strncpy(strWiFiSSID, !doc[LBL_WIFI_SSID].isNull() ? doc[LBL_WIFI_SSID] : WIFI_SSID, sizeof(strWiFiSSID));
  //  if (!doc[LBL_TCP_USER].isNull()) {strncpy(strTCPUser, doc[LBL_TCP_USER], sizeof(strTCPUser));}
  strncpy(strTCPPassword, !doc[LBL_TCP_PASSWORD].isNull() ? doc[LBL_TCP_PASSWORD] : TCP_PASSWORD, sizeof(strTCPPassword));
  intTCPPort = !doc[LBL_TCP_PORT].isNull() ? doc[LBL_TCP_PORT] : TCP_PORT;
  strncpy(strAccessCode, !doc[LBL_ACCESS_CODE].isNull() ? doc[LBL_ACCESS_CODE] : ACCESS_CODE, sizeof(strAccessCode));
  return true;
}

bool saveConfig() {

  StaticJsonDocument<512> doc;
  doc[LBL_MQTT_SERVER] = strMQTTServer;
  doc[LBL_MQTT_PORT] = intMQTTPort;
  doc[LBL_GSM_ENABLE] = intGSMEnable;
  doc[LBL_SET_WO_ACCESS_CODE] = intSetWOAccessCode;
  doc[LBL_MQTT_USER] = strMQTTUser;
  doc[LBL_MQTT_PASSWORD] = strMQTTPassword;
  //  doc[LBL_OTA_PASSWORD] = strOTAPassword;
  doc[LBL_OTA_WEB_SERVER] = strOTAWebServer;
  doc[LBL_OTA_WEB_PORT] = intOTAWebPort;
  //  doc[LBL_OTA_WEB_USER] = strOTAWebUser;
  //  doc[LBL_OTA_WEB_PASSWORD] = strOTAWebPassword;
  doc[LBL_OTA_WEB_PAGE] = strOTAWebPage;
  doc[LBL_WIFI_PASSWORD] = strWiFiPassword;
  doc[LBL_WIFI_SSID] = strWiFiSSID;
  //  doc[LBL_TCP_USER] = strTCPUser;
  doc[LBL_TCP_PASSWORD] = strTCPPassword;
  doc[LBL_TCP_PORT] = intTCPPort;
  doc[LBL_ACCESS_CODE] = strAccessCode;

  File configFile = LittleFS.open(DATA_FILENAME, "w");
  if (!configFile) {
    if (intSub) {
      String s = F("Failed to open config file for writing.\n");
      createMessage(intSub, s.c_str(), s.length());
    }
    return false;
  }
  serializeJson(doc, configFile);
  configFile.close();
  return true;
}

bool loadHelp(int slot) {
  File helpFile = LittleFS.open(HELP_FILENAME, "r");
  if (!helpFile) {
    if (intSub) {
      String s = F("Failed to open help file.\n");
      createMessage(intSub, s.c_str(), s.length());
    }
    return false;
  }

  size_t size = helpFile.size();
  uint8_t ptrData[MAX_TCP_MESSAGE_LEN];

  while (size) {
    if (size > MAX_TCP_MESSAGE_LEN) {
      helpFile.read(ptrData, MAX_TCP_MESSAGE_LEN);
      createMessage (slots[slot].terminal, (char*) ptrData, MAX_TCP_MESSAGE_LEN);
      size -= MAX_TCP_MESSAGE_LEN;
    } else {
      helpFile.read(ptrData, size);
      createMessage (slots[slot].terminal, (char*) ptrData, size);
      size = 0;
    }
  }
  helpFile.close();
  return true;

}

int checkUpdate(int slot, bool reboot) {

  t_httpUpdate_return result = ESPhttpUpdate.update(espClient, strOTAWebServer, intOTAWebPort, strOTAWebPage, VERSION);
  String s = F("Current version: [") + String(VERSION) + F("]\nCheck for updates on ") + String(strOTAWebServer) + F(":") + String(intOTAWebPort) + String(strOTAWebPage) + STR_N;

  if (slot >= 0 and slot <= MAX_TCP_CONNECTIONS) {

    if (slots[slot].active) {

      switch (result) {
        case HTTP_UPDATE_FAILED:

          s += F("Update failed. ") + String(ERR_HTTP_UPDATE_FAILED) + F(" (") + ESPhttpUpdate.getLastError() + F(") - ") +  ESPhttpUpdate.getLastErrorString();
          break;
        case HTTP_UPDATE_NO_UPDATES:
          s += F("Update no Update. ") + String(WRN_HTTP_UPDATE_NO_UPDATES);
          break;
        case HTTP_UPDATE_OK:
          s += F("Update ok. ") + String(INFO_HTTP_UPDATE_OK);
          break;
      }
    }
  }

  s += STR_N;
  createMessage(intSub + slot, s.c_str(), s.length());
  return result;
}

bool sendATCommand (char* command) {
  size_t r = 0;
  if (ssGSM) {
    r = ssGSM.write(command, strlen(command));
    ssGSM.write('\r');
  }

  return (bool)r;
}

bool createSequence(int terminal, const char* command) {
  /*
    if (serMessages.size() >= MAX_SER_MESSAGES) {
      if (terminal >= 0) {
        createMessage (terminal, ERR_DEVICE_BUSY, strlen(ERR_DEVICE_BUSY));
      }
      return false;
    }
  */
  serMessages.push_back (new serMessage);
  serMessages.back()->type = serMessageCommand;
  if (!setStringVariable(terminal, (char *)serMessages.back()->data, command, MAX_SERIAL_SEQ_LEN, charAllowKeypad)) {
    return false;
  }

  uint8_t *p = serMessages.back()->data;
  while (*p) {
    //convert the buffer for rs485
    if (isdigit(*p)) {
      *p = (*p | SERIAL_PK_BEGIN_MASK) & 0xCF;
    } else {
      *p = (*p == '*') ? 0x8F : 0x8E;
    }
    p++;
    serMessages.back()->length++;
  }
  return true;
}

bool sendRawCommand(char* command) {

  char *ptrCommand = strtok(command, " ");
  char *p;
  digitalWrite(REDE_PIN, HIGH);
  while (ptrCommand != NULL) {

    long result = strtol(ptrCommand, &p, 10);

    if (result >= 0 && result <= 255) {
      Serial.write((uint8_t)result);
      delay(SERIAL_INTERVAL_TIMER);
    } else {
      //delay (5);
      digitalWrite(REDE_PIN, LOW);
      return false;
    }
    ptrCommand = strtok (NULL, " ");
  }
  delay (SERIAL_INTERVAL_TIMER);
  digitalWrite(REDE_PIN, LOW);
  return true;
}

bool setAlarm(int terminal, armCommand command) {
  String s;
  switch (command) {
    case armA: case armB: case armABC:
      if ( !(cpStatus & SERIAL_PK_MODE_MASK) ) {
        return false;
      }
      s = STR_COMMAND_PREFIX + String(command);
      if (!intSetWOAccessCode) {
        s += strAccessCode;
      }
      break;
    case armNone:
      if (!(cpStatus & SERIAL_PK_ARMED_MASK )) {
        return false;
      }
      s = strAccessCode;
      break;
    default:
      return false;
  }
  return createSequence(terminal, s.c_str());
}

void setup() {

  pinMode(LED_PIN, OUTPUT);
  pinMode(REDE_PIN, OUTPUT);
  pinMode(GSM_RST_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);

  digitalWrite(LED_PIN, ledBlink);
  digitalWrite(REDE_PIN, LOW);
  digitalWrite(GSM_RST_PIN, LOW);
  digitalWrite(LED_PIN, ledBlink);
  analogWrite(TRIGGER_PIN, TRIGGER_NONE);


  inMessages.reserve (MAX_TCP_MESSAGES);
  //serMessages.reserve (MAX_SER_MESSAGES);

  //load default values
  strcpy(strMQTTServer, MQTT_SERVER);
  strcpy(strMQTTUser, MQTT_USER);
  strcpy(strMQTTPassword, MQTT_PASSWORD);
  //char strOTAPassword[50]=  "\0"; //OTA_PASSWORD;
  strcpy(strOTAWebServer, OTA_WEB_SERVER);
  //char strOTAWebUser[50] =  "\0"; //OTA_WEB_USER;
  //char strOTAWebPassword[50] =  NULL; //OTA_WEB_PASSWORD;
  strcpy(strOTAWebPage, OTA_WEB_PAGE);
  strcpy(strWiFiPassword, WIFI_PASSWORD);
  strcpy(strWiFiSSID, WIFI_SSID);
  //char strTCPUser[50] =  NULL; //TCP_USER;
  strcpy(strTCPPassword, TCP_PASSWORD);
  strcpy(strAccessCode, ACCESS_CODE);

#ifdef LOCAL_SERIAL
  Serial.begin(9600);
  while (!Serial) {
    delay (SERIAL_INTERVAL_TIMER);
  }
  Serial.println();
  Serial.printf("Booting ver. %s (%s)\n", VERSION, WIFI_SSID);
#endif

  //assign a terminal id to each slot
  byte b = 1;
  for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    intBroadcast += b;
    slots[i].terminal = b;
    b = b << 1;
  }

#ifdef LOCAL_SERIAL
  Serial.println("Get Chip ID");
  Serial.println(ESP.getChipId());
  Serial.println("Mounting FS...");
#endif

  if (!LittleFS.begin()) {

#ifdef LOCAL_SERIAL
    Serial.println("Failed to mount file system");
#endif
  }

  if (!loadConfig()) {
#ifdef LOCAL_SERIAL
    Serial.println("Failed to load config");
  } else {
    Serial.println("Config loaded");
#endif
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(strWiFiSSID, strWiFiPassword);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
#ifdef LOCAL_SERIAL
    Serial.println("Connection Failed! Try default settings...");
#endif
    delay(5000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
#ifdef LOCAL_SERIAL
      Serial.println("Connection Failed! Reboot...");
#endif
      ESP.restart();
    }
  }

  String s;
  s = String(NAME) + String(WiFi.macAddress());
  s.replace(":", "");
  WiFi.hostname(s);
#ifdef LOCAL_SERIAL
  Serial.println(WiFi.localIP().toString());
#endif


  //check for updates
#ifdef LOCAL_SERIAL
  Serial.printf("Check update: %d\n", checkUpdate(-1, true));
#endif
  checkUpdate(-1, true);

  //start console
  AsyncServer* server = new AsyncServer(TCP_PORT); // start listening on tcp port
  server->onClient(&handleNewClient, server);
  server->begin();

  //start mqtt
  mqttClient.setServer(strMQTTServer, intMQTTPort);
  mqttClient.setCallback(mqttCallback);

  //start serial console
  Serial.begin(9600);
  while (!Serial) {
    delay (SERIAL_INTERVAL_TIMER);
  }

  strMQTTCommandTopic = MQTT_PREFIX + WiFi.hostname() + MQTT_ALARM + MQTT_COMMAND;
  strMQTTCommandTopicTR1 = String(MQTT_PREFIX) + String(MQTT_COMMAND);
}

void loop() {

  m = millis();

  //look output from serial
  intSerialInput = Serial.available();

  /*
    ----------------------------------------------
    GSM MANAGEMENT
    ----------------------------------------------
  */
  if (intGSMEnable) {
    if (!gsmStatus) {
      if (intSub) {
        createMessage (intSub, INFO_OPEN_GSM_CONNECTION, strlen(INFO_OPEN_GSM_CONNECTION));
      }
      if (ssGSM) {
        ssGSM.end();
      }
      ssGSM.begin(57600, SWSERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN, false, 32); //,256);
      if (!ssGSM) {
        if (intSub) {
          createMessage (intSub, ERR_SERIAL_GSM, strlen(ERR_SERIAL_GSM));
        }
        gsmStatus = gsmStateErr; //no more try
      } else {
        //Start acknowledgment
        gsmStatus = gsmStateAck;
        lngGSMTimeout = m;
        gsmRetry = GSM_RETRY;
      }
    }
  } else if (ssGSM) {

    ssGSM.end();
    if (intSub) {
      createMessage (intSub, INFO_CLOSE_GSM_CONNECTION, strlen(INFO_CLOSE_GSM_CONNECTION));
    }
    gsmStatus = gsmStateUnk;
  }

  if ( gsmStatus == gsmStateAck && (m >= (lngGSMTimeout + SERIAL_MAX_INTERVAL_TIMER))) {
    lngGSMTimeout = m;

    if (!gsmRetry) {

      //reset device
      resetGSM();
    } else {

      //send AT
      ssGSM.write(GSM_CMD_AT, strlen(GSM_CMD_AT));
      gsmRetry--;
    }
  }

  // check reset GSM command status
  //if (digitalRead(GSM_RST_PIN)) {
  if ((gsmStatus == gsmStateRst) && (m > (lngGSMResetTime + GSM_INTERVAL_TIMER))) {
    digitalWrite(GSM_RST_PIN, LOW);
    //delay (15);
    //pinMode(GSM_RST_PIN, INPUT_PULLUP);
    gsmStatus = gsmStateUnk;
  }

  //get output from GSM
  if (ssGSM) {
    int intGSMInput = ssGSM.available();
    if (intGSMInput > 0) {
      uint8_t buf[intGSMInput];
      intGSMInput = ssGSM.readBytes(buf, intGSMInput);
      if (intGSMInput) { //data ok
        lngLastGSMUpdateTime = m;

        if (gsmPacketLen >= MAX_SERIAL_PK_LEN - 1) { //1 char is needed to store '\0'
          if (intSub) {
            createMessage (intSub, ERR_GSM_INCOMING_DATA, strlen(ERR_GSM_INCOMING_DATA));
          }
          gsmPacketLen = 0;
        }

        for (int i = 0; i <= intGSMInput - 1; i++) {
          gsmMessage[gsmPacketLen] = buf[i];
          gsmPacketLen++;

          //wait for the end of the packet
          if (buf[i] == '\n') {
            //add termination char
            //gsmPacketLen++;
            gsmMessage[gsmPacketLen] = '\0';

            //sniffing
            if (intSubGSM) {
              createMessage (intSubGSM, (char*)gsmMessage, gsmPacketLen);
            }

            //read the incoming packet
            switch (gsmStatus) {
              case gsmStateAck:
                if (!strcmp( (char*)gsmMessage, STR_GSM_OK)) {
                  if (intSub) {
                    createMessage (intSub, INFO_GSM_CONNECTION_OK, strlen(INFO_GSM_CONNECTION_OK));
                  }
                  gsmStatus = gsmStateOk; //baud rate sync ok
                }//is STR_GSM_OK?
            } //switch (gsmStatus)
            //empty the buffer

            gsmPacketLen = 0;
          } // EOF \n
        } //for i

      }
    }
  }

  /*
    ----------------------------------------------
    RS485 SEND SEQUENCE
    ----------------------------------------------
  */
  if (serMessages.size() > 0) {
    if (serMessages[0]->pointer < serMessages[0]->length) {

      if (!busStatus && !serInputLen) {
        lngLastCommandSent = m;
        digitalWrite(REDE_PIN, HIGH);
        delay(2);
        
        if (serMessages[0]->type){
          busStatus = BUS_WAITING_ACK;
          Serial.write(serMessages[0]->data[serMessages[0]->pointer]);
          //delay(20);
          Serial.write(SERIAL_PK_END);
          //delay(3); // this delay is needed to avoid feedback in incoming traffic (investigation needed) 
          serMessages[0]->pointer++;  
        } else{ 
          //busStatus = BUS_IGNORE_ACK;
          while(serMessages[0]->pointer < serMessages[0]->length){
            Serial.write(serMessages[0]->data[serMessages[0]->pointer]);
            serMessages[0]->pointer++;
            //delay(SERIAL_INTERVAL_TIMER);
          }//while
        }
        delay(3); // this delay is needed to avoid feedback in incoming traffic (investigation needed)   
        digitalWrite(REDE_PIN, LOW);
      } //!busStatus && !serPacketLen
    } else {
      //remove the message from the queue
      delete serMessages[0];    
      serMessages.erase(serMessages.begin());   
      if (!serMessages.size()) {
        //free memory
        emptySerialOutput();
      }
    } //(serMessages[0]->pointer < serMessages[0]->length)
  } //serMessages.size() > 0

  /*
    ----------------------------------------------
    TCP MANAGEMENT
    ----------------------------------------------
  */

  for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    slots[i].busy = false;
    strcpy(slots[i].command, "");
    strcpy(slots[i].parameter, "");
    strcpy(slots[i].value, "");
  }

  //check for new messages
  if (inMessages.size() > 0) {

    for (int i = 0; i < inMessages.size(); i++) {

      if (!slots[inMessages[i]->terminal].authenticated) {
        if (slots[inMessages[i]->terminal].authAttempts <= 0) { //can be lower than 0 if commands are sent quickly
          //close connection
          clients[inMessages[i]->terminal]->close();
        }

        //check password
        char password[MAX_TCP_MESSAGE_LEN + 1]; //store the termination of the string
        strncpy(password, inMessages[i]->data, inMessages[i]->length);
        password[inMessages[i]->length] = NULL;

        char *p  = strchr(password, '\r');
        if (p) {
          *p = '\0';
        }
        p  = strchr(password, '\n');
        if (p) {
          *p = '\0';
        }

        if (!strcmp(password, strTCPPassword)) {
          slots[inMessages[i]->terminal].authenticated = true;
          createMessage (slots[inMessages[i]->terminal].terminal, INFO_LOGIN_OK, strlen(INFO_LOGIN_OK));
        } else {
          createMessage (slots[inMessages[i]->terminal].terminal, ERR_AUTH, strlen(ERR_AUTH));
          slots[inMessages[i]->terminal].authAttempts--;
        }

      } else {
        //for each message get the commands '\n'
        char command[MAX_TCP_MESSAGE_LEN + 1]; //store the termination of the string
        strncpy(command, inMessages[i]->data, inMessages[i]->length);
        command[inMessages[i]->length] = NULL;

        char *ptrEndCommand;
        char *ptrCommand = strtok_r(command, "\r\n", &ptrEndCommand);

        //if '\r\n' is missing then calculate the whole message as a single command (putty)
        if (!ptrEndCommand) {
          ptrEndCommand = ptrCommand + inMessages[i]->length;
        }

        int intCount;          //point to the first param
        bool blCheckCommand;   //check if the command is ok
        bool blCheck2ndPrn;    //check if a second parameter is needed
        bool blCheckValue;     //check if a value is needed

        while (ptrCommand != NULL) {
          intCount = 0;             //point to the first param
          blCheckCommand = true;   //check if the command is ok
          blCheck2ndPrn = true;    //check if a second parameter is needed
          blCheckValue = true;     //check if a value is needed

          char *ptrEndParameter;
          char *ptrParameter = strtok_r(ptrCommand, " ", &ptrEndParameter);

          while (ptrParameter != NULL) {

            //for each command get the params ' '
            if (!intCount) { //intCount == 0

              //get the instruction
              if (!strcmp(ptrParameter, "e") || !strcmp(ptrParameter, "v")) {
                blCheck2ndPrn = false;
                blCheckValue = false;
                strcpy(slots[inMessages[i]->terminal].command, ptrParameter);

              } else if (!strcmp(ptrParameter, "t")) {
                blCheckValue = false;
                strcpy(slots[inMessages[i]->terminal].command, ptrParameter);
              } else if (!strcmp(ptrParameter, "g") || !strcmp(ptrParameter, "h") || !strcmp(ptrParameter, "i") || !strcmp(ptrParameter, "k") || !strcmp(ptrParameter, "l") || !strcmp(ptrParameter, "m")
                         || !strcmp(ptrParameter, "S") || !strcmp(ptrParameter, "q") || !strcmp(ptrParameter, "r") || !strcmp(ptrParameter, "s") || !strcmp(ptrParameter, "u")) {
                strcpy(slots[inMessages[i]->terminal].command, ptrParameter);
              } else {
                // unknown command
                blCheckCommand = false;
              }
            } else {// if (intCount > 0) { // --> intCount>0 --> (!blCheck2dnPrn && blCheckCommand){

              //get the other params
              //check 'p' command
              if (!strcmp(slots[inMessages[i]->terminal].command, "t")) {
                strcpy(slots[inMessages[i]->terminal].parameter, ptrParameter);
                blCheckCommand = true;
              } else if (!strcmp(slots[inMessages[i]->terminal].command, "v")) { //check 'v' command

                //check variable
                if (!(strcmp(ptrParameter, LBL_MQTT_USER)) || !(strcmp(ptrParameter, LBL_MQTT_PASSWORD)) ||
                    !(strcmp(ptrParameter, LBL_MQTT_SERVER)) || !(strcmp(ptrParameter, LBL_MQTT_PORT)) || !(strcmp(ptrParameter, LBL_GSM_ENABLE)) || !(strcmp(ptrParameter, LBL_SET_WO_ACCESS_CODE)) ||
                    !(strcmp(ptrParameter, LBL_OTA_WEB_SERVER)) || !(strcmp(ptrParameter, LBL_OTA_WEB_PORT)) ||
                    !(strcmp(ptrParameter, LBL_OTA_WEB_PAGE)) || !(strcmp(ptrParameter, LBL_TCP_PASSWORD)) || !(strcmp(ptrParameter, LBL_ACCESS_CODE)) ||
                    !(strcmp(ptrParameter, LBL_TCP_PORT)) || !(strcmp(ptrParameter, LBL_WIFI_PASSWORD)) || !(strcmp(ptrParameter, LBL_WIFI_SSID))) {
                  strcpy(slots[inMessages[i]->terminal].parameter, ptrParameter);

                } else {
                  //malformed command
                  blCheckCommand = false;
                }

              } else if (!strcmp(slots[inMessages[i]->terminal].command, "e")) { //check 'e' command

                //check commands that don't need for a value
                if (!strcmp(ptrParameter, "x") || !strcmp(ptrParameter, "y")) {
                  strcpy(slots[inMessages[i]->terminal].parameter, ptrParameter);
                  blCheckValue = true;
                } else if (!strcmp(ptrParameter, "a") || !strcmp(ptrParameter, "at") || !strcmp(ptrParameter, "s") || !strcmp(ptrParameter, "rw")) {
                  strcpy(slots[inMessages[i]->terminal].parameter, ptrParameter);
                } else {
                  //malformed command
                  blCheckCommand = false;
                }
              }
              blCheck2ndPrn = true;
            }
            intCount++;

            //check if a next cycle is needed
            if (!blCheckCommand) {
              break;
            }

            if (blCheck2ndPrn && !blCheckValue) {

              //get value
              if (!ptrEndParameter) {
                //value is missing
                blCheckCommand = false;
              } else {
                strncpy(slots[inMessages[i]->terminal].value, ptrEndParameter, ptrEndCommand - ptrEndParameter);
                blCheckValue = true;
              }
              break;
            }
            ptrParameter = strtok_r (NULL, " ", &ptrEndParameter);
          }

          //execute command
          if (blCheckCommand && blCheck2ndPrn && blCheckValue) {

            if (!strcmp(slots[inMessages[i]->terminal].command, "e")) {

              if (!strcmp(slots[inMessages[i]->terminal].parameter, "rw")) {
                //createMessage(intSub,slots[inMessages[i]->terminal].value, strlen(slots[inMessages[i]->terminal].value));
                blCheckCommand = sendRawCommand(slots[inMessages[i]->terminal].value);
                if (!blCheckCommand) {
                  createMessage (slots[inMessages[i]->terminal].terminal, ERR_WRONG_VALUE, strlen(ERR_WRONG_VALUE));
                }
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, "s")) {
                blCheckCommand = createSequence(slots[inMessages[i]->terminal].terminal, slots[inMessages[i]->terminal].value);
                //no need to send error message

              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, "a")) {
                char *p;
                long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);
                if (result >= 0 and result <= 3) {
                  blCheckCommand = setAlarm(slots[inMessages[i]->terminal].terminal, (armCommand)result);
                  //no need to send error message
                }
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, "at")) {
                blCheckCommand = sendATCommand(slots[inMessages[i]->terminal].value);
                if (!blCheckCommand) {
                  createMessage (slots[inMessages[i]->terminal].terminal, ERR_WRONG_VALUE, strlen(ERR_WRONG_VALUE));
                }
              }
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "h")) {
              loadHelp (inMessages[i]->terminal);

            } else if (!strcmp(slots[inMessages[i]->terminal].command, "i")) {
              //info esp
              String strInfo = F("Chip:\nFree heap:\t\t") + String(ESP.getFreeHeap()) + F("\nVersion:\t\t") + String(VERSION) + F("\n\nVariables:\nGSM_ENABLE:\t\t\"") + String(intGSMEnable) +
                               F("\"\nACCESS_CODE:\t\t\"") + String(strAccessCode) +
                               F("\"\nSET_WO_ACCESS_CODE:\t\"") + String(intSetWOAccessCode) +
                               F("\"\nMQTT_SERVER:\t\t\"") + String(strMQTTServer) + F("\"\nMQTT_PORT:\t\t\"") + String(intMQTTPort) +
                               F("\"\nMQTT_USER:\t\t\"") + String(strMQTTUser) + F("\"\nMQTT_PASSWORD:\t\t\"") + String(strMQTTPassword) +
                               F("\"\nOTA_WEB_SERVER:\t\t\"") + String(strOTAWebServer) + F("\"\nOTA_WEB_PORT:\t\t\"") + String(intOTAWebPort) + F("\"\nOTA_WEB_PAGE:\t\t\"") + String(strOTAWebPage) +
                               F("\"\nTCP_PASSWORD:\t\t\"") + String(strTCPPassword) + F("\"\nTCP_PORT:\t\t\"") + String(intTCPPort) +
                               F("\"\nWIFI_SSID:\t\t\"") + String(strWiFiSSID) + F("\"\nWIFI_PASSWORD:\t\t\"") + String(strWiFiPassword) + F("\"\n");
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));

              //WiFi
              strInfo = F("\nWiFi:\nMAC:\t\t\t") + String(WiFi.macAddress()) + F("\nIP:\t\t\t") + WiFi.localIP().toString() + F("\nTerminals:\t\t") + String(intTerminals) + STR_N;
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));

              //MQTT
              strInfo = F("\nMQTT:\nName:\t\t\t") + String(WiFi.hostname()) + F("\nStatus\t\t\t") + String(mqttClient.connected()) + STR_N;
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));

              //GSM
              strInfo = F("\nGSM:\nStatus:\t\t\t") + gsmStatusToString() + STR_N;
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));

              //Alarm Status
              strInfo = F("\nAlarm Status:\nUp-to-date:\t\t") + cpUpToDateToString() + STR_N;
              if (cpUpToDate) {
                strInfo += F("Mode:\t\t\t") + cpModeToString() + F("\nArmed:\t\t\t") + cpArmedToString() + F("\nTriggered:\t\t") + cpTriggredToString() + F("\nActivated:\t\t") + cpActivatedToString() +
                           F("\nDelayed:\t\t") + cpDelayedToString() + STR_N;
              }
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));

              //Detailed info
              strInfo = F("\nDetails:\nDevice:\t\t\t") + String(cpDevice) + F("\nMessage:\t\t") + cpMessageToString() + F("\nWarning:\t\t") + cpWarningToString() + F("\nBattery:\t\t") + cpBatteryToString() +
                        F("\nA:\t\t\t") + cpAToString() + F("\nB:\t\t\t") + cpBToString() + F("\nC:\t\t\t") + cpCToString() + STR_N;
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));


            } else if (!strcmp(slots[inMessages[i]->terminal].command, "k")) {
              resetGSM();
              //blCheckCommand = false;

            } else if (!strcmp(slots[inMessages[i]->terminal].command, "l")) {
              if (!loadConfig()) {
                //notify error
                createMessage (slots[inMessages[i]->terminal].terminal, ERR_LOAD_CONFIG, strlen(ERR_LOAD_CONFIG));
                blCheckCommand = false;
              }
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "m")) {
              if (intSub & slots[inMessages[i]->terminal].terminal) {
                //disable
                intSub = intSub & (intBroadcast - slots[inMessages[i]->terminal].terminal);
              } else {
                //enable
                intSub += slots[inMessages[i]->terminal].terminal;
              }
              String s = STR_SUBSCRIPTION + String(intSub & slots[inMessages[i]->terminal].terminal ? STR_ON : STR_OFF) + STR_N;
              createMessage (slots[inMessages[i]->terminal].terminal, s.c_str(), s.length());
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "g")) {
              if (intSubGSM & slots[inMessages[i]->terminal].terminal) {
                //disable
                intSubGSM = intSubGSM & (intBroadcast - slots[inMessages[i]->terminal].terminal);
              } else {
                //enable
                intSubGSM += slots[inMessages[i]->terminal].terminal;
              }
              String s = STR_SUBSCRIPTION + String(intSubGSM & slots[inMessages[i]->terminal].terminal ? STR_ON : STR_OFF) + STR_N;
              createMessage (slots[inMessages[i]->terminal].terminal, s.c_str(), s.length());
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "S")) {
              if (intSubRS485 & slots[inMessages[i]->terminal].terminal) {
                //disable
                intSubRS485 = intSubRS485 & (intBroadcast - slots[inMessages[i]->terminal].terminal);
              } else {
                //enable
                intSubRS485 += slots[inMessages[i]->terminal].terminal;
              }
              String s = STR_SUBSCRIPTION + String(intSubRS485 & slots[inMessages[i]->terminal].terminal ? STR_ON : STR_OFF) + STR_N;
              createMessage (slots[inMessages[i]->terminal].terminal, s.c_str(), s.length());

            } else if (!strcmp(slots[inMessages[i]->terminal].command, "t")) {

              char *p;
              long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);

              if (result >= 0 and result <= 3) {
                if (!triggerAlarm(result)) {
                  createMessage (slots[inMessages[i]->terminal].terminal, ERR_TRIGGER, strlen(ERR_TRIGGER));
                  blCheckCommand = false;
                }
              } else {
                blCheckCommand = false;
                createMessage (slots[inMessages[i]->terminal].terminal, ERR_MALFORMED_COMMAND, strlen(ERR_MALFORMED_COMMAND));
              }
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "q")) {
              // close connection
              clients[inMessages[i]->terminal]->close();
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "r")) {
              ESP.restart();

            } else if (!strcmp(slots[inMessages[i]->terminal].command, "s")) {
              if (!saveConfig()) {
                //notify error
                createMessage (slots[inMessages[i]->terminal].terminal, ERR_SAVE_CONFIG, strlen(ERR_SAVE_CONFIG));
                blCheckCommand = false;
              }
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "u")) {
              blCheckCommand = (checkUpdate(inMessages[i]->terminal, false) == HTTP_UPDATE_OK);
              if (!blCheckCommand) {
                createMessage (slots[inMessages[i]->terminal].terminal, ERR_CHECK_UPDATE, strlen(ERR_CHECK_UPDATE));
              }
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "v")) {
              //get the variable
              if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_MQTT_SERVER)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strMQTTServer, slots[inMessages[i]->terminal].value, sizeof(strMQTTServer), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_MQTT_PORT)) {

                char *p;
                long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);

                if (result >= 1 and result <= 65535) {
                  blCheckCommand = true;
                  intMQTTPort = (int)result;
                } else {
                  blCheckCommand = false;
                }

              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_GSM_ENABLE)) {

                char *p;
                long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);

                if (result >= 0 and result <= 1) {
                  blCheckCommand = true;
                  intGSMEnable = (int)result;

                } else {
                  blCheckCommand = false;
                }
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_SET_WO_ACCESS_CODE)) {

                char *p;
                long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);

                if (result >= 0 and result <= 1) {
                  blCheckCommand = true;
                  intSetWOAccessCode = (int)result;

                } else {
                  blCheckCommand = false;
                }
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_MQTT_USER)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strMQTTUser, slots[inMessages[i]->terminal].value, sizeof(strMQTTUser), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_MQTT_PASSWORD)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strMQTTPassword, slots[inMessages[i]->terminal].value, sizeof(strMQTTPassword), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_OTA_WEB_SERVER)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strOTAWebServer, slots[inMessages[i]->terminal].value, sizeof(strOTAWebServer), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_OTA_WEB_PORT)) {
                char *p;
                long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);
                if (result >= 1 and result <= 65535) {
                  blCheckCommand = true;
                  intOTAWebPort = (int)result;
                } else {
                  blCheckCommand = false;
                }
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_OTA_WEB_PAGE)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strOTAWebPage, slots[inMessages[i]->terminal].value, sizeof(strOTAWebPage), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_TCP_PASSWORD)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strTCPPassword, slots[inMessages[i]->terminal].value, sizeof(strTCPPassword), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_TCP_PORT)) {

                char *p;
                long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);
                if (result >= 1 and result <= 65535) {
                  blCheckCommand = true;
                  intTCPPort = (int)result;
                } else {
                  blCheckCommand = false;
                }
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_ACCESS_CODE)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strAccessCode, slots[inMessages[i]->terminal].value, sizeof(strAccessCode), charAllowDigits));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_WIFI_PASSWORD)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strWiFiPassword, slots[inMessages[i]->terminal].value, sizeof(strWiFiPassword), charAllowAll));
              } else if (!strcmp(slots[inMessages[i]->terminal].parameter, LBL_WIFI_SSID)) {
                blCheckCommand = (setStringVariable(slots[inMessages[i]->terminal].terminal, strWiFiSSID, slots[inMessages[i]->terminal].value, sizeof(strWiFiSSID), charAllowAll));
              }
            }
          } else {
            //notify error in command parsing
            createMessage (slots[inMessages[i]->terminal].terminal, ERR_MALFORMED_COMMAND, strlen(ERR_MALFORMED_COMMAND));
            blCheckCommand = false;
          }
          if (blCheckCommand) {
            //notify end execution
            createMessage (slots[inMessages[i]->terminal].terminal, INFO_COMMAND_OK, strlen(INFO_COMMAND_OK));
          }
          ptrCommand = strtok_r (NULL, "\n", &ptrEndCommand);
        } //ptrCommand != NULL
      }
      //clear the incoming message
      inMessages[i]->terminal = 0; //useless
    }  //for each message
  } //inMessages.size >0

  //remove executed inMessages
  if (inMessages.size() > 0) {
    for (int i = inMessages.size() - 1; i >= 0; i--) {
      //if (!inMessages[i]->terminal){
      delete inMessages[i];
      inMessages.erase(inMessages.begin() + i);
      //}
    }
  }

  //send reply
  if (outMessages.size() > 0) {
    for (int i = 0; i < outMessages.size(); i++) {
      for (int j = 0; j < MAX_TCP_CONNECTIONS; j++) {
        if ((slots[j].terminal & outMessages[i]->terminal) && slots[j].active) {
          if (!slots[j].busy) {
            if (addDataToMessage(clients[j], outMessages[i]->data, outMessages[i]->length)) {
              //outMessages[i]->active = false;
              slots[j].hasData = true;
              outMessages[i]->terminal -= slots[j].terminal;
            } else {
              slots[j].busy = true;
            }
          }
        }
      }
    }
  } else {
    // free memory
    outMessages.clear();
    outMessages.reserve (MAX_TCP_MESSAGES);
  }

  //commit unsent messages
  for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    if (slots[i].active && slots[i].hasData) {
      sendMessage(clients[i]);
      slots[i].hasData = false;
    }
  }

  //remove sent messages
  if (outMessages.size() > 0) {
    for (int i = outMessages.size() - 1; i >= 0; i--) {
      if (!outMessages[i]->terminal  || !intTerminals) {
        delete outMessages[i];
        outMessages.erase(outMessages.begin() + i);
      }
    }
  }

  /*
    ----------------------------------------------
    MQTT MANAGEMENT
    ----------------------------------------------
  */
  if (!mqttClient.connected() && (m > lngLastReconnectAttempt + MQTT_RECONNECT_TIMER)) {
    if (mqttReconnect()) {
      mqttRefreshAlarm();
      mqttRefreshInfo();
    } else {
      lngLastReconnectAttempt = m;
    }
  } else {
    // Client connected
    if (m > lngLastLoopCall + MQTT_LOOP_TIMER) {
      mqttClient.loop();
      lngLastLoopCall = m;
    }
  }

  if (m - lngLastPublishAttempt > MQTT_PUBLISH_TIMER) {
    lngLastPublishAttempt = m;

    //refresh all
    mqttRefreshAlarm();
    mqttRefreshInfo();
    mqttRefreshDevice();
  } else {
    if (cpUpToDate == 2) {
      //refresh alarm state
      mqttRefreshAlarm();
    }
    if (isCPInfo) {
      //refresh alarm state
      mqttRefreshInfo();
      isCPInfo = false;
    }
    if (isCPDevice) {
      //refresh device state
      mqttRefreshDevice();
      isCPDevice = false;
    }
    if (isCPMessage) {
      //refresh message state
      mqttRefreshMessage();
      isCPMessage = false;
    }
  }

  /*
    ----------------------------------------------
    RS485 MANAGEMENT
    ----------------------------------------------
  */

  if (intSerialInput > 0) {

    ledBlink = LOW;
    digitalWrite(LED_PIN, ledBlink);
    lngLedBlinkTimer = m;
    delay(1);

    uint8_t buf[intSerialInput];

    intSerialInput = Serial.readBytes(buf, intSerialInput);
    if (intSerialInput) { //data ok
      lngLastSerialUpdateTime = m;
      for (int i = 0; i <= intSerialInput - 1; i++) {

        // wait for the beginning of the packet
        if ( (( !(buf[i] & SERIAL_PK_BEGIN_MASK) || (buf[i] == SERIAL_PK_END)) && serInputLen ) || ((buf[i] & SERIAL_PK_BEGIN_MASK) && !serInputLen ) ) { //first byte control

          //add to buffer
          serInput[serInputLen] = buf[i];
          serInputLen++;
          if (serInputLen >= MAX_SERIAL_PK_LEN) {
            if (intSub) {
              createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
            }
            serInputLen = 0;
          } else {

            if ( buf[i] == SERIAL_PK_END) { // end of packet.
              bool crcResult = false;

              if (intSubRS485) { //sniffing
                int len = (serInputLen * 3) + 2;
                char strHex[len];
                for (int j = 0; j <= serInputLen; j++) {
                  sprintf(strHex + (j * 3), "%02X ", serInput[j]);

                }
                strHex[len - 2] = '\n';
                strHex[len - 1] = 0;

                createMessage(intSubRS485, strHex, len);
              } //intSubRS485

              //calculate CRC
              crcResult = !calculateCRC((uint8_t *)serInput, serInputLen - 1);

              //decode packet
              if (crcResult) {

                //check 1st byte (heading)
                switch (serInput[0]) {
                  case 0xED: case 0xF0: //Control Panel Message

                    //get Control Panel attributes
                    lngLastCPUpdateTime = m;
                    cpUpToDate = (cpStatus == serInput[1]) ? cpUpToDate = 1 : cpUpToDate = 2;
                    cpStatus = serInput[1];

                    //get Message
                    isCPMessage = (cpMessage != serInput[2]);
                    cpMessage = serInput[2];

                    //get Device
                    isCPDevice = (cpDevice != serInput[3]);
                    cpDevice = serInput[3];

                    //get info from Control Panel
                    isCPInfo = (cpInfo != serInput[4]);
                    cpInfo = serInput[4];
                    break;

                  case 0xE3: case 0xE4: case 0xE7: //configuration?
                    break;
                  case 0xE5:
                    break;
                  case 0xE6:
                    break;
                  case 0xEC: // text declaration (ie: device rename)
                    // EC 60 [device] [TEXT+\0] [CRC] FF
                    //      ie: EC 60 01 74 65 73 74 31 00 76 FF (renames device #1 in "test1")
                    // EC 78 03 02 00 -> GSM Communicator
                    // EC 78 03 02 02 -> ARC Alarm
                    // EC 38 -> Reply
                    break;
                  case 0xEF:
                    break;
                  case 0xE9:
                    break;
                  case 0xC7: case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD:
                    break;
                  case 0xCE: case 0xCF: case 0xE0: case 0xE1: case 0xE2: case 0xEA: case 0xEB: case 0xEE:
                    break;
                  case 0xE8: //Status Change
                    switch (serInput[1]) {
                      // 02=Trigger delayed, 04=Trigger Immediate, 0B=Maintenance mode, 0C=Service mode, 0D=Query device, 0E=Normal Mode, 0F=??
                      case 0x0D:
                        //broadcast gsm
                        uint8_t s[9];
                        //memcpy_P(s,SER_MSG_GSM_ECHO,2);
                        //createStream(s, 2);
                        memcpy_P(s,SER_MSG_GSM,9);
                        createStream(s, 9);
                        break;
                    }
                    break;
                  case 0xC6: //Pong from Keypad
                  case 0xB0: //?
                  case 0xB7: //?
                  case 0xB8: //?
                    break;
                  case 0xB5: //events in memory
                    break;
                  case 0xB2: //0xB2 tamper keypad command
                    break;
                  case 0x8F: case 0x8E: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: case 0x89:
                    //Keypad *,#, 0-9

                    lngLastCommandSent = m;
                    busStatus = busStatus | BUS_BUSY;
                    // if wait response then cancel sequence
                    if (busStatus & BUS_WAITING_ACK) {
                      //error
                      if (intSub) {
                        createMessage (intSub, ERR_SEQUENCE_CANCELLED, strlen(ERR_SEQUENCE_CANCELLED));
                      }
                      //cancel any sequence
                      emptySerialOutput();
                     
                    }
                    break;
                  case 0x8A:
                    //Control Panel Settings
                    lngLastCommandSent = m;
                    busStatus = busStatus | BUS_BUSY;
                    // if wait response then cancel sequence
                    if (busStatus & BUS_WAITING_ACK) {
                      //error
                      if (intSub) {
                        createMessage (intSub, ERR_SEQUENCE_CANCELLED, strlen(ERR_SEQUENCE_CANCELLED));
                      }
                      //cancel sequence
                      emptySerialOutput();
                    }
                    break;
                  case 0xA0: case 0xA5:
                    //Command Accepted/Authenticated
                    lngLastCommandSent = m;
                    if (!(busStatus & BUS_BUSY)) {
                      busStatus = BUS_FREE;
                    }
                    break;
                  case 0xA1: case 0xA3:
                    //Command Executed
                    busStatus = BUS_FREE;
                    break;
                  case 0xA4:
                    //Command Expired/Cancelled/Forbidden
                    if (busStatus & BUS_WAITING_ACK) {
                      //error
                      if (intSub) {
                        createMessage (intSub, ERR_SEQUENCE_REJECTED, strlen(ERR_SEQUENCE_REJECTED));
                      }
                      //cancel sequence
                     emptySerialOutput();
                    }
                    busStatus = BUS_FREE;
                    break;
                  default:
                    //unknown packet
                    if (intSub) {
                      createMessage (intSub, ERR_UNKNOWN_DATA, strlen(ERR_UNKNOWN_DATA));
                    }
                }

              } else { //crcRersult
                if (intSub) {
                  createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
                }
              }

              //empty the buffer
              serInputLen = 0;

            } // end of Packet
          } // serInputLen >=SERIAL_PK_LEN
        } // first byte control
      } // for i

    } //intSerialInput <>0

  } //intSerialInput > 0


  /*
    ----------------------------------------------
    TIMERS
    ----------------------------------------------
  */
  if (!ledBlink && (m >= (lngLedBlinkTimer + LED_INTERVAL_TIMER))) {
    ledBlink = HIGH;
    digitalWrite(LED_PIN, ledBlink);
  }

  if (busStatus && (m >= (lngLastCommandSent + BUS_MAX_INTERVAL_TIMER))) {
    //command expiration
    //if ((busStatus & BUS_WAITING_ACK) || (busStatus & BUS_IGNORE_ACK)) {
    if (busStatus & BUS_WAITING_ACK) {
      //***serSeqLen = SERIAL_SEQ_LEN;
      if (intSub) {
        createMessage (intSub, ERR_SEQUENCE_CANCELLED, strlen(ERR_SEQUENCE_CANCELLED));
      }
      emptySerialOutput();
    }
    busStatus = BUS_FREE;
  }

  if (m >= (lngLastSerialUpdateTime + SERIAL_MAX_INTERVAL_TIMER)) {
    //packet expiration
    if (intSub) {
      createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
    }
    // free memory
    emptySerialOutput();
    lngLastSerialUpdateTime = m;
  }

  //test up-to-date
  if (m >= (lngLastCPUpdateTime + CP_MAX_INTERVAL_TIMER)) {
    //data expired
    lngLastCPUpdateTime = m;
    cpUpToDate = 0;
    isCPInfo = false;
    isCPDevice = false;
    isCPMessage = false;
  }
}
