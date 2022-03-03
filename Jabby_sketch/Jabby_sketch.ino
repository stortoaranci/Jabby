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
  int lenght = -1;
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

//Led
bool ledBlink = HIGH;
long lngLedBlinkTimer = 0;

//Time
long m = 0;

//Console
TCPSlot slots[MAX_TCP_CONNECTIONS];
//static std::vector<AsyncClient*> clients[MAX_TCP_CONNECTIONS]; // a list to hold all clients
static AsyncClient* clients[MAX_TCP_CONNECTIONS];
std::vector<TCPMessage*> inMessages;    // a list of incoming messages from clients
std::vector<TCPMessage*> outMessages;   // a list of outgoing messages from clients
int intBroadcast = 0;                   //holds the broadcast for all the slots
int intSub = 0;                         //holds the terminals that want to receive broadcast messages from server;
int intSubRS485 = 0;                    //holds the terminals that want to receive RS485 packets from server;
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
uint8_t serMessage[SERIAL_PK_LEN];
uint8_t serPacketLen = 0;
long lngLastSerialUpdateTime = 0;
int intSerialInput = 0;
uint8_t serSequence[SERIAL_SEQ_LEN];
uint8_t serSeqLen = SERIAL_SEQ_LEN; //just over the end of the array
uint8_t busStatus = 0;
long lngLastCommandSent = 0;

//GSM
SoftwareSerial ssGSM;
//SoftwareSerial ssGSM(GSM_RX_PIN, GSM_TX_PIN);
int intGSMInput = 0;
long lngGSMResetTime = 0;

//Control Panel
uint8_t cpStatus =   0xFF; //last known CP status

uint8_t cpUpToDate = 0;         // the state of the CP is up-to-date. 0 = not updated; 1 = updated; 2 = force mqtt refresh
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
    //outMessages.back()->lenght = sizeData;
    //outMessages.back()->active = true;
    if (sizeData > MAX_TCP_MESSAGE_LEN) { //send messages in chunk
      memcpy(outMessages.back()->data, ptrData, MAX_TCP_MESSAGE_LEN);
      outMessages.back()->lenght = MAX_TCP_MESSAGE_LEN;
      ptrData += MAX_TCP_MESSAGE_LEN;
      sizeData -= MAX_TCP_MESSAGE_LEN;
    } else {
      memcpy(outMessages.back()->data, ptrData, sizeData);
      outMessages.back()->lenght = sizeData;
      sizeData = 0;
    }
  }
  return true;
}

bool addDataToMessage(AsyncClient* client, const char* data, size_t len ) {
  if (client->space() > len) {
    client->add(data, len);
    //client->send();
    return true;
  }  else {
    return false;
  }
}

bool sendMessage(AsyncClient* client) {
  //if (client == nullptr){ return false;}
  if (client->canSend()) {
    //client->add(data, len);
    client->send();
    return true;
  }  else {
    return false;
  }
}

//MQTT
void mqttRefresh() {

  String t;
  String s;

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_ONLINE;
  s = String(cpUpToDate != 0);
  mqttClient.publish(t.c_str(), s.c_str());

  t = MQTT_PREFIX + WiFi.hostname() + MQTT_ALARM + + MQTT_STATE;
  s = mqttStateToString();
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
      mqttRefresh();
    }
  } else {
    if (cpUpToDate) {
      if (!strcmp(strMQTTCommandTopic.c_str(), topic)) {
        char p[len + 1];
        memset(p, 0, sizeof(p));
        memcpy(p, (char*)payload, len);

        if (!strcmp(MQTT_PAYLOAD_ARM_AWAY, p) && (cpStatus & SERIAL_PK_MODE_MASK )) {
          setSequence(-1, STR_ARM_ABC_COMMAND);
        } else if (!strcmp(MQTT_PAYLOAD_ARM_HOME, p) && (cpStatus & SERIAL_PK_MODE_MASK )) {
          setSequence(-1, STR_ARM_A_COMMAND);
        } else if (!strcmp(MQTT_PAYLOAD_ARM_NIGHT, p) && (cpStatus & SERIAL_PK_MODE_MASK )) {
          setSequence(-1, STR_ARM_B_COMMAND);
        } else if (!strcmp(MQTT_PAYLOAD_DISARM, p) && (cpStatus & SERIAL_PK_ARMED_MASK )) {
          //disarm
          setSequence(-1, strAccessCode);
        } else if (!strcmp(MQTT_PAYLOAD_TRIGGER, p) ) {
          //trigger
          triggerAlarm(1);
          delay (TRIGGER_INTERVAL_TIMER);
          triggerAlarm(0);
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
      inMessages.back()->lenght = len;
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
  if (cpStatus & SERIAL_PK_FIRED) {
    return STR_MQTT_TRIGGERED;
  }
  if (cpStatus & SERIAL_PK_TRIGGERED) {
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

String gsmStatusToString (gsmState state) {

  switch (state) {
    case gsmStateNotFound: {
        return STR_NOT_FOUND;
      }
    case gsmStateEnabled: {
        return STR_ENABLED;
      }
    default: {
        return STR_DISABLED;
      }
  }
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

String cpFiredToString() {
  return (cpStatus & SERIAL_PK_FIRED ) ? STR_YES : STR_NO;
}

String cpTriggeredToString() {
  return (cpStatus & SERIAL_PK_TRIGGERED) ? STR_YES : STR_NO;
}

String cpDelayedToString() {
  return (cpStatus & SERIAL_PK_DELAYED) ? STR_YES : STR_NO;
}


//Instructions
void triggerAlarm (uint8_t t) {

  switch (t) {
    case 1:
      analogWrite(TRIGGER_PIN, TRIGGER_DEVICE);
      break;
    case 2:
      analogWrite(TRIGGER_PIN, TRIGGER_TAMPER);
      break;
    default:
      analogWrite(TRIGGER_PIN, TRIGGER_NONE);
  }
}

bool initGSM() {
  if (ssGSM) {
    ssGSM.end();
  }
  if (intGSMEnable) {

    ssGSM.begin(57600, SWSERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN, false); //,256);
    //ssGSM.begin(9600);
  }
  return ssGSM;
}

void resetGSM() {
  if (ssGSM) {
    ssGSM.end();
  }
  lngGSMResetTime = m;
  digitalWrite(GSM_RST_PIN, HIGH);
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

bool setSequence(int terminal, const char* command) {

  if (serSeqLen != SERIAL_SEQ_LEN) { //running command
    if (terminal >= 0) {
      createMessage (terminal, ERR_DEVICE_BUSY, strlen(ERR_DEVICE_BUSY));
    }
    return false;
  }
  if (!setStringVariable(terminal, (char *)serSequence, command, SERIAL_SEQ_LEN, charAllowKeypad)) {
    return false;
  }

  uint8_t *p = &serSequence[0];
  while (*p) {
    //convert the buffer for rs485
    if (isdigit(*p)) {
      *p = (*p | SERIAL_PK_BEGIN_MASK) & 0xCF;
    } else {
      *p = (*p == '*') ? 0x8F : 0x8E;
    }
    p++;
  }
  serSeqLen = 0; //enable sequence
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
      delay(10);
    } else {
      //delay (5);
      digitalWrite(REDE_PIN, LOW);
      return false;
    }
    ptrCommand = strtok (NULL, " ");
  }
  delay (10);
  digitalWrite(REDE_PIN, LOW);
  return true;
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

#ifdef localSerial
  Serial.begin(9600);
  while (!Serial) {
    delay (10);
  }
  Serial.println();
  Serial.printf(F("Booting ver. %s (%s)\n"), VERSION, WIFI_SSID);
#endif

  //assign a terminal id to each slot
  byte b = 1;
  for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    intBroadcast += b;
    slots[i].terminal = b;
    b = b << 1;
  }

#ifdef localSerial
  Serial.println(F("Get Chip ID"));
  Serial.println(ESP.getChipId());
  Serial.println(F("Mounting FS..."));
#endif

  if (!LittleFS.begin()) {

#ifdef localSerial
    Serial.println(F("Failed to mount file system"));
#endif
  }

  if (!loadConfig()) {
#ifdef localSerial
    Serial.println(F("Failed to load config"));
  } else {
    Serial.println(F("Config loaded"));
#endif
  }

  // GSM
  if (intGSMEnable) {

    if (!initGSM()) {
#ifdef localSerial
      Serial.println(F("Failed to start GSM"));
#endif
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(strWiFiSSID, strWiFiPassword);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
#ifdef localSerial
    Serial.println(F("Connection Failed! Try default settings..."));
#endif
    delay(5000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
#ifdef localSerial
      Serial.println(F("Connection Failed! Reboot..."));
#endif
      ESP.restart();
    }
  }

  String s;
  s = String(NAME) + String(WiFi.macAddress());
  s.replace(":", "");
  WiFi.hostname(s);
#ifdef localSerial
  Serial.println(WiFi.localIP().toString());
#endif


  //check for updates
#ifdef localSerial
  Serial.printf(F("Check update: %d\n"), checkUpdate(-1, true));
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
    delay (10);
  }

  strMQTTCommandTopic = MQTT_PREFIX + WiFi.hostname() + MQTT_ALARM + MQTT_COMMAND;
  strMQTTCommandTopicTR1 = String(MQTT_PREFIX) + String(MQTT_COMMAND);
}

void loop() {

  m = millis();

  //look output from serial
  intSerialInput = Serial.available();

  //get output from GSM
  if (ssGSM) {
    int intGSMInput = ssGSM.available();
    if (intGSMInput > 0) {
      uint8_t buf[intGSMInput];
      intGSMInput = ssGSM.readBytes(buf, intGSMInput);
      if (intGSMInput) { //data ok
        if (intSub) {
          createMessage (intSub, (char*)buf, intGSMInput);
        }
      }
    }
  }

  //send sequence
  if ((serSeqLen < SERIAL_SEQ_LEN) && !busStatus && !serPacketLen) {
    lngLastCommandSent = m;
    busStatus = BUS_WAITING_RESPONSE;
    digitalWrite(REDE_PIN, HIGH);
    delay(1);
    Serial.write(serSequence[serSeqLen]);
    //delay(20);
    Serial.write(SERIAL_PK_END);
    delay(3); // this delay is needed to avoid feedback in incoming traffic (investigation needed)
    digitalWrite(REDE_PIN, LOW);
    serSeqLen++;
    if (!serSequence[serSeqLen]) {
      serSeqLen = SERIAL_SEQ_LEN;
    }
  }

  //TCP Management
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
        strncpy(password, inMessages[i]->data, inMessages[i]->lenght);
        password[inMessages[i]->lenght] = NULL;

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
        strncpy(command, inMessages[i]->data, inMessages[i]->lenght);
        command[inMessages[i]->lenght] = NULL;

        char *ptrEndCommand;
        char *ptrCommand = strtok_r(command, "\r\n", &ptrEndCommand);

        //if '\r\n' is missing then calculate the whole message as a single command (putty)
        if (!ptrEndCommand) {
          ptrEndCommand = ptrCommand + inMessages[i]->lenght;
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
              } else if (!strcmp(ptrParameter, "h") || !strcmp(ptrParameter, "i") || !strcmp(ptrParameter, "k") || !strcmp(ptrParameter, "l") || !strcmp(ptrParameter, "m")
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
                    !(strcmp(ptrParameter, LBL_MQTT_SERVER)) || !(strcmp(ptrParameter, LBL_MQTT_PORT)) || !(strcmp(ptrParameter, LBL_GSM_ENABLE)) ||
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
                } else if (!strcmp(ptrParameter, "rw") || !strcmp(ptrParameter, "at") || !strcmp(ptrParameter, "s")) {
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
                blCheckCommand = setSequence(slots[inMessages[i]->terminal].terminal, slots[inMessages[i]->terminal].value);
                //no need to send error message

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
                               F("\"\nMQTT_SERVER:\t\t\"") + String(strMQTTServer) + F("\"\nMQTT_PORT:\t\t\"") + String(intMQTTPort) +
                               F("\"\nMQTT_USER:\t\t\"") + String(strMQTTUser) + F("\"\nMQTT_PASSWORD:\t\t\"") + String(strMQTTPassword) +
                               F("\"\nOTA_WEB_SERVER:\t\t\"") + String(strOTAWebServer) + F("\"\nOTA_WEB_PORT:\t\t\"") + String(intOTAWebPort) + F("\"\nOTA_WEB_PAGE:\t\t\"") + String(strOTAWebPage) +
                               F("\"\nTCP_PASSWORD:\t\t\"") + String(strTCPPassword) + F("\"\nTCP_PORT:\t\t\"") + String(intTCPPort) +
                               F("\"\nWIFI_SSID:\t\t\"") + String(strWiFiSSID) + F("\"\nWIFI_PASSWORD:\t\t\"") + String(strWiFiPassword) + F("\"\n");
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));

              /*
                                          //mqtt
                                          strInfo = "\nMQTT:\nName:\t\t\t" + String(WiFi.hostname()) + "\nStatus\t\t\t" + String(mqttClient.connected())
                                          + "\nCommand:\t\t" + String(r.mqttCommand) + "\nState:\t\t\t" + mqttStateToString(r.mqttState) + String ("\n");
                                          createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));
              */
              //WiFi
              strInfo = F("\nWiFi:\nMAC:\t\t\t") + String(WiFi.macAddress()) + F("\nIP:\t\t\t") + WiFi.localIP().toString() + F("\nTerminals:\t\t") + String(intTerminals) + STR_N;
              createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));
              //GSM
              /*
                strInfo = "\nGSM:\nStatus:\t\t\t" + String(gsmStatusToString(getGSMStatus())) + "\n";
                //strInfo = "\nGSM:\nStatus:\t\t\t" + String(getGSM()) + "\n";
                createMessage(slots[inMessages[i]->terminal].terminal, strInfo.c_str(), strlen(strInfo.c_str()));
              */


              //Control Panel
              strInfo = F("\nControl Panel:\nUp-to-date:\t\t") + String(cpUpToDate) + STR_N;
              if (cpUpToDate) {
                strInfo += F("Mode:\t\t\t") + cpModeToString() + F("\nArmed:\t\t\t") + cpArmedToString() + F("\nFired:\t\t\t") + cpFiredToString() + F("\nTriggered:\t\t") + cpTriggeredToString() +
                           F("\nDelayed:\t\t") + cpDelayedToString() + STR_N;
              }
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
              String s = F("Subscription: ") + String(intSub & slots[inMessages[i]->terminal].terminal ? STR_ON : STR_OFF) + STR_N;
              createMessage (slots[inMessages[i]->terminal].terminal, s.c_str(), s.length());
            } else if (!strcmp(slots[inMessages[i]->terminal].command, "S")) {
              if (intSubRS485 & slots[inMessages[i]->terminal].terminal) {
                //disable
                intSubRS485 = intSubRS485 & (intBroadcast - slots[inMessages[i]->terminal].terminal);
              } else {
                //enable
                intSubRS485 += slots[inMessages[i]->terminal].terminal;
              }
              String s = F("Subscription: ") + String(intSubRS485 & slots[inMessages[i]->terminal].terminal ? STR_ON : STR_OFF) + STR_N;
              createMessage (slots[inMessages[i]->terminal].terminal, s.c_str(), s.length());

            } else if (!strcmp(slots[inMessages[i]->terminal].command, "t")) {

              char *p;
              long result = strtol(slots[inMessages[i]->terminal].value, &p, 10);

              if (result >= 0 and result <= 2) {
                if (blCheckCommand) {
                  triggerAlarm(result);
                } else {
                  //createMessage (slots[inMessages[i]->terminal].terminal, ERR_CHANGE_MODE, strlen(ERR_CHANGE_MODE));
                }
              } else {
                blCheckCommand = false;
                createMessage (slots[inMessages[i]->terminal].terminal, ERR_MALFORMED_COMMAND, strlen(ERR_MALFORMED_COMMAND));
              }
              if (!blCheckCommand) {
                //notify error
                //createMessage (slots[inMessages[i]->terminal].terminal, ERR_CHANGE_MODE, strlen(ERR_CHANGE_MODE));
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
            if (addDataToMessage(clients[j], outMessages[i]->data, outMessages[i]->lenght)) {
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

  //MQTT
  if (!mqttClient.connected() && (m > lngLastReconnectAttempt + MQTT_RECONNECT_TIMER)) {
    if (mqttReconnect()) {
      mqttRefresh();
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

  if ((m - lngLastPublishAttempt > MQTT_PUBLISH_TIMER) || cpUpToDate == 2) {
    lngLastPublishAttempt = m;
    mqttRefresh();
  }

  // get data from rs485
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
        if ( (( !(buf[i] & SERIAL_PK_BEGIN_MASK) || (buf[i] == SERIAL_PK_END)) && serPacketLen ) || ((buf[i] & SERIAL_PK_BEGIN_MASK) && !serPacketLen ) ) { //first byte control

          //add to buffer
          serMessage[serPacketLen] = buf[i];
          serPacketLen++;
          if (serPacketLen >= SERIAL_PK_LEN) {
            if (intSub) {
              createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
            }
            serPacketLen = 0;
          } else {

            if ( buf[i] == SERIAL_PK_END) { // end of packet.
              bool crcResult = false;

              if (intSubRS485) { //sniffing
                int len = (serPacketLen * 3) + 2;
                char strHex[len];
                for (int j = 0; j <= serPacketLen; j++) {
                  sprintf(strHex + (j * 3), "%02X ", serMessage[j]);

                }
                strHex[len - 2] = '\n';
                strHex[len - 1] = 0;

                createMessage(intSubRS485, strHex, len);
              } //intSubRS485

              //calculate CRC
              if (serPacketLen > 2) {
                int magicNumber = 0x7F;
                for (int j = 0; j < serPacketLen - 1; j++) {
                  int c = serMessage[j];
                  for (int k = 8; k > 0; k--) {
                    magicNumber += magicNumber;
                    if (c & 0x80) {
                      magicNumber += 1;
                    }
                    if (magicNumber & SERIAL_PK_BEGIN_MASK) {
                      magicNumber ^= 0xA3;
                    }
                    c += c;
                  } // for k
                } // for i
                crcResult = !magicNumber;
              } else {
                crcResult = true;
              }

              //decode packet
              if (crcResult) {

                //check 1st byte (heading)
                switch (serMessage[0]) {
                  case 0xED: case 0xF0: //Control Panel Message

                    //get Control Panel attributes
                    lngLastCPUpdateTime = m;
                    cpUpToDate = (cpStatus == serMessage[1]) ? cpUpToDate = 1 : cpUpToDate = 2;
                    cpStatus = serMessage[1];
                    break;

                  case 0xE3: case 0xE4: case 0xE7:
                    break;
                  case 0xE5:
                    break;
                  case 0xE6:
                    break;
                  case 0xEC:
                    break;
                  case 0xEF:
                    break;
                  case 0xC7: case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD:
                    break;
                  case 0xCE: case 0xCF: case 0xE0: case 0xE1: case 0xE2: case 0xE8: case 0xEA: case 0xEB: case 0xEE:
                    break;
                  case 0xC6:
                    //data from Keypad
                    break;
                  case 0x8F: case 0x8E: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: case 0x89:
                    //Keypad *,#, 0-9

                    lngLastCommandSent = m;
                    busStatus = busStatus | BUS_BUSY;
                    // if wait response then cancel sequence
                    if (busStatus & BUS_WAITING_RESPONSE) {
                      //error
                      if (intSub) {
                        createMessage (intSub, ERR_SEQUENCE_CANCELLED, strlen(ERR_SEQUENCE_CANCELLED));
                      }
                      //cancel sequence
                      serSeqLen = SERIAL_SEQ_LEN;
                    }
                    break;
                  case 0x8A:
                    //Control Panel Settings
                    lngLastCommandSent = m;
                    busStatus = busStatus | BUS_BUSY;
                    // if wait response then cancel sequence
                    if (busStatus & BUS_WAITING_RESPONSE) {
                      //error
                      if (intSub) {
                        createMessage (intSub, ERR_SEQUENCE_CANCELLED, strlen(ERR_SEQUENCE_CANCELLED));
                      }
                      //cancel sequence
                      serSeqLen = SERIAL_SEQ_LEN;
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
                    if (busStatus & BUS_WAITING_RESPONSE) {
                      //error
                      if (intSub) {
                        createMessage (intSub, ERR_SEQUENCE_REJECTED, strlen(ERR_SEQUENCE_REJECTED));
                      }
                      //cancel sequence
                      serSeqLen = SERIAL_SEQ_LEN;
                    }
                    busStatus = BUS_FREE;
                    break;

                  default:
                    //unknown packet
                    if (intSub) {
                      createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
                    }
                }

              } else { //crcRersult
                if (intSub) {
                  createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
                }
              }

              //empty the buffer
              serPacketLen = 0;

            } // end of Packet
          } // serPacketLen >=SERIAL_PK_LEN
        } // first byte control
      } // for i

    } //intSerialInput <>0

  } //intSerialInput > 0


//TIMERS
  if (!ledBlink && (m >= (lngLedBlinkTimer + LED_INTERVAL_TIMER))) {
    ledBlink=HIGH;
    digitalWrite(LED_PIN, ledBlink);
  }

  if (busStatus && (m >= (lngLastCommandSent + BUS_MAX_INTERVAL_TIMER))) {
    //command expiration
    if (busStatus & BUS_WAITING_RESPONSE) {
      serSeqLen = SERIAL_SEQ_LEN;
      if (intSub) {
        createMessage (intSub, ERR_SEQUENCE_CANCELLED, strlen(ERR_SEQUENCE_CANCELLED));
      }
    }
    busStatus = BUS_FREE;
  }

  if (m >= (lngLastSerialUpdateTime + SERIAL_MAX_INTERVAL_TIMER)) {
    //packet expiration
    if (intSub) {
      createMessage (intSub, ERR_INCOMING_DATA, strlen(ERR_INCOMING_DATA));
    }
    serPacketLen = 0;
    lngLastSerialUpdateTime = m;
  }

  //test up-to-date
  if (m >= (lngLastCPUpdateTime + CP_MAX_INTERVAL_TIMER)) {
    //data expired
    lngLastCPUpdateTime = m;
    cpUpToDate = 0;
  }

  // check reset GSM command status
  if (digitalRead(GSM_RST_PIN)) {
    if (m > (lngGSMResetTime + GSM_INTERVAL_TIMER)) {
      digitalWrite(GSM_RST_PIN, LOW);
      if (intGSMEnable) {
        initGSM();
      }
    }
  }
}
