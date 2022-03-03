//#define localSerial

#define LED_PIN D4
#define REDE_PIN D7
#define TRIGGER_PIN D5
#define GSM_RST_PIN D6
#define GSM_RX_PIN D2
#define GSM_TX_PIN D1
#define LED_PIN D4

const PROGMEM char* VERSION = "Jabby_0.1.6";
const PROGMEM char* NAME ="Jabby";

const PROGMEM char*  DATA_FILENAME = "/data.json";
const PROGMEM char*  HELP_FILENAME = "/help.txt";

#define MAX_TCP_CONNECTIONS   3
#define MAX_TCP_MESSAGES      5
#define MAX_TCP_MESSAGE_LEN   128
#define MAX_TCP_AUTH_ATTEMPTS 3

//SERIAL
#define SERIAL_PK_LEN 24

#define SERIAL_PK_BEGIN_MASK              0x80
#define SERIAL_PK_END                     0xFF
#define SERIAL_PK_ARMED_MASK              0x03
#define SERIAL_PK_ARMED_A                 0x01
#define SERIAL_PK_ARMED_B                 0x02
#define SERIAL_PK_ARMED_ABC               0x03
#define SERIAL_PK_FIRED                   0x04
#define SERIAL_PK_TRIGGERED               0x08
#define SERIAL_PK_DELAYED                 0x10
#define SERIAL_PK_MODE_MASK               0x60
#define SERIAL_PK_MODE_SERVICE            0
#define SERIAL_PK_MODE_MASTER             0x20
#define SERIAL_PK_MODE_NORMAL_UNSPLITTED  0x40
#define SERIAL_PK_MODE_NORMAL_SPLITTED    0x60

#define SERIAL_SEQ_LEN 16

//BUS
#define BUS_FREE              0
#define BUS_WAITING_RESPONSE  1
#define BUS_BUSY              2

//GSM
typedef enum {
  gsmStateNotFound            =0,
  gsmStateEnabled             =1,
  gsmStateDisabled            =2
} gsmState;

//timers
#define LED_INTERVAL_TIMER            5
#define SERIAL_MAX_INTERVAL_TIMER     500
#define GSM_INTERVAL_TIMER            230
#define CP_MAX_INTERVAL_TIMER         1000
#define BUS_MAX_INTERVAL_TIMER        10000
#define TRIGGER_INTERVAL_TIMER        1000
#define MQTT_RECONNECT_TIMER          10000
#define MQTT_LOOP_TIMER               1000
#define MQTT_PUBLISH_TIMER            60000
//#define WAIT_VERY_LONG 900000

//TRIGGER
#define TRIGGER_NONE    55
#define TRIGGER_DEVICE  150
#define TRIGGER_TAMPER  0

//STRINGS
const PROGMEM char* STR_N ="\n";
const PROGMEM char* STR_ON ="on";
const PROGMEM char* STR_OFF ="off";
const PROGMEM char* STR_YES ="Yes";
const PROGMEM char* STR_NO ="No";

const PROGMEM char* STR_DISABLED ="Disabled";
const PROGMEM char* STR_ENABLED ="Enabled";
const PROGMEM char* STR_NOT_FOUND ="Not Found";

const PROGMEM char* STR_MODE_NORMAL_SPLITTED ="Normal (splitted)";
const PROGMEM char* STR_MODE_NORMAL_UNSPLITTED ="Normal (unsplitted)";
const PROGMEM char* STR_MODE_MASTER ="Master";
const PROGMEM char* STR_MODE_SERVICE ="Service";

const PROGMEM char* STR_ARMED_ABC ="Armed (ABC)";
const PROGMEM char* STR_ARMED_A ="Armed (A)";
const PROGMEM char* STR_ARMED_B ="Armed (B)";
const PROGMEM char* STR_ARMED_NONE ="Disarmed";

const PROGMEM char* STR_MQTT_DISARMED ="disarmed";
const PROGMEM char* STR_MQTT_ARMED_HOME ="armed_home";
const PROGMEM char* STR_MQTT_ARMED_NIGHT ="armed_night";
const PROGMEM char* STR_MQTT_ARMED_AWAY ="armed_away";
const PROGMEM char* STR_MQTT_TRIGGERED ="triggered";
const PROGMEM char* STR_MQTT_ARMING ="arming";
const PROGMEM char* STR_MQTT_DISARMING ="disarming";
//const PROGMEM char* STR_MQTT_PENDING ="pending";
const PROGMEM char* STR_ARM_ABC_COMMAND ="*1";
const PROGMEM char* STR_ARM_A_COMMAND ="*2";
const PROGMEM char* STR_ARM_B_COMMAND ="*3";
const PROGMEM char* STR_COMMAND_PREFIX ="*";

//MQTT
const PROGMEM char* MQTT_PREFIX="tr1/";
const PROGMEM char* MQTT_ONLINE="/online";
const PROGMEM char* MQTT_STATE="state";
const PROGMEM char* MQTT_ALARM="/alarm/";
const PROGMEM char* MQTT_COMMAND="command";
const PROGMEM char* MQTT_PAYLOAD_ARM_AWAY="ARM_AWAY"; //"ABC";
const PROGMEM char* MQTT_PAYLOAD_ARM_HOME="ARM_HOME"; //"A";
const PROGMEM char* MQTT_PAYLOAD_ARM_NIGHT="ARM_NIGHT"; //"B";
const PROGMEM char* MQTT_PAYLOAD_DISARM="DISARM";
const PROGMEM char* MQTT_PAYLOAD_TRIGGER="TRIGGER";
const PROGMEM char* MQTT_PAYLOAD_ANNOUNCE="announce";


typedef enum {
  charAllowAll        =0,
  charAllowDigits     =1,
  charAllowKeypad     =2
} charRestriction;

//MESSAGES
const PROGMEM char* ERR_MESSAGE_TOO_BIG ="[E101]-Message too big.\n";
const PROGMEM char* ERR_TOO_MANY_MESSAGES ="[E102]-Too many messages.\n";
const PROGMEM char* ERR_NO_ROOM_FOR_CONNECTION ="[E103]-No room for additional connections.\n";
const PROGMEM char* ERR_UNKNOWN_COMMAND ="[E104]-Unknown command.\n";
const PROGMEM char* ERR_MALFORMED_COMMAND ="[E105]-Malformed command.\n";
const PROGMEM char* ERR_HTTP_UPDATE_FAILED ="[E106]-";
const PROGMEM char* ERR_LOAD_CONFIG ="[E107]-Unable to load config file.\n";
const PROGMEM char* ERR_SAVE_CONFIG ="[E108]-Unable to save config file.\n";
const PROGMEM char* ERR_VALUE_TOO_LARGE ="[E109]-Value too large.\n";
const PROGMEM char* ERR_WRONG_VALUE ="[E110]-Wrong value.\n";
const PROGMEM char* ERR_GSM_INIT ="[E111]-Unable to init GSM.\n";
const PROGMEM char* ERR_DEVICE_BUSY ="[E112]-Unable to execute command. The device is busy.\n";
const PROGMEM char* ERR_AUTH ="[E113]-Access Denied.\n";
const PROGMEM char* ERR_CHECK_UPDATE ="[E114]-Unable to update.\n";
const PROGMEM char* ERR_INCOMING_DATA ="[E115]-Error in serial incoming data.\n";
const PROGMEM char* ERR_SEQUENCE_REJECTED ="[E116]-Sequence rejected or expired.\n";
const PROGMEM char* ERR_SEQUENCE_CANCELLED ="[E117]-Sequence aborted.\n";
const PROGMEM char* WRN_HTTP_UPDATE_NO_UPDATES ="[W201]-No update available.\n";
const PROGMEM char* INFO_COMMAND_OK ="[I0]-Done.\n";
const PROGMEM char* INFO_HTTP_UPDATE_OK ="[I1]-Update done (reboot needed).\n";
const PROGMEM char* INFO_LOGIN_OK ="[I2]-Welcome!\n";

//LABELS
const PROGMEM char* LBL_MQTT_SERVER ="MQTT_SERVER";
const PROGMEM char* LBL_MQTT_PORT ="MQTT_PORT";
const PROGMEM char* LBL_MQTT_USER = "MQTT_USER";
const PROGMEM char* LBL_MQTT_PASSWORD ="MQTT_PASSWORD";
const PROGMEM char* LBL_OTA_PASSWORD ="OTA_PASSWORD";
const PROGMEM char* LBL_OTA_WEB_SERVER ="OTA_WEB_SERVER";
const PROGMEM char* LBL_OTA_WEB_PORT ="OTA_WEB_PORT";
const PROGMEM char* LBL_OTA_WEB_PAGE ="OTA_WEB_PAGE";
const PROGMEM char* LBL_TCP_PASSWORD ="TCP_PASSWORD";
const PROGMEM char* LBL_TCP_PORT ="TCP_PORT";
const PROGMEM char* LBL_WIFI_PASSWORD ="WIFI_PASSWORD";
const PROGMEM char* LBL_WIFI_SSID ="WIFI_SSID";
const PROGMEM char* LBL_GSM_ENABLE ="GSM_ENABLE";
const PROGMEM char* LBL_ACCESS_CODE ="ACCESS_CODE";
