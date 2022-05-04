//#define LOCAL_SERIAL


#define SECURE_MQTT

#define LED_PIN D4
#define REDE_PIN D7
#define TRIGGER_PIN D5

const PROGMEM char* VERSION = "Jabby_0.2.9";
const PROGMEM char* NAME ="Jabby";

//file system
const PROGMEM char* DATA_FILENAME = "/data.json";
const PROGMEM char* HELP_FILENAME = "/help.txt";

#define MAX_TCP_CONNECTIONS   3
#define MAX_TCP_MESSAGES      5
#define MAX_TCP_MESSAGE_LEN   64
#define MAX_TCP_AUTH_ATTEMPTS 3

//default values
#define SET_WO_ACCESS_CODE    1

//SERIAL
#define MAX_SERIAL_PK_LEN     24
#define MAX_SER_MESSAGES      3

//byte [1]
#define SERIAL_PK_BEGIN_MASK              0x80
#define SERIAL_PK_END                     0xFF
#define SERIAL_PK_ARMED_MASK              0x03
#define SERIAL_PK_ARMED_A                 0x01
#define SERIAL_PK_ARMED_B                 0x02
#define SERIAL_PK_ARMED_ABC               0x03
#define SERIAL_PK_TRIGGERED               0x04
#define SERIAL_PK_ACTIVATED               0x08
#define SERIAL_PK_DELAYED                 0x10
#define SERIAL_PK_MODE_MASK               0x60
#define SERIAL_PK_MODE_SERVICE            0
#define SERIAL_PK_MODE_MASTER             0x20
#define SERIAL_PK_MODE_NORMAL_UNSPLITTED  0x40
#define SERIAL_PK_MODE_NORMAL_SPLITTED    0x60

// byte [4]
#define SERIAL_PK_BATTERY                 1
#define SERIAL_PK_C                       2
#define SERIAL_PK_B                       4
#define SERIAL_PK_A                       8
#define SERIAL_PK_WARNING                 0x30

//Serial Outgoing traffic
#define MAX_SERIAL_MSG_LEN 16   //max message len
#define MAX_SERIAL_SEQ_LEN 16   //max sequence len
#define SERIAL_SEQ_EOF 0xFF

//const PROGMEM uint8_t SER_MSG_TAMPER_KEYPAD[] = {0xB2, SERIAL_PK_END};

//CRC
#define CRC_POLY 0xA3

//BUS
#define BUS_FREE                    0
#define BUS_WAITING_ECHO            1
#define BUS_WAITING_ACK             2
#define BUS_WAITING_EXTENDED_ACK    4
#define BUS_WAITING_PROMPT          8
#define BUS_BUSY                   16
#define BUS_NOT_ECHO             0xFE
#define BUS_NOT_EXTENDED_ACK     0xFB


//SERIAL COMMANDS
typedef enum {
  serMessageStream       =0,
  serMessageCommand      =1
} serMessageType;

//timers
#define LED_INTERVAL_TIMER                5
#define SERIAL_INTERVAL_TIMER             10
#define SERIAL_MAX_INTERVAL_TIMER         500
#define CP_MAX_INTERVAL_TIMER             1000
#define BUS_MAX_INTERVAL_TIMER            10000
#define TRIGGER_INTERVAL_TIMER            1000
#define MQTT_RECONNECT_TIMER              10000
#define MQTT_LOOP_TIMER                   1000
#define MQTT_PUBLISH_TIMER                60000


//TRIGGER
#define TRIGGER_NONE    55
#define TRIGGER_DEVICE  150
#define TRIGGER_TAMPER  0

//ARM COMMANDS
typedef enum {
  armNone        =0,
  armABC         =1,
  armA           =2,
  armB           =3
} armCommand;

//SERIAL MESSAGES
const PROGMEM char* SER_MSG_01 ="Istant Alarm";
const PROGMEM char* SER_MSG_02 ="Delayed Alarm";
const PROGMEM char* SER_MSG_03 ="Fire Alarm";
const PROGMEM char* SER_MSG_04 ="Panic Alarm";
const PROGMEM char* SER_MSG_05 ="Tamper Alarm";
const PROGMEM char* SER_MSG_06 ="Invalid code-entries exceeded";
const PROGMEM char* SER_MSG_07 ="Device fault";
const PROGMEM char* SER_MSG_08 ="Complete setting";
const PROGMEM char* SER_MSG_09 ="Complete unsetting";
const PROGMEM char* SER_MSG_0C ="Codeless setting";
const PROGMEM char* SER_MSG_0D ="Partial setting A";
const PROGMEM char* SER_MSG_0E ="Internal comm. failure";
const PROGMEM char* SER_MSG_0F ="Mains dropout";
const PROGMEM char* SER_MSG_10 ="Mains recovery";
const PROGMEM char* SER_MSG_11 ="Battery Fault";
const PROGMEM char* SER_MSG_14 ="Accumulator Fault CP";
const PROGMEM char* SER_MSG_15 ="Accumulator CP Ok";
const PROGMEM char* SER_MSG_17 ="Alarm 24h";
const PROGMEM char* SER_MSG_18 ="Radio jamming";
const PROGMEM char* SER_MSG_1A ="Set A (split)";
const PROGMEM char* SER_MSG_1B ="Set B (split)";
const PROGMEM char* SER_MSG_1C ="Unset A (split)";
const PROGMEM char* SER_MSG_1D ="Unset B (split)";
const PROGMEM char* SER_MSG_1E ="Set C (split)";
const PROGMEM char* SER_MSG_1F ="Unset C (split)";
const PROGMEM char* SER_MSG_21 ="Partial setting AB";
const PROGMEM char* SER_MSG_33 ="Serivce Mode";
const PROGMEM char* SER_MSG_34 ="Maintenance Mode";
const PROGMEM char* SER_MSG_36 ="Invalid code-exc";
const PROGMEM char* SER_MSG_38 ="Exit Delay";
const PROGMEM char* SER_MSG_39 ="Entrance Delay";
const PROGMEM char* SER_MSG_3A ="Trigger Detect";
const PROGMEM char* SER_MSG_3F ="Active Detectors";
const PROGMEM char* SER_MSG_40 ="Power ON CP";
const PROGMEM char* SER_MSG_43 ="End of Alarm";
const PROGMEM char* SER_MSG_4E ="Alarm cancelled by user";
const PROGMEM char* SER_MSG_4F ="CP reset";
const PROGMEM char* SER_MSG_50 ="All tampers calm";
const PROGMEM char* SER_MSG_51 ="All faults removed";
const PROGMEM char* SER_MSG_52 ="System power ok";
const PROGMEM char* SER_MSG_55 ="Master code reset";
const PROGMEM char* SER_MSG_56 ="Master code changed";
const PROGMEM char* SER_MSG_59 ="Mains dropout exceeding 30 min";
const PROGMEM char* SER_MSG_5A ="Unconfirmed alarm";
const PROGMEM char* SER_MSG_5B ="Service request";
const PROGMEM char* SER_MSG_5C ="PgX on";
const PROGMEM char* SER_MSG_5D ="PgX off";
const PROGMEM char* SER_MSG_5E ="PgY on";
const PROGMEM char* SER_MSG_5F ="PgY off";
const PROGMEM char* SER_MSG_60 ="Eng. reset req.";
const PROGMEM char* SER_MSG_61 ="Eng. reset done";

//STRINGS
const PROGMEM char* STR_0 ="\0";
const PROGMEM char* STR_N ="\n";
//const PROGMEM char* STR_R ="\r";
const PROGMEM char* STR_ON ="on";
const PROGMEM char* STR_OFF ="off";
const PROGMEM char* STR_YES ="Yes";
const PROGMEM char* STR_NO ="No";
const PROGMEM char* STR_OK ="Ok";
const PROGMEM char* STR_SUBSCRIPTION ="Subscription: ";
const PROGMEM char* STR_DEVICE =" device: ";
const PROGMEM char* STR_MODE_ENTERED =" mode entered";
const PROGMEM char* STR_MODE_EXIT =" mode exit";
const PROGMEM char* STR_ARMED_AFTER_POWER_UP ="Alarm after powering up";

const PROGMEM char* STR_UNKNOWN ="Unknown";
const PROGMEM char* STR_RESET ="Reset";
const PROGMEM char* STR_ACKNOWLEDGMENT ="Acknowledgment";

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
const PROGMEM char* MQTT_PAYLOAD_TRIGGER="TRIGGER";     //device
const PROGMEM char* MQTT_PAYLOAD_TRIGGER_2="TRIGGER_2"; //tampter
const PROGMEM char* MQTT_PAYLOAD_TRIGGER_3="TRIGGER_3"; //Key pad tampter simulation
const PROGMEM char* MQTT_PAYLOAD_ANNOUNCE="announce";

const PROGMEM char* MQTT_MODE="/mode";
const PROGMEM char* MQTT_ARMED="/armed";
const PROGMEM char* MQTT_TRIGGERED="/triggered";
const PROGMEM char* MQTT_ACTIVATED="/activated";
const PROGMEM char* MQTT_DELAYED="/delayed";

const PROGMEM char* MQTT_DEVICE="/device";
const PROGMEM char* MQTT_INFO_WARNING="/warning";
const PROGMEM char* MQTT_INFO_BATTERY="/battery";
const PROGMEM char* MQTT_INFO_A="/a";
const PROGMEM char* MQTT_INFO_B="/b";
const PROGMEM char* MQTT_INFO_C="/c";

const PROGMEM char* MQTT_MESSAGE="/message";

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
//const PROGMEM char* ERR_GSM_INIT ="[E111]-Unable to init GSM.\n";
//const PROGMEM char* ERR_DEVICE_BUSY ="[E112]-Unable to execute command. The device is busy.\n";
const PROGMEM char* ERR_AUTH ="[E113]-Access Denied.\n";
const PROGMEM char* ERR_CHECK_UPDATE ="[E114]-Unable to update.\n";
const PROGMEM char* ERR_INCOMING_DATA ="[E115]-Error in serial incoming data.\n";
const PROGMEM char* ERR_SEQUENCE_REJECTED ="[E116]-Sequence rejected or expired.\n";
const PROGMEM char* ERR_SEQUENCE_CANCELLED ="[E117]-Sequence aborted.\n";
const PROGMEM char* ERR_UNKNOWN_DATA ="[E118]-Unknown data.\n";
const PROGMEM char* ERR_TRIGGER ="[E118]-Unable to trigger alarm.\n";
const PROGMEM char* ERR_SERIAL_GSM ="[E119]-Unable to open GSM serial port.\n";
//const PROGMEM char* ERR_GSM_INCOMING_DATA ="[E120]-Error in GSM incoming data.\n";
//const PROGMEM char* ERR_GSM_TIMEOUT ="[E121]-Timeout in GSM communication.\n";
//const PROGMEM char* ERR_GSM_REJECTED ="[E122]-GSM command rejected.\n";
//const PROGMEM char* ERR_GSM_UNKNOWN ="[E123]-GSM unknown response.\n";
const PROGMEM char* WRN_HTTP_UPDATE_NO_UPDATES ="[W201]-No update available.\n";
const PROGMEM char* INFO_COMMAND_OK ="[I0]-Done.\n";
const PROGMEM char* INFO_HTTP_UPDATE_OK ="[I1]-Update done (reboot needed).\n";
const PROGMEM char* INFO_LOGIN_OK ="[I2]-Welcome!\n";
const PROGMEM char* INFO_OPEN_GSM_CONNECTION ="[I3]-Open GSM connection.\n";
const PROGMEM char* INFO_CLOSE_GSM_CONNECTION ="[I4]-Close GSM connection.\n";
//const PROGMEM char* INFO_GSM_CONNECTION_OK ="[I5]-GSM connection OK.\n";

//LABELS
const PROGMEM char* LBL_MQTT_SERVER ="MS";
const PROGMEM char* LBL_MQTT_PORT ="MP";
const PROGMEM char* LBL_MQTT_USER = "MU";
const PROGMEM char* LBL_MQTT_PASSWORD ="MPW";
const PROGMEM char* LBL_OTA_PASSWORD ="OPW";
const PROGMEM char* LBL_OTA_WEB_SERVER ="OWS";
const PROGMEM char* LBL_OTA_WEB_PORT ="OWP";
const PROGMEM char* LBL_OTA_WEB_PAGE ="OWPG";
const PROGMEM char* LBL_TCP_PASSWORD ="TPW";
const PROGMEM char* LBL_TCP_PORT ="TP";
const PROGMEM char* LBL_WIFI_PASSWORD ="WPW";
const PROGMEM char* LBL_WIFI_SSID ="WS";
const PROGMEM char* LBL_ACCESS_CODE ="AC";
const PROGMEM char* LBL_SET_WO_ACCESS_CODE ="SWOAC";
