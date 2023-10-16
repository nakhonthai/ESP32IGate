/*
 Name:		ESP32 APRS Internet Gateway
 Created:	1-Nov-2021 14:27:23
 Author:	HS5TQA/Atten
 Support IS: host:aprs.dprns.com port:14580
 Support IS monitor: http://aprs.dprns.com:14501
 Support in LINE Group APRS Only
*/

#include <Arduino.h>
#include "main.h"
#include <LibAPRSesp.h>
#include <limits.h>
#include <KISS.h>
#include "webservice.h"
#include <WiFiUdp.h>
#include "ESP32Ping.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include "cppQueue.h"
#include "BluetoothSerial.h"
#include "digirepeater.h"
#include "igate.h"
#include "wireguardif.h"
#include "wireguard.h"

#include <TinyGPS++.h>
#include <pbuf.h>
#include <parse_aprs.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <Fonts/Seven_Segment24pt7b.h>

#include "wireguard_vpn.h"
#include <WiFiUdp.h>

#include <WiFiClientSecure.h>

#include "AFSK.h"

// #include <PPPoS.h>

#define EEPROM_SIZE 1024

#ifdef SDCARD
#include <SPI.h> //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include "FS.h"
#include "SPIFFS.h"
#define SDCARD_CS 13
#define SDCARD_CLK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 2
#endif

#ifdef OLED
#include <Wire.h>
#include "Adafruit_SSD1306.h"
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET 4        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// For a connection via I2C using the Arduino Wire include:
// #include <Wire.h>		 // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
// OR #include "SH1106Wire.h"   // legacy: #include "SH1106.h"

// Initialize the OLED display using Arduino Wire:
// SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_64); // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
// SSD1306Wire display(0x3c, D3, D5);  // ADDRESS, SDA, SCL  -  If not, they can be specified manually.
// SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);  // ADDRESS, SDA, SCL, OLEDDISPLAY_GEOMETRY  -  Extra param required for 128x32 displays.
// SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL

#endif

struct pbuf_t aprs;
ParseAPRS aprsParse;

TinyGPSPlus gps;

#ifdef SA818
#define VBAT_PIN 35
#define WIRE 4
#define POWER_PIN 12
#define PULLDOWN_PIN 27
#define SQL_PIN 33
HardwareSerial SerialRF(1);
#endif

#define MODEM_PWRKEY 5
#define MODEM_TX 17
#define MODEM_RX 16

#define LED_TX 4
#define LED_RX 2

#define PPP_APN "internet"
#define PPP_USER ""
#define PPP_PASS ""

const char *str_status[] = {
    "IDLE_STATUS",
    "NO_SSID_AVAIL",
    "SCAN_COMPLETED",
    "CONNECTED",
    "CONNECT_FAILED",
    "CONNECTION_LOST",
    "DISCONNECTED"};

// PPPoS ppp;

time_t systemUptime = 0;
time_t wifiUptime = 0;

boolean KISS = false;
bool aprsUpdate = false;

boolean gotPacket = false;
AX25Msg incomingPacket;

bool lastPkg = false;
bool afskSync = false;
String lastPkgRaw = "";
float dBV = 0;
int mVrms = 0;

cppQueue PacketBuffer(sizeof(AX25Msg), 5, IMPLEMENTATION); // Instantiate queue
cppQueue dispBuffer(300, 5, IMPLEMENTATION);

statusType status;
RTC_DATA_ATTR igateTLMType igateTLM;
RTC_DATA_ATTR txQueueType txQueue[PKGTXSIZE];

extern RTC_DATA_ATTR uint8_t digiCount;

Configuration config;

pkgListType pkgList[PKGLISTSIZE];

TelemetryType Telemetry[TLMLISTSIZE];

TaskHandle_t taskNetworkHandle;
TaskHandle_t taskAPRSHandle;

HardwareSerial SerialTNC(2);

WeatherData weather;
// BluetoothSerial SerialBT;

// Set your Static IP address for wifi AP
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 254);
IPAddress subnet(255, 255, 255, 0);

IPAddress vpn_IP(192, 168, 44, 195);

int pkgTNC_count = 0;

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length();

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
} // END

boolean isValidNumber(String str)
{
    for (byte i = 0; i < str.length(); i++)
    {
        if (isDigit(str.charAt(i)))
            return true;
    }
    return false;
}

uint8_t checkSum(uint8_t *ptr, size_t count)
{
    uint8_t lrc, tmp;
    uint16_t i;
    lrc = 0;
    for (i = 0; i < count; i++)
    {
        tmp = *ptr++;
        lrc = lrc ^ tmp;
    }
    return lrc;
}

void saveEEPROM()
{
    uint8_t chkSum = 0;
    byte *ptr;
    ptr = (byte *)&config;
    EEPROM.writeBytes(1, ptr, sizeof(Configuration));
    chkSum = checkSum(ptr, sizeof(Configuration));
    EEPROM.write(0, chkSum);
    EEPROM.commit();
#ifdef DEBUG
    Serial.print("Save EEPROM ChkSUM=");
    Serial.println(chkSum, HEX);
#endif
}

void defaultConfig()
{
    Serial.println("Default configure mode!");
    sprintf(config.aprs_mycall, "MYCALL");
    config.aprs_ssid = 0;
    sprintf(config.aprs_host, "aprs.dprns.com");
    config.aprs_port = 14580;
    sprintf(config.aprs_passcode, "00000");
    sprintf(config.aprs_moniCall, "%s-%d", config.aprs_mycall, config.aprs_ssid);
    sprintf(config.aprs_filter, "g/HS*/E2*");
    sprintf(config.wifi_ssid, "APRSTH");
    sprintf(config.wifi_pass, "aprsthnetwork");
    sprintf(config.wifi_ap_ssid, "ESP32IGate");
    sprintf(config.wifi_ap_pass, "aprsthnetwork");
    sprintf(config.mqtt_host, "aprs.dprns.com");
    config.wifi_client = true;
    config.synctime = true;
    config.mqtt_port = 1883;
    config.aprs_beacon = 600;
    config.gps_lat = 13.7555;
    config.gps_lon = 100.4930;
    config.gps_alt = 3;
    config.tnc = true;
    config.inet2rf = true;
    config.rf2inet = true;
    config.aprs = true;
    config.wifi = true;
    config.wifi_mode = WIFI_AP_STA_FIX;
        config.wifi_ch = 1;
    config.tnc_digi = true;
    config.tnc_telemetry = true;
    config.tnc_btext[0] = 0;
    config.tnc_beacon = 0;
    config.aprs_table = '/';
    config.aprs_symbol = '&';
    config.digi_delay = 2000;
    config.tx_timeslot = 5000;
    sprintf(config.aprs_path, "WIDE1-1");
    sprintf(config.aprs_comment, "ESP32 Internet Gateway");
    sprintf(config.tnc_comment, "ESP32 Build in TNC");
    sprintf(config.aprs_filter, "g/HS*/E2*");
    sprintf(config.tnc_path, "WIDE1-1");
    memset(config.aprs_object,0,sizeof(config.aprs_object));
    config.wifi_power = 44;
    config.input_hpf = true;
#ifdef SA818
    config.freq_rx = 144.3900;
    config.freq_tx = 144.3900;
    config.offset_rx = 0;
    config.offset_tx = 0;
    config.tone_rx = 0;
    config.tone_tx = 0;
    config.band = 0;
    config.sql_level = 1;
    config.rf_power = LOW;
    config.volume = 4;
    config.input_hpf = false;
#endif
    input_HPF = config.input_hpf;
    config.vpn = true;
    config.modem = false;
    config.wg_port = 51820;
    sprintf(config.wg_peer_address, "203.150.19.23");
    sprintf(config.wg_local_address, "44.63.31.202");
    sprintf(config.wg_netmask_address, "255.255.255.255");
    sprintf(config.wg_gw_address, "44.63.31.193");
    sprintf(config.wg_public_key, "ZEFr+/B/T5+k0DhVG/GOTvAOjeOiuFKmwtu/cy23xVs=");
    sprintf(config.wg_private_key, "gH2YqDa+St6x5eFhomVQDwtV1F0YMQd3HtOElPkZgVY=");
    config.timeZone = 7;
    config.oled_enable = true;
    config.oled_timeout = 60;
    config.filterMessage = true;
    config.filterStatus = false;
    config.filterTelemetry = false;
    config.filterWeather = true;
    config.filterTracker = true;
    config.filterMove = true;
    config.filterPosition = true;
    config.dispINET = true;
    config.dispTNC = true;
    config.dispDelay = 3; // Sec
    saveEEPROM();
}

unsigned long NTP_Timeout;
unsigned long pingTimeout;

const char *lastTitle = "LAST HERT";

int tlmList_Find(char *call)
{
    int i;
    for (i = 0; i < TLMLISTSIZE; i++)
    {
        if (strstr(Telemetry[i].callsign, call) != NULL)
            return i;
    }
    return -1;
}

int tlmListOld()
{
    int i, ret = 0;
    time_t minimum = Telemetry[0].time;
    for (i = 1; i < TLMLISTSIZE; i++)
    {
        if (Telemetry[i].time < minimum)
        {
            minimum = Telemetry[i].time;
            ret = i;
        }
        if (Telemetry[i].time > now())
            Telemetry[i].time = 0;
    }
    return ret;
}

char pkgList_Find(char *call)
{
    char i;
    for (i = 0; i < PKGLISTSIZE; i++)
    {
        if (strstr(pkgList[(int)i].calsign, call) != NULL)
            return i;
    }
    return -1;
}

char pkgListOld()
{
    char i, ret = 0;
    time_t minimum = pkgList[0].time;
    for (i = 1; i < PKGLISTSIZE; i++)
    {
        if (pkgList[(int)i].time < minimum)
        {
            minimum = pkgList[(int)i].time;
            ret = i;
        }
    }
    return ret;
}

void sort(pkgListType a[], int size)
{
    pkgListType t;
    char *ptr1;
    char *ptr2;
    char *ptr3;
    ptr1 = (char *)&t;
    for (int i = 0; i < (size - 1); i++)
    {
        for (int o = 0; o < (size - (i + 1)); o++)
        {
            if (a[o].time < a[o + 1].time)
            {
                ptr2 = (char *)&a[o];
                ptr3 = (char *)&a[o + 1];
                memcpy(ptr1, ptr2, sizeof(pkgListType));
                memcpy(ptr2, ptr3, sizeof(pkgListType));
                memcpy(ptr3, ptr1, sizeof(pkgListType));
            }
        }
    }
}

void sortPkgDesc(pkgListType a[], int size)
{
    pkgListType t;
    char *ptr1;
    char *ptr2;
    char *ptr3;
    ptr1 = (char *)&t;
    for (int i = 0; i < (size - 1); i++)
    {
        for (int o = 0; o < (size - (i + 1)); o++)
        {
            if (a[o].pkg < a[o + 1].pkg)
            {
                ptr2 = (char *)&a[o];
                ptr3 = (char *)&a[o + 1];
                memcpy(ptr1, ptr2, sizeof(pkgListType));
                memcpy(ptr2, ptr3, sizeof(pkgListType));
                memcpy(ptr3, ptr1, sizeof(pkgListType));
            }
        }
    }
}

uint8_t pkgType(const char *raw)
{
    uint8_t type = 0;
    char packettype = 0;
    const char *info_start, *body;
    int paclen = strlen(raw);
    // info_start = (char*)strchr(raw, ':');
    // if (info_start == NULL) return 0;
    // info_start=0;
    packettype = (char)raw[0];
    body = &raw[1];

    switch (packettype)
    {
    case '=':
    case '/':
    case '@':
        if (strchr(body, 'r') != NULL)
        {
            if (strchr(body, 'g') != NULL)
            {
                if (strchr(body, 't') != NULL)
                {
                    if (strchr(body, 'P') != NULL)
                    {
                        type = PKG_WX;
                    }
                }
            }
        }
        break;
    case ':':
        type = PKG_MESSAGE;
        if (body[9] == ':' &&
            (memcmp(body + 9, ":PARM.", 6) == 0 ||
             memcmp(body + 9, ":UNIT.", 6) == 0 ||
             memcmp(body + 9, ":EQNS.", 6) == 0 ||
             memcmp(body + 9, ":BITS.", 6) == 0))
        {
            type = PKG_TELEMETRY;
        }
        break;
    case '>':
        type = PKG_STATUS;
        break;
    case '?':
        type = PKG_QUERY;
        break;
    case ';':
        type = PKG_OBJECT;
        break;
    case ')':
        type = PKG_ITEM;
        break;
    case 'T':
        type = PKG_TELEMETRY;
        break;
    case '#': /* Peet Bros U-II Weather Station */
    case '*': /* Peet Bros U-I  Weather Station */
    case '_': /* Weather report without position */
        type = PKG_WX;
        break;
    default:
        type = 0;
        break;
    }
    return type;
}

void pkgListUpdate(char *call, uint8_t type)
{
    char i = pkgList_Find(call);
    time_t now;
    time(&now);
    if (i != 255)
    { // Found call in old pkg
        pkgList[(uint)i].time = now;
        pkgList[(uint)i].pkg++;
        pkgList[(uint)i].type = type;
        // Serial.print("Update: ");
    }
    else
    {
        i = pkgListOld();
        pkgList[(uint)i].time = now;
        pkgList[(uint)i].pkg = 1;
        pkgList[(uint)i].type = type;
        strcpy(pkgList[(uint)i].calsign, call);
        // strcpy(pkgList[(uint)i].ssid, &ssid[0]);
        pkgList[(uint)i].calsign[10] = 0;
        // Serial.print("NEW: ");
    }
}

bool pkgTxUpdate(const char *info, int delay)
{
    char *ecs = strstr(info, ">");
    if (ecs == NULL)
        return false;
    // Replace
    for (int i = 0; i < PKGTXSIZE; i++)
    {
        if (txQueue[i].Active)
        {
            if (!(strncmp(&txQueue[i].Info[0], info, info - ecs)))
            {
                strcpy(&txQueue[i].Info[0], info);
                txQueue[i].Delay = delay;
                txQueue[i].timeStamp = millis();
                return true;
            }
        }
    }

    // Add
    for (int i = 0; i < PKGTXSIZE; i++)
    {
        if (txQueue[i].Active == false)
        {
            strcpy(&txQueue[i].Info[0], info);
            txQueue[i].Delay = delay;
            txQueue[i].Active = true;
            txQueue[i].timeStamp = millis();
            break;
        }
    }
    return true;
}

bool pkgTxSend()
{
    for (int i = 0; i < PKGTXSIZE; i++)
    {
        if (txQueue[i].Active)
        {
            int decTime = millis() - txQueue[i].timeStamp;
            if (decTime > txQueue[i].Delay)
            {
#ifdef SA818
                digitalWrite(POWER_PIN, config.rf_power); // RF Power LOW
#endif
                APRS_setPreamble(350L);
                APRS_sendTNC2Pkt(String(txQueue[i].Info)); // Send packet to RF
                txQueue[i].Active = false;
                igateTLM.TX++;
#ifdef DEBUG
                printTime();
                Serial.println("TX->RF: " + String(txQueue[i].Info));
#endif
                return true;
            }
        }
    }
    return false;
}

uint8_t *packetData;
// ฟังชั่นถูกเรียกมาจาก ax25_decode
void aprs_msg_callback(struct AX25Msg *msg)
{
    AX25Msg pkg;

    memcpy(&pkg, msg, sizeof(AX25Msg));
    PacketBuffer.push(&pkg); // ใส่แพ็จเก็จจาก TNC ลงคิวบัพเฟอร์
}

void printTime()
{
    struct tm tmstruct;
    getLocalTime(&tmstruct, 5000);
    Serial.print("[");
    Serial.print(tmstruct.tm_hour);
    Serial.print(":");
    Serial.print(tmstruct.tm_min);
    Serial.print(":");
    Serial.print(tmstruct.tm_sec);
    Serial.print("]");
}

uint8_t gwRaw[PKGLISTSIZE][66];
uint8_t gwRawSize[PKGLISTSIZE];
int gwRaw_count = 0, gwRaw_idx_rd = 0, gwRaw_idx_rw = 0;

void pushGwRaw(uint8_t *raw, uint8_t size)
{
    if (gwRaw_count > PKGLISTSIZE)
        return;
    if (++gwRaw_idx_rw >= PKGLISTSIZE)
        gwRaw_idx_rw = 0;
    if (size > 65)
        size = 65;
    memcpy(&gwRaw[gwRaw_idx_rw][0], raw, size);
    gwRawSize[gwRaw_idx_rw] = size;
    gwRaw_count++;
}

uint8_t popGwRaw(uint8_t *raw)
{
    uint8_t size = 0;
    if (gwRaw_count <= 0)
        return 0;
    if (++gwRaw_idx_rd >= PKGLISTSIZE)
        gwRaw_idx_rd = 0;
    size = gwRawSize[gwRaw_idx_rd];
    memcpy(raw, &gwRaw[gwRaw_idx_rd][0], size);
    if (gwRaw_count > 0)
        gwRaw_count--;
    return size;
}

#ifdef SA818
unsigned long SA818_Timeout = 0;
void SA818_INIT(bool boot)
{
#ifdef SR_FRS
    Serial.println("Radio Module SR_FRS Init");
#else
    Serial.println("Radio Module SA818/SA868 Init");
#endif
    if (boot)
    {
        SerialRF.begin(9600, SERIAL_8N1, 14, 13);
        pinMode(POWER_PIN, OUTPUT);
        pinMode(PULLDOWN_PIN, OUTPUT);
        pinMode(SQL_PIN, INPUT_PULLUP);

        digitalWrite(POWER_PIN, LOW);
        digitalWrite(PULLDOWN_PIN, LOW);
        delay(500);
        digitalWrite(PULLDOWN_PIN, HIGH);
        delay(1500);
        SerialRF.println();
        delay(500);
    }
    SerialRF.println();
    delay(500);
    char str[100];
    if (config.sql_level > 8)
        config.sql_level = 8;
#ifdef SR_FRS
    sprintf(str, "AT+DMOSETGROUP=%01d,%0.4f,%0.4f,%d,%01d,%d,0", config.band, config.freq_tx + ((float)config.offset_tx / 1000000), config.freq_rx + ((float)config.offset_rx / 1000000), config.tone_rx, config.sql_level, config.tone_tx);
    SerialRF.println(str);
    delay(500);
    // Module auto power save setting
    SerialRF.println("AT+DMOAUTOPOWCONTR=1");
    delay(500);
    SerialRF.println("AT+DMOSETVOX=0");
    delay(500);
    SerialRF.println("AT+DMOSETMIC=1,0,0");
#else
    sprintf(str, "AT+DMOSETGROUP=%01d,%0.4f,%0.4f,%04d,%01d,%04d", config.band, config.freq_tx + ((float)config.offset_tx / 1000000), config.freq_rx + ((float)config.offset_rx / 1000000), config.tone_tx, config.sql_level, config.tone_rx);
    SerialRF.println(str);
    delay(500);
    SerialRF.println("AT+SETTAIL=0");
    delay(500);
    SerialRF.println("AT+SETFILTER=1,1,1");
#endif
    // SerialRF.println(str);
    delay(500);
    if (config.volume > 8)
        config.volume = 8;
    SerialRF.printf("AT+DMOSETVOLUME=%d\r\n", config.volume);
}

void SA818_SLEEP()
{
    digitalWrite(POWER_PIN, LOW);
    digitalWrite(PULLDOWN_PIN, LOW);
    // SerialGPS.print("$PMTK161,0*28\r\n");
    // AFSK_TimerEnable(false);
}

void SA818_CHECK()
{
    while (SerialRF.available() > 0)
        SerialRF.read();
    SerialRF.println("AT+DMOCONNECT");
    delay(100);
    if (SerialRF.available() > 0)
    {
        String ret = SerialRF.readString();
        if (ret.indexOf("DMOCONNECT") > 0)
        {
            SA818_Timeout = millis();
#ifdef DEBUG
            // Serial.println(SerialRF.readString());
            Serial.println("Radio SA818/SR_FRS Activated");
#endif
        }
    }
    else
    {
        Serial.println("Radio SA818/SR_FRS deActive");
        digitalWrite(POWER_PIN, LOW);
        digitalWrite(PULLDOWN_PIN, LOW);
        delay(500);
        SA818_INIT(true);
    }
    // SerialGPS.print("$PMTK161,0*28\r\n");
    // AFSK_TimerEnable(false);
}
#endif
// #ifdef SA818
// unsigned long SA818_Timeout = 0;
// void SA818_INIT(uint8_t HL)
// {

//     pinMode(0, INPUT);
//     pinMode(POWER_PIN, OUTPUT);
//     pinMode(PULLDOWN_PIN, OUTPUT);
//     pinMode(SQL_PIN, INPUT_PULLUP);

//     SerialRF.begin(9600, SERIAL_8N1, 14, 13);

//     digitalWrite(PULLDOWN_PIN, HIGH);
//     digitalWrite(POWER_PIN, LOW);
//     delay(500);
//     // AT+DMOSETGROUP=1,144.3900,144.3900,0,1,0,0
//     SerialRF.println("AT+DMOSETGROUP=0,144.3900,144.3900,0,1,0,0");
//     delay(10);
//     SerialRF.println("AT+DMOAUTOPOWCONTR=1");
//     delay(10);
//     SerialRF.println("AT+DMOSETVOLUME=9");
//     delay(10);
//     SerialRF.println("AT+DMOSETVOX=0");
//     delay(10);
//     SerialRF.println("AT+DMOSETMIC=8,0,0");
//     delay(100);
//     // AFSK_TimerEnable(true);
//     digitalWrite(POWER_PIN, HL);
// }

// void SA818_SLEEP()
// {
//     digitalWrite(POWER_PIN, LOW);
//     digitalWrite(PULLDOWN_PIN, LOW);
//     // SerialGPS.print("$PMTK161,0*28\r\n");
//     // AFSK_TimerEnable(false);
// }

// void SA818_CHECK()
// {
//     while (SerialRF.available() > 0)
//         SerialRF.read();
//     SerialRF.println("AT+DMOCONNECT");
//     delay(100);
//     if (SerialRF.available() > 0)
//     {
//         String ret = SerialRF.readString();
//         if (ret.indexOf("DMOCONNECT") > 0)
//         {
//             SA818_Timeout = millis();
// #ifdef DEBUG
//             // Serial.println(SerialRF.readString());
//             Serial.println("SA818 Activated");
// #endif
//         }
//     }
//     else
//     {
//         Serial.println("SA818 deActive");
//         digitalWrite(POWER_PIN, LOW);
//         digitalWrite(PULLDOWN_PIN, LOW);
//         delay(500);
//         SA818_INIT(LOW);
//     }
//     // SerialGPS.print("$PMTK161,0*28\r\n");
//     // AFSK_TimerEnable(false);
// }
// #endif

WiFiClient aprsClient;

boolean APRSConnect()
{
    // Serial.println("Connect TCP Server");
    String login = "";
    int cnt = 0;
    uint8_t con = aprsClient.connected();
    // Serial.println(con);
    if (con <= 0)
    {
        if (!aprsClient.connect(config.aprs_host, config.aprs_port)) // เชื่อมต่อกับเซิร์ฟเวอร์ TCP
        {
            // Serial.print(".");
            delay(100);
            cnt++;
            if (cnt > 50) // วนร้องขอการเชื่อมต่อ 50 ครั้ง ถ้าไม่ได้ให้รีเทิร์นฟังค์ชั่นเป็น False
                return false;
        }
        // ขอเชื่อมต่อกับ aprsc
        if(strlen(config.aprs_object)>=3){
            uint16_t passcode=aprsParse.passCode(config.aprs_object);
            login = "user " + String(config.aprs_object) + " pass " + String(passcode,DEC) + " vers ESP32IGate V" + String(VERSION) + " filter " + String(config.aprs_filter);
        }else{
        if (config.aprs_ssid == 0)
            login = "user " + String(config.aprs_mycall) + " pass " + String(config.aprs_passcode) + " vers ESP32IGate V" + String(VERSION) + " filter " + String(config.aprs_filter);
        else
            login = "user " + String(config.aprs_mycall) + "-" + String(config.aprs_ssid) + " pass " + String(config.aprs_passcode) + " vers ESP32IGate V" + String(VERSION) + " filter " + String(config.aprs_filter);
        }
        aprsClient.println(login);
        // Serial.println(login);
        // Serial.println("Success");
        delay(500);
    }
    return true;
}

long oledSleepTimeout = 0;
bool showDisp = false;
uint8_t curTab = 0;

void setup()
{
    byte *ptr;
    pinMode(0, INPUT_PULLUP); // BOOT Button
    pinMode(LED_RX, OUTPUT);
    pinMode(LED_TX, OUTPUT);

    // Set up serial port
    Serial.begin(9600); // debug
    Serial.setRxBufferSize(256);
    SerialTNC.begin(9600, SERIAL_8N1, 16, 17);
    SerialTNC.setRxBufferSize(500);

    Serial.println();
    Serial.println("Start ESP32IGate V" + String(VERSION));
    Serial.println("Push BOOT after 3 sec for Factory Default config.");

#ifdef OLED
    Wire.begin();
    Wire.setClock(400000L);

    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false); // initialize with the I2C addr 0x3C (for the 128x64)
    // Initialising the UI will init the display too.
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(30, 5);
    display.print("ESP32 IGATE");
    display.setCursor(1, 17);
    display.print("Firmware Version " + String(VERSION));
    display.drawLine(10, 30, 110, 30, WHITE);
    display.setCursor(1, 40);
    display.print("Push B Factory reset");
    // topBar(-100);
    display.display();
#endif

    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println(F("failed to initialise EEPROM")); // delay(100000);
    }

#ifdef OLED
    delay(1000);
    digitalWrite(LED_TX, HIGH);
    display.setCursor(50, 50);
    display.print("3 Sec");
    display.display();
    delay(1000);
    digitalWrite(LED_RX, HIGH);
    display.setCursor(40, 50);
    display.print("        ");
    display.display();
    display.setCursor(50, 50);
    display.print("2 Sec");
    display.display();
    delay(1000);
    display.setCursor(40, 50);
    display.print("        ");
    display.display();
    display.setCursor(50, 50);
    display.print("1 Sec");
    display.display();
#else
    delay(1000);
    digitalWrite(LED_TX, HIGH);
    delay(1000);
    digitalWrite(LED_RX, HIGH);
    delay(1000);
#endif
    if (digitalRead(0) == LOW)
    {
        defaultConfig();
        Serial.println("Manual Default configure!");
#ifdef OLED
        display.clearDisplay();
        display.setCursor(10, 22);
        display.print("Factory Reset!");
        display.display();
#endif
        while (digitalRead(0) == LOW)
        {
            delay(500);
            digitalWrite(LED_TX, LOW);
            digitalWrite(LED_RX, LOW);
            delay(500);
            digitalWrite(LED_TX, HIGH);
            digitalWrite(LED_RX, HIGH);
        }
    }
    digitalWrite(LED_TX, LOW);
    digitalWrite(LED_RX, LOW);

    // ตรวจสอบคอนฟิกซ์ผิดพลาด
    ptr = (byte *)&config;
    EEPROM.readBytes(1, ptr, sizeof(Configuration));
    uint8_t chkSum = checkSum(ptr, sizeof(Configuration));
    Serial.printf("EEPROM Check %0Xh=%0Xh(%dByte)\n", EEPROM.read(0), chkSum, sizeof(Configuration));
    if (EEPROM.read(0) != chkSum)
    {
        Serial.println("Config EEPROM Error!");
        defaultConfig();
    }
    input_HPF = config.input_hpf;

#ifdef SA818
    SA818_INIT(true);
#endif

#ifdef OLED
    if (config.oled_enable == true)
    {
        display.clearDisplay();
        display.setTextSize(1);
        display.display();
    }
#endif

    showDisp = true;
    curTab = 3;
    oledSleepTimeout = millis() + (config.oled_timeout * 1000);

    // enableLoopWDT();
    // enableCore0WDT();
    enableCore1WDT();

    // Task 1
    xTaskCreatePinnedToCore(
        taskAPRS,        /* Function to implement the task */
        "taskAPRS",      /* Name of the task */
        8192,            /* Stack size in words */
        NULL,            /* Task input parameter */
        1,               /* Priority of the task */
        &taskAPRSHandle, /* Task handle. */
        0);              /* Core where the task should run */

    // Task 2
    xTaskCreatePinnedToCore(
        taskNetwork,        /* Function to implement the task */
        "taskNetwork",      /* Name of the task */
        16384,              /* Stack size in words */
        NULL,               /* Task input parameter */
        1,                  /* Priority of the task */
        &taskNetworkHandle, /* Task handle. */
        1);                 /* Core where the task should run */
}

int pkgCount = 0;

float conv_coords(float in_coords)
{
    // Initialize the location.
    float f = in_coords;
    // Get the first two digits by turning f into an integer, then doing an integer divide by 100;
    // firsttowdigits should be 77 at this point.
    int firsttwodigits = ((int)f) / 100; // This assumes that f < 10000.
    float nexttwodigits = f - (float)(firsttwodigits * 100);
    float theFinalAnswer = (float)(firsttwodigits + nexttwodigits / 60.0);
    return theFinalAnswer;
}

void DD_DDDDDtoDDMMSS(float DD_DDDDD, int *DD, int *MM, int *SS)
{
    float uDD_DDDDD = abs(DD_DDDDD);
    *DD = (int)uDD_DDDDD;                       // сделали из 37.45545 это 37 т.е. Градусы
    *MM = (int)((uDD_DDDDD - *DD) * 60);        // получили минуты
    *SS = ((uDD_DDDDD - *DD) * 60 - *MM) * 100; // получили секунды
}

String send_fix_location()
{
    String tnc2Raw = "";
    int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
    char strtmp[300], loc[30];
    char lon_ew, lat_ns;
    lon_ew = 'E';
    lat_ns = 'N';
    memset(strtmp, 0, 300);
    DD_DDDDDtoDDMMSS(config.gps_lat, &lat_dd, &lat_mm, &lat_ss);
    DD_DDDDDtoDDMMSS(config.gps_lon, &lon_dd, &lon_mm, &lon_ss);
    if(config.gps_lat < 0){
        lat_ns = 'S';
    }
    if(config.gps_lon < 0){
        lon_ew = 'W';
    }
    if(strlen(config.aprs_object)>=3){
        sprintf(loc, ")%s!%02d%02d.%02d%c%c%03d%02d.%02d%c%c",config.aprs_object,lat_dd, lat_mm, lat_ss, lat_ns, config.aprs_table, lon_dd, lon_mm, lon_ss, lon_ew,config.aprs_symbol);
    }else{
        sprintf(loc, "!%02d%02d.%02d%c%c%03d%02d.%02d%c%c", lat_dd, lat_mm, lat_ss, lat_ns, config.aprs_table, lon_dd, lon_mm, lon_ss, lon_ew, config.aprs_symbol);
    }
    if (config.aprs_ssid == 0)
        sprintf(strtmp, "%s>APE32I", config.aprs_mycall);
    else
        sprintf(strtmp, "%s-%d>APE32I", config.aprs_mycall, config.aprs_ssid);
    tnc2Raw = String(strtmp);
    if (config.aprs_path[0] != 0)
    {
        tnc2Raw += ",";
        tnc2Raw += String(config.aprs_path);
    }
    tnc2Raw += ":";
    tnc2Raw += String(loc);
    tnc2Raw += String(config.aprs_comment);
    return tnc2Raw;
}

int packet2Raw(String &tnc2, AX25Msg &Packet)
{
    if (Packet.len < 5)
        return 0;
    tnc2 = String(Packet.src.call);
    if (Packet.src.ssid > 0)
    {
        tnc2 += String(F("-"));
        tnc2 += String(Packet.src.ssid);
    }
    tnc2 += String(F(">"));
    tnc2 += String(Packet.dst.call);
    if (Packet.dst.ssid > 0)
    {
        tnc2 += String(F("-"));
        tnc2 += String(Packet.dst.ssid);
    }
    for (int i = 0; i < Packet.rpt_count; i++)
    {
        tnc2 += String(",");
        tnc2 += String(Packet.rpt_list[i].call);
        if (Packet.rpt_list[i].ssid > 0)
        {
            tnc2 += String("-");
            tnc2 += String(Packet.rpt_list[i].ssid);
        }
        if (Packet.rpt_flags & (1 << i))
            tnc2 += "*";
    }
    tnc2 += String(F(":"));
    tnc2 += String((const char *)Packet.info);
    tnc2 += String("\n");

    // #ifdef DEBUG_TNC
    //     Serial.printf("[%d] ", ++pkgTNC_count);
    //     Serial.print(tnc2);
    // #endif
    return tnc2.length();
}

long sendTimer = 0;
bool AFSKInitAct = false;
int btn_count = 0;
long timeCheck = 0;
int timeHalfSec = 0;
long int wxTimeout=0;

#ifdef WX

int GetAPRSDataAvg(char *strData)
{
unsigned int i;

int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
    char strtmp[300], obj[13];

    //readData();
    //gmtime(timeStamp);
    //#asm("wdr");
    memset(&obj[0],0,sizeof(obj));
    memset(&strData[0],0,sizeof(strData));

    DD_DDDDDtoDDMMSS(config.gps_lat, &lat_dd, &lat_mm, &lat_ss);
    DD_DDDDDtoDDMMSS(config.gps_lon, &lon_dd, &lon_mm, &lon_ss);
    if(strlen(config.aprs_object)>=3){
        sprintf(obj,";%s",config.aprs_object);
        for(i=strlen(config.aprs_object)+1;i<10;i++){
            obj[i]=0x20;
        }
        obj[i]='*';
        //sprintf(loc, ";%s!%02d%02d.%02dN%c%03d%02d.%02dE%c",config.aprs_object,lat_dd, lat_mm, lat_ss, config.aprs_table, lon_dd, lon_mm, lon_ss, config.aprs_symbol);
    }else{
        sprintf(obj,"!");
        obj[1]=0;
        //sprintf(loc, "!%02d%02d.%02dN%c%03d%02d.%02dE%c", lat_dd, lat_mm, lat_ss, config.aprs_table, lon_dd, lon_mm, lon_ss, config.aprs_symbol);
    }
    if (config.aprs_ssid == 0)
        sprintf(strtmp, "%s>APE32I,WIDE1-1:", config.aprs_mycall);
    else
        sprintf(strtmp, "%s-%d>APE32I,WIDE1-1:", config.aprs_mycall, config.aprs_ssid);

    strcat(strData,strtmp);
    strcat(strData,obj);

     time_t now;
     time(&now);
    struct tm * info=gmtime(&now);
    sprintf(strtmp,"%02d%02d%02dz%02d%02d.%02dN/%03d%02d.%02dE_",info->tm_mday,info->tm_hour,info->tm_min,lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss);
    //sprintf(strtmp,"%02d%02d.%02dN/%03d%02d.%02dE_",lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss);
    strcat(strData,strtmp);

    sprintf(strtmp,"%03u/%03ug%03u",weather.winddirection,(unsigned int)(weather.windspeed*0.621),(unsigned int)(weather.windgust*0.621));
    strcat(strData,strtmp);

    unsigned int tempF=(unsigned int)((weather.temperature*9/5)+32);
    sprintf(strtmp,"t%03u",tempF);
    strcat(strData,strtmp);
    
    unsigned int rain=(unsigned int)((weather.rain*100.0F)/25.6F);
    unsigned int rain24=(unsigned int)((weather.rain24hr*100.0F)/25.6F);
    if(info->tm_min==0){
        rain=0;
        weather.rain=0;
    }
    if(info->tm_hour==0 && info->tm_min==0){
        rain24=0;
        weather.rain24hr=0;
    }
    sprintf(strtmp,"r%03up%03uP...",rain,rain24);
    strcat(strData,strtmp);

     if(weather.solar<1000)
        sprintf(strtmp,"L%03u",(unsigned int)weather.solar);
    else
        sprintf(strtmp,"l%03u",(unsigned int)weather.solar-1000);
    strcat(strData,strtmp);

    sprintf(strtmp,"h%02u",(unsigned int)weather.humidity);
    strcat(strData,strtmp);

    sprintf(strtmp,"b%05u",(unsigned int)(weather.barometric*10));
    strcat(strData,strtmp);

    //sprintf(strtmp,"m%03u",(int)((weather.soitemp*9/5)+32));
    //strcat(strData,strtmp);

    //sprintf(strtmp,"M%03u",(unsigned int)(weather.soihum/10));
    //strcat(strData,strtmp);

    // sprintf(strtmp,"W%04u",(unsigned int)(weather.water));
    // strcat(strData,strtmp);

    sprintf(strtmp," BAT:%0.2fV/%dmA",weather.vbat,(int)(weather.ibat*1000));
    strcat(strData,strtmp);
    //sprintf(strtmp,"/%0.1fmA",weather.ibat*1000);
    //strcat(strData,strtmp);

    i=strlen(strData);
    return i;
}

bool weatherUpdate=false;
void getWeather()
{
    String stream,weatherRaw;
	int st = 0;
	if (Serial.available()>30) {
		stream=Serial.readString();
        //Serial.println(stream);
        delay(100);
		st = stream.indexOf("DATA:");
		//Serial.printf("Find ModBus > %d",st);
		//Serial.println(stream);
		if (st>=0){
			//String data = stream.substring(st+5);
			weatherRaw = stream.substring(st + 5);
			weatherUpdate = true;
			//Serial.println("Weather DATA: " + weatherRaw);
            
		}
		else {
			st = stream.indexOf("CLK?");
			if (st >=0) {
                struct tm br_time;
	             getLocalTime(&br_time, 5000);
                char strtmp[30];
                SerialTNC.printf(strtmp,"#CLK=%02d/%02d/%02d,%02d:%02d:%02d\r\n",br_time.tm_year-2000,br_time.tm_mon,br_time.tm_mday,br_time.tm_hour,br_time.tm_min,br_time.tm_sec);
				//String data = stream.substring(st + 5);
				//weatherRaw = stream;
				//weatherUpdate = true;
			}
		}

		if (weatherUpdate) {
			//String value;
			float rainSample = getValue(weatherRaw, ',', 2).toFloat();
            weather.rain += rainSample;
            weather.rain24hr+=rainSample;
			weather.windspeed = getValue(weatherRaw, ',', 3).toFloat();
			weather.winddirection = getValue(weatherRaw, ',', 4).toInt();
			weather.solar = getValue(weatherRaw, ',', 5).toInt();
			weather.barometric = getValue(weatherRaw, ',', 6).toFloat()*10.0F;
			weather.temperature = getValue(weatherRaw, ',', 7).toFloat();
			weather.humidity = getValue(weatherRaw, ',', 8).toFloat();
			weather.windgust = getValue(weatherRaw, ',', 13).toFloat();
            weather.vbat = getValue(weatherRaw, ',', 9).toFloat();
            weather.vsolar = getValue(weatherRaw, ',', 10).toFloat();
            weather.ibat = getValue(weatherRaw, ',', 11).toFloat();
            weather.pbat = getValue(weatherRaw, ',', 12).toFloat();

            weatherUpdate=false;
            wxTimeout=millis();
            char strData[300];
            size_t lng=GetAPRSDataAvg(strData);
            if (aprsClient.connected())
                    aprsClient.println(String(strData)); // Send packet to Inet
            else
                    pkgTxUpdate(strData, 0);
            
//Serial.printf("APRS: %s\r\n",strData);
		}
	Serial.flush();
	}
	//ModbusSerial.flush();
}
#endif

void loop()
{
    vTaskDelay(5 / portTICK_PERIOD_MS);
#ifdef WX
    getWeather();
#endif

    if (digitalRead(0) == LOW)
    {
        btn_count++;
        if (btn_count > 1000) // Push BOOT 10sec
        {
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(LED_TX_PIN, HIGH);
        }
    }
    else
    {
        if (btn_count > 0)
        {
            // Serial.printf("btn_count=%dms\n", btn_count * 10);
            if (btn_count > 1000) // Push BOOT 10sec to Factory Default
            {
                // digitalWrite(LED_RX, LOW);
                // digitalWrite(LED_TX, LOW);
                defaultConfig();
                Serial.println("SYSTEM REBOOT NOW!");
                esp_restart();
            }
            else
            {
                showDisp = true;
                if (oledSleepTimeout > 0)
                {
                    curTab++;
                    if (curTab > 3)
                        curTab = 0;
                }
            }
            btn_count = 0;
        }
    }

#ifdef OLED
    // Popup Display
    if (config.oled_enable == true)
    {
        if (dispBuffer.getCount() > 0)
        {
            if (millis() > timeHalfSec)
            {
                char tnc2[300];
                dispBuffer.pop(&tnc2);
                dispWindow(String(tnc2), 0, false);
            }
        }
        else
        {
            // Sleep display
            if (millis() > timeHalfSec)
            {
                if (timeHalfSec > 0)
                {
                    timeHalfSec = 0;
                    oledSleepTimeout = millis() + (config.oled_timeout * 1000);
                }
                else
                {
                    if (millis() > oledSleepTimeout && oledSleepTimeout > 0)
                    {
                        oledSleepTimeout = 0;
                        display.clearDisplay();
                        display.display();
                    }
                }
            }
        }

        if (showDisp)
        {
            showDisp = false;
            timeHalfSec = 0;
            oledSleepTimeout = millis() + (config.oled_timeout * 1000);
            switch (curTab)
            {
            case 0:
                statisticsDisp();
                break;
            case 1:
                pkgLastDisp();
                break;
            case 2:
                pkgCountDisp();
                break;
            case 3:
                systemDisp();
                break;
            }
        }
    }
#endif

    if (millis() > timeCheck)
    {
        timeCheck = millis() + 10000;
        if (ESP.getFreeHeap() < 60000)
            esp_restart();
        // Serial.println(String(ESP.getFreeHeap()));
    }
#ifdef SA818
    // if (SerialRF.available())
    // {
    //     Serial.print(Serial.readString());
    // }
#endif
    if (AFSKInitAct == true)
    {
#ifdef SA818
        AFSK_Poll(true, config.rf_power);
#else
        AFSK_Poll(false, LOW);
#endif
    }
}

String sendIsAckMsg(String toCallSign, int msgId)
{
    char str[300];
    char call[11];
    int i;
    memset(&call[0], 0, 11);
    sprintf(call, "%s-%d", config.aprs_mycall, config.aprs_ssid);
    strcpy(&call[0], toCallSign.c_str());
    i = strlen(call);
    for (; i < 9; i++)
        call[i] = 0x20;
    memset(&str[0], 0, 300);

    sprintf(str, "%s-%d>APE32I%s::%s:ack%d", config.aprs_mycall, config.aprs_ssid, VERSION, call, msgId);
    //	client.println(str);
    return String(str);
}

void sendIsPkg(char *raw)
{
    char str[300];
    sprintf(str, "%s-%d>APE32I%s:%s", config.aprs_mycall, config.aprs_ssid, VERSION, raw);
    // client.println(str);
    String tnc2Raw = String(str);
    if (aprsClient.connected())
        aprsClient.println(tnc2Raw); // Send packet to Inet
    if (config.tnc && config.tnc_digi)
        pkgTxUpdate(str, 0);
}

void sendIsPkgMsg(char *raw)
{
    char str[300];
    char call[11];
    int i;
    memset(&call[0], 0, 11);
    if (config.aprs_ssid == 0)
        sprintf(call, "%s", config.aprs_mycall);
    else
        sprintf(call, "%s-%d", config.aprs_mycall, config.aprs_ssid);
    i = strlen(call);
    for (; i < 9; i++)
        call[i] = 0x20;

    if (config.aprs_ssid == 0)
        sprintf(str, "%s>APE32I::%s:%s", config.aprs_mycall, call, raw);
    else
        sprintf(str, "%s-%d>APE32I::%s:%s", config.aprs_mycall, config.aprs_ssid, call, raw);

    String tnc2Raw = String(str);
    if (aprsClient.connected())
        aprsClient.println(tnc2Raw); // Send packet to Inet
    if (config.tnc && config.tnc_digi)
        pkgTxUpdate(str, 0);
    // APRS_sendTNC2Pkt(tnc2Raw); // Send packet to RF
}

long timeSlot;
void taskAPRS(void *pvParameters)
{
    //	long start, stop;
    // char *raw;
    // char *str;
    Serial.println("Task APRS has been start");
    PacketBuffer.clean();

    APRS_init();
    APRS_setCallsign(config.aprs_mycall, config.aprs_ssid);
    APRS_setPath1(config.aprs_path, 1);
    APRS_setPreamble(300);
    APRS_setTail(0);
    sendTimer = millis() - (config.aprs_beacon * 1000) + 30000;
    igateTLM.TeleTimeout = millis() + 60000; // 1Min
    AFSKInitAct = true;
    timeSlot = millis();
    for (;;)
    {
        long now = millis();
        // wdtSensorTimer = now;
        time_t timeStamp;
        time(&timeStamp);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // serviceHandle();
        if (now > (timeSlot + 10))
        {
            if (!digitalRead(LED_PIN))
            { // RX State Fail
                if (pkgTxSend())
                    timeSlot = millis() + config.tx_timeslot; // Tx Time Slot = 5sec.
                else
                    timeSlot = millis();
            }
            else
            {
                timeSlot = millis() + 500;
            }
        }

//         if (digitalRead(0) == LOW)
//         {
//             btn_count++;
//             if (btn_count > 1000) // Push BOOT 10sec
//             {
//                 digitalWrite(LED_PIN, HIGH);
//                 digitalWrite(LED_TX_PIN, HIGH);
//             }
//         }
//         else
//         {
//             if (btn_count > 0)
//             {
//                 // Serial.printf("btn_count=%dms\n", btn_count * 10);
//                 if (btn_count > 1000) // Push BOOT 10sec to Factory Default
//                 {
//                     defaultConfig();
//                     Serial.println("SYSTEM REBOOT NOW!");
//                     esp_restart();
//                 }
//                 else if (btn_count > 10) // Push BOOT >100mS to PTT Fix location
//                 {
//                     if (config.tnc)
//                     {
//                         String tnc2Raw = send_fix_location();
//                         pkgTxUpdate(tnc2Raw.c_str(), 0);
//                         // APRS_sendTNC2Pkt(tnc2Raw); // Send packet to RF
// #ifdef DEBUG_TNC
//                         Serial.println("Manual TX: " + tnc2Raw);
// #endif
//                     }
//                 }
//                 btn_count = 0;
//             }
//         }
#ifdef SA818
// if(digitalRead(SQL_PIN)==HIGH){
// 	delay(10);
// 	if(digitalRead(SQL_PIN)==LOW){
// 		while(SerialRF.available()) SerialRF.read();
// 		SerialRF.println("RSSI?");
// 		delay(100);
// 		String ret=SerialRF.readString();
// 		Serial.println(ret);
// 		if(ret.indexOf("RSSI=")>=0){
// 			String sig=getValue(ret,'=',2);
// 			Serial.printf("SIGNAL %s\n",sig.c_str());
// 		}
// 	}
// }
#endif

        if (now > (sendTimer + (config.aprs_beacon * 1000)))
        {
            sendTimer = now;
            if (digiCount > 0)
                digiCount--;
#ifdef SA818
            SA818_CHECK();
#endif
            if (AFSKInitAct == true)
            {
                #ifdef WX
                if((millis()-wxTimeout)>(config.aprs_beacon * 1000)){
                #endif
                String tnc2Raw = send_fix_location();
                if (aprsClient.connected())
                    aprsClient.println(tnc2Raw); // Send packet to Inet
                if (config.tnc)
                {                    
                    pkgTxUpdate(tnc2Raw.c_str(), 0);
                    // APRS_sendTNC2Pkt(tnc2Raw);       // Send packet to RF
#ifdef DEBUG_TNC
                    // Serial.println("TX: " + tnc2Raw);
#endif
                }
                #ifdef WX
                }
                #endif
            }
            // send_fix_location();
            //  APRS_setCallsign(config.aprs_mycall, config.aprs_ssid);
            //  	APRS_setPath1("WIDE1", 1);
            //  	APRS_setPreamble(350);
            //  	APRS_setTail(50);
            // APRS_sendTNC2Pkt("HS5TQA-6>APE32I,TRACE2-2:=1343.76N/10026.06E&ESP32 APRS Internet Gateway");
        }

        if (config.tnc_telemetry)
        {
            if (igateTLM.TeleTimeout < millis())
            {
                igateTLM.TeleTimeout = millis() + 600000; // 10Min
                if ((igateTLM.Sequence % 6) == 0)
                {
                    sendIsPkgMsg((char *)&PARM[0]);
                    sendIsPkgMsg((char *)&UNIT[0]);
                    sendIsPkgMsg((char *)&EQNS[0]);
                }
                char rawTlm[100];
                if (config.aprs_ssid == 0)
                    sprintf(rawTlm, "%s>APE32I:T#%03d,%d,%d,%d,%d,%d,00000000", config.aprs_mycall, igateTLM.Sequence, igateTLM.RF2INET, igateTLM.INET2RF, igateTLM.RX, igateTLM.TX, igateTLM.DROP);
                else
                    sprintf(rawTlm, "%s-%d>APE32I:T#%03d,%d,%d,%d,%d,%d,00000000", config.aprs_mycall, config.aprs_ssid, igateTLM.Sequence, igateTLM.RF2INET, igateTLM.INET2RF, igateTLM.RX, igateTLM.TX, igateTLM.DROP);

                if (aprsClient.connected())
                    aprsClient.println(String(rawTlm)); // Send packet to Inet
                if (config.tnc && config.tnc_digi)
                    pkgTxUpdate(rawTlm, 0);
                // APRS_sendTNC2Pkt(String(rawTlm)); // Send packet to RF
                igateTLM.Sequence++;
                if (igateTLM.Sequence > 999)
                    igateTLM.Sequence = 0;
                igateTLM.DROP = 0;
                igateTLM.INET2RF = 0;
                igateTLM.RF2INET = 0;
                igateTLM.RX = 0;
                igateTLM.TX = 0;
                // client.println(raw);
            }
        }

        // IGate RF->INET
        if (config.tnc)
        {
            if (PacketBuffer.getCount() > 0)
            {
                String tnc2;
                // นำข้อมูลแพ็จเกจจาก TNC ออกจากคิว
                PacketBuffer.pop(&incomingPacket);
                packet2Raw(tnc2, incomingPacket);

                if (config.oled_enable == true)
                {
                    // if (config.dispTNC == true)
                    dispBuffer.push(tnc2.c_str());
                }

                // IGate Process
                if (config.rf2inet && aprsClient.connected())
                {
                    int ret = igateProcess(incomingPacket);
                    if (ret == 0)
                    {
                        status.dropCount++;
                        igateTLM.DROP++;
                    }
                    else
                    {
                        status.rf2inet++;
                        igateTLM.RF2INET++;
                        // igateTLM.TX++;
#ifdef DEBUG
                        printTime();
                        Serial.print("RF->INET: ");
                        Serial.println(tnc2);
#endif
                        char call[11];
                        if (incomingPacket.src.ssid > 0)
                            sprintf(call, "%s-%d", incomingPacket.src.call, incomingPacket.src.ssid);
                        else
                            sprintf(call, "%s", incomingPacket.src.call);

                        uint8_t type = pkgType((char *)incomingPacket.info);
                        pkgListUpdate(call, type);
                    }
                }

                // Digi Repeater Process
                if (config.tnc_digi)
                {
                    int dlyFlag = digiProcess(incomingPacket);
                    if (dlyFlag > 0)
                    {
                        int digiDelay;
                        status.digiCount++;
                        igateTLM.RX++;
                        if (dlyFlag == 1)
                        {
                            digiDelay = 0;
                        }
                        else
                        {
                            if (config.digi_delay == 0)
                            { // Auto mode
                                if (digiCount > 20)
                                    digiDelay = random(5000);
                                else if (digiCount > 10)
                                    digiDelay = random(3000);
                                else if (digiCount > 0)
                                    digiDelay = random(1500);
                                else
                                    digiDelay = random(500);
                            }
                            else
                            {
                                digiDelay = random(config.digi_delay);
                            }
                        }
                        String digiPkg;
                        packet2Raw(digiPkg, incomingPacket);
                        pkgTxUpdate(digiPkg.c_str(), digiDelay);
                    }
                    else
                    {
                        igateTLM.DROP++;
                    }
                }

                lastPkg = true;
                lastPkgRaw = tnc2;
                // ESP_BT.println(tnc2);
                status.allCount++;
            }
        }
    }
}

int mqttRetry = 0;
long wifiTTL = 0;

void taskNetwork(void *pvParameters)
{
    int c = 0;
    Serial.println("Task Network has been start");
    //     pinMode(MODEM_PWRKEY, OUTPUT);
    //         // Pull down PWRKEY for more than 1 second according to manual requirements
    //     digitalWrite(MODEM_PWRKEY, HIGH);
    //     delay(100);
    //     digitalWrite(MODEM_PWRKEY, LOW);
    //     delay(1000);
    //     digitalWrite(MODEM_PWRKEY, HIGH);

    // Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    //   Serial1.setTimeout(10);
    //   Serial1.setRxBufferSize(2048);
    //   ppp.begin(&Serial1);

    //   Serial.print("Connecting PPPoS");
    //   ppp.connect(PPP_APN, PPP_USER, PPP_PASS);
    //   while (!ppp.status()) {
    //     delay(500);
    //     Serial.print(".");
    //   }
    //   Serial.println("OK");

    if (config.wifi_mode == WIFI_AP_STA_FIX || config.wifi_mode == WIFI_AP_FIX)
    { // AP=false
        // WiFi.mode(config.wifi_mode);
        if (config.wifi_mode == WIFI_AP_STA_FIX)
        {
            WiFi.mode(WIFI_AP_STA);
        }
        else if (config.wifi_mode == WIFI_AP_FIX)
        {
            WiFi.mode(WIFI_AP);
        }
        // กำหนดค่าการทำงานไวไฟเป็นแอสเซสพ้อย
        WiFi.softAP(config.wifi_ap_ssid, config.wifi_ap_pass); // Start HOTspot removing password will disable security
        WiFi.softAPConfig(local_IP, gateway, subnet);
        Serial.print("Access point running. IP address: ");
        Serial.print(WiFi.softAPIP());
        Serial.println("");
    }
    else if (config.wifi_mode == WIFI_STA_FIX)
    {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        Serial.println(F("WiFi Station Only mode."));
    }
    else
    {
        WiFi.mode(WIFI_OFF);
        WiFi.disconnect(true);
        delay(100);
        Serial.println(F("WiFi OFF All mode."));
        // SerialBT.begin("ESP32TNC");
    }

    webService();
    pingTimeout = millis() + 10000;
    for (;;)
    {
        // wdtNetworkTimer = millis();
        vTaskDelay(5 / portTICK_PERIOD_MS);
        serviceHandle();

        if (config.wifi_mode == WIFI_AP_STA_FIX || config.wifi_mode == WIFI_STA_FIX)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                unsigned long int tw = millis();
                if (tw > wifiTTL)
                {
#ifndef I2S_INTERNAL
                    AFSK_TimerEnable(false);
#endif
                    wifiTTL = tw + 60000;
                    Serial.println("WiFi connecting..");
                    // udp.endPacket();
                    WiFi.disconnect();
                    WiFi.setTxPower((wifi_power_t)config.wifi_power);
                    WiFi.setHostname("ESP32IGate");
                    WiFi.begin(config.wifi_ssid, config.wifi_pass);
                    if (config.vpn)
                        wireguard_remove();
                    // Wait up to 1 minute for connection...
                    for (c = 0; (c < 30) && (WiFi.status() != WL_CONNECTED); c++)
                    {
                        // Serial.write('.');
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        // for (t = millis(); (millis() - t) < 1000; refresh());
                    }
                    if (c >= 30)
                    { // If it didn't connect within 1 min
                        Serial.println("Failed. Will retry...");
                        WiFi.disconnect();
                        // WiFi.mode(WIFI_OFF);
                        delay(3000);
                        // WiFi.mode(WIFI_STA);
                        WiFi.reconnect();
                        continue;
                    }

                    Serial.println("WiFi connected");
                    Serial.print("IP address: ");
                    Serial.println(WiFi.localIP());

                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    NTP_Timeout = millis() + 5000;
// Serial.println("Contacting Time Server");
// configTime(3600 * timeZone, 0, "aprs.dprns.com", "1.pool.ntp.org");
// vTaskDelay(3000 / portTICK_PERIOD_MS);
#ifndef I2S_INTERNAL
                    AFSK_TimerEnable(true);
#endif
                }
            }
            else
            {

                if (millis() > NTP_Timeout)
                {
                    NTP_Timeout = millis() + 86400000;
                    // Serial.println("Config NTP");
                    // setSyncProvider(getNtpTime);
                    Serial.println("Contacting Time Server");
                    configTime(3600 * config.timeZone, 0, "203.150.19.26", "110.170.126.101", "77.68.122.252");
                    vTaskDelay(3000 / portTICK_PERIOD_MS);
                    time_t systemTime;
                    time(&systemTime);
                    setTime(systemTime);
                    if (systemUptime == 0)
                    {
                        systemUptime = now();
                    }
                    pingTimeout = millis() + 2000;
                    if (config.vpn)
                    {
                        if (!wireguard_active())
                        {
                            Serial.println("Setup Wiregurad VPN!");
                            wireguard_setup();
                        }
                    }
                }

                if (config.aprs)
                {
                    if (aprsClient.connected() == false)
                    {
                        APRSConnect();
                    }
                    else
                    {
                        if (aprsClient.available())
                        {
                            pingTimeout = millis() + 300000;                // Reset ping timout
                            String line = aprsClient.readStringUntil('\n'); // อ่านค่าที่ Server ตอบหลับมาทีละบรรทัด
#ifdef DEBUG_IS
                            printTime();
                            Serial.print("APRS-IS ");
                            Serial.println(line);
#endif
                            status.isCount++;
                            int start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
                            if (start_val > 3)
                            {
                                if (config.dispINET == true)
                                    dispBuffer.push(line.c_str());

                                // raw = (char *)malloc(line.length() + 1);
                                String src_call = line.substring(0, start_val);
                                String msg_call = "::" + src_call;

                                status.allCount++;
                                // igateTLM.RX++;
                                if (config.tnc && config.inet2rf)
                                {
                                    if (line.indexOf(msg_call) <= 0) // src callsign = msg callsign ไม่ใช่หัวข้อโทรมาตร
                                    {
                                        if (line.indexOf(":T#") < 0) // ไม่ใช่ข้อความโทรมาตร
                                        {
                                            if (line.indexOf("::") > 0) // ข้อความเท่านั้น
                                            {                           // message only
                                                // raw[0] = '}';
                                                // line.toCharArray(&raw[1], line.length());
                                                // tncTxEnable = false;
                                                // SerialTNC.flush();
                                                // SerialTNC.println(raw);
                                                pkgTxUpdate(line.c_str(), 0);
                                                // APRS_sendTNC2Pkt(line); // Send out RF by TNC build in
                                                //  tncTxEnable = true;
                                                status.inet2rf++;
                                                igateTLM.INET2RF++;
                                                printTime();
#ifdef DEBUG
                                                Serial.print("INET->RF ");
                                                Serial.println(line);
#endif
                                            }
                                        }
                                    }
                                    else
                                    {
                                        igateTLM.DROP++;
                                        Serial.print("INET Message TELEMETRY from ");
                                        Serial.println(src_call);
                                    }
                                }

                                // memset(&raw[0], 0, sizeof(raw));
                                // line.toCharArray(&raw[0], start_val + 1);
                                // raw[start_val + 1] = 0;
                                // pkgListUpdate(&raw[0], 0);
                                // free(raw);
                            }
                        }
                    }
                }

                // if (millis() > pingTimeout)
                //                 {
                //                     pingTimeout = millis() + 3000;
                //                     Serial.print("Ping to " + vpn_IP.toString());
                //                     if (ping_start(vpn_IP, 3, 0, 0, 10) == true)
                //                     {
                //                         Serial.println("VPN Ping Success!!");
                //                     }
                //                     else
                //                     {
                //                         Serial.println("VPN Ping Fail!");
                //                     }
                //                 }

                if (millis() > pingTimeout)
                {
                    pingTimeout = millis() + 300000;
                    Serial.println("Ping GW to " + WiFi.gatewayIP().toString());
                    if (ping_start(WiFi.gatewayIP(), 3, 0, 0, 5) == true)
                    {
                        Serial.println("GW Success!!");
                    }
                    else
                    {
                        Serial.println("GW Fail!");
                        WiFi.disconnect();
                        wifiTTL = 0;
                    }
                    if (config.vpn)
                    {
                        IPAddress vpnIP;
                        vpnIP.fromString(String(config.wg_gw_address));
                        Serial.println("Ping VPN to " + vpnIP.toString());
                        if (ping_start(vpnIP, 2, 0, 0, 10) == true)
                        {
                            Serial.println("VPN Ping Success!!");
                        }
                        else
                        {
                            Serial.println("VPN Ping Fail!");
                            wireguard_remove();
                            delay(3000);
                            wireguard_setup();
                        }
                    }
                }
            }
        }
    }
}

// Routine
void line_angle(signed int startx, signed int starty, unsigned int length, unsigned int angle, unsigned int color)
{
    display.drawLine(startx, starty, (startx + length * cosf(angle * 0.017453292519)), (starty + length * sinf(angle * 0.017453292519)), color);
}

int xSpiGlcdSelFontHeight = 8;
int xSpiGlcdSelFontWidth = 5;

void compass_label(signed int startx, signed int starty, unsigned int length, double angle, unsigned int color)
{
    double angleNew;
    // ushort Color[2];
    uint8_t x_N, y_N, x_S, y_S;
    int x[4], y[4], i;
    int xOffset, yOffset;
    yOffset = (xSpiGlcdSelFontHeight / 2);
    xOffset = (xSpiGlcdSelFontWidth / 2);
    // GLCD_WindowMax();
    angle += 270.0F;
    angleNew = angle;
    for (i = 0; i < 4; i++)
    {
        if (angleNew > 360.0F)
            angleNew -= 360.0F;
        x[i] = startx + (length * cosf(angleNew * 0.017453292519));
        y[i] = starty + (length * sinf(angleNew * 0.017453292519));
        x[i] -= xOffset;
        y[i] -= yOffset;
        angleNew += 90.0F;
    }
    angleNew = angle + 45.0F;
    for (i = 0; i < 4; i++)
    {
        if (angleNew > 360.0F)
            angleNew -= 360.0F;
        x_S = startx + ((length - 3) * cosf(angleNew * 0.017453292519));
        y_S = starty + ((length - 3) * sinf(angleNew * 0.017453292519));
        x_N = startx + ((length + 3) * cosf(angleNew * 0.017453292519));
        y_N = starty + ((length + 3) * sinf(angleNew * 0.017453292519));
        angleNew += 90.0F;
        display.drawLine(x_S, y_S, x_N, y_N, color);
    }
    display.drawCircle(startx, starty, length, color);
    display.setFont();
    display.drawChar((uint8_t)x[0], (uint8_t)y[0], 'N', WHITE, BLACK, 1);
    display.drawChar((uint8_t)x[1], (uint8_t)y[1], 'E', WHITE, BLACK, 1);
    display.drawChar((uint8_t)x[2], (uint8_t)y[2], 'S', WHITE, BLACK, 1);
    display.drawChar((uint8_t)x[3], (uint8_t)y[3], 'W', WHITE, BLACK, 1);
}

void compass_arrow(signed int startx, signed int starty, unsigned int length, double angle, unsigned int color)
{
    double angle1, angle2;
    int xdst, ydst, x1sta, y1sta, x2sta, y2sta;
    int length2 = length / 2;
    angle += 270.0F;
    if (angle > 360.0F)
        angle -= 360.0F;
    xdst = startx + length * cosf(angle * 0.017453292519);
    ydst = starty + length * sinf(angle * 0.017453292519);
    angle1 = angle + 135.0F;
    if (angle1 > 360.0F)
        angle1 -= 360.0F;
    angle2 = angle + 225.0F;
    if (angle2 > 360.0F)
        angle2 -= 360.0F;
    x1sta = startx + length2 * cosf(angle1 * 0.017453292519);
    y1sta = starty + length2 * sinf(angle1 * 0.017453292519);
    x2sta = startx + length2 * cosf(angle2 * 0.017453292519);
    y2sta = starty + length2 * sinf(angle2 * 0.017453292519);
    display.drawLine(startx, starty, xdst, ydst, color);
    display.drawLine(xdst, ydst, x1sta, y1sta, color);
    display.drawLine(x1sta, y1sta, startx, starty, color);
    display.drawLine(startx, starty, x2sta, y2sta, color);
    display.drawLine(x2sta, y2sta, xdst, ydst, color);
}

const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
bool dispPush = 0;
unsigned long disp_delay = 0;

void dispWindow(String line, uint8_t mode, bool filter)
{
    uint16_t bgcolor, txtcolor;
    bool Monitor = false;
    char text[200];
    unsigned char x = 0;
    char itemname[10];
    int start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
    if (start_val > 3)
    {
        // Serial.println(line);
        String src_call = line.substring(0, start_val);
        memset(&aprs, 0, sizeof(pbuf_t));
        aprs.buf_len = 300;
        aprs.packet_len = line.length();
        line.toCharArray(&aprs.data[0], aprs.packet_len);
        int start_info = line.indexOf(":", 0);
        int end_ssid = line.indexOf(",", 0);
        int start_dst = line.indexOf(">", 2);
        int start_dstssid = line.indexOf("-", start_dst);
        if ((start_dstssid > start_dst) && (start_dstssid < start_dst + 10))
        {
            aprs.dstcall_end_or_ssid = &aprs.data[start_dstssid];
        }
        else
        {
            aprs.dstcall_end_or_ssid = &aprs.data[end_ssid];
        }
        aprs.info_start = &aprs.data[start_info + 1];
        aprs.dstname = &aprs.data[start_dst + 1];
        aprs.dstname_len = end_ssid - start_dst;
        aprs.dstcall_end = &aprs.data[end_ssid];
        aprs.srccall_end = &aprs.data[start_dst];

        // Serial.println(aprs.info_start);
        // aprsParse.parse_aprs(&aprs);
        if (aprsParse.parse_aprs(&aprs))
        {
            if (filter == true)
            {
                if (config.filterStatus && (aprs.packettype & T_STATUS))
                {
                    Monitor = true;
                }
                else if (config.filterMessage && (aprs.packettype & T_MESSAGE))
                {
                    Monitor = true;
                }
                else if (config.filterTelemetry && (aprs.packettype & T_TELEMETRY))
                {
                    Monitor = true;
                }
                else if (config.filterWeather && (aprs.packettype & T_WX))
                {
                    Monitor = true;
                }

                if (config.filterPosition && (aprs.packettype & T_POSITION))
                {
                    double lat, lon;
                    if ((config.mygps == true) && gps.location.isValid())
                    {
                        lat = gps.location.lat();
                        lon = gps.location.lng();
                    }
                    else
                    {
                        lat = config.gps_lat;
                        lon = config.gps_lon;
                    }
                    double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                    if (config.filterDistant == 0)
                    {
                        Monitor = true;
                    }
                    else
                    {
                        if (dist < config.filterDistant)
                            Monitor = true;
                        else
                            Monitor = false;
                    }
                }

                if (config.filterTracker && (aprs.packettype & T_POSITION))
                {
                    if (aprs.flags & F_CSRSPD)
                    {
                        double lat, lon;
                        if ((config.mygps == true) && gps.location.isValid())
                        {
                            lat = gps.location.lat();
                            lon = gps.location.lng();
                        }
                        else
                        {
                            lat = config.gps_lat;
                            lon = config.gps_lon;
                        }
                        double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                        if (config.filterDistant == 0)
                        {
                            Monitor = true;
                        }
                        else
                        {
                            if (dist < config.filterDistant)
                                Monitor = true;
                            else
                                Monitor = false;
                        }
                    }
                }

                if (config.filterMove && (aprs.packettype & T_POSITION))
                {
                    if (aprs.flags & F_CSRSPD)
                    {
                        if (aprs.speed > 0)
                        {
                            double lat, lon;
                            if ((config.mygps == true) && gps.location.isValid())
                            {
                                lat = gps.location.lat();
                                lon = gps.location.lng();
                            }
                            else
                            {
                                lat = config.gps_lat;
                                lon = config.gps_lon;
                            }
                            double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                            if (config.filterDistant == 0)
                            {
                                Monitor = true;
                            }
                            else
                            {
                                if (dist < config.filterDistant)
                                    Monitor = true;
                                else
                                    Monitor = false;
                            }
                        }
                    }
                }
            }
            else
            {
                Monitor = true;
            }
        }
        else
        {
            return;
        }

        if (Monitor)
        {
            display.clearDisplay();
            if (dispPush)
            {
                disp_delay = 600 * 1000;
                display.drawRoundRect(0, 0, 128, 16, 5, WHITE);
            }
            else
            {
                disp_delay = config.dispDelay * 1000;
            }
            timeHalfSec = millis() + disp_delay;
            // display.fillRect(0, 0, 128, 16, WHITE);
            const uint8_t *ptrSymbol;
            uint8_t symIdx = aprs.symbol[1] - 0x21;
            if (symIdx > 95)
                symIdx = 0;
            if (aprs.symbol[0] == '/')
            {
                ptrSymbol = &Icon_TableA[symIdx][0];
            }
            else if (aprs.symbol[0] == '\\')
            {
                ptrSymbol = &Icon_TableB[symIdx][0];
            }
            else
            {
                if (aprs.symbol[0] < 'A' || aprs.symbol[0] > 'Z')
                {
                    aprs.symbol[0] = 'N';
                    aprs.symbol[1] = '&';
                    symIdx = 5; // &
                }
                ptrSymbol = &Icon_TableB[symIdx][0];
            }
            display.drawYBitmap(0, 0, ptrSymbol, 16, 16, WHITE);
            if (!(aprs.symbol[0] == '/' || aprs.symbol[0] == '\\'))
            {
                display.drawChar(5, 4, aprs.symbol[0], BLACK, WHITE, 1);
                display.drawChar(6, 5, aprs.symbol[0], BLACK, WHITE, 1);
            }
            display.setCursor(20, 7);
            display.setTextSize(1);
            display.setFont(&FreeSansBold9pt7b);

            if (aprs.srcname_len > 0)
            {
                memset(&itemname, 0, sizeof(itemname));
                memcpy(&itemname, aprs.srcname, aprs.srcname_len);
                Serial.println(itemname);
                display.print(itemname);
            }
            else
            {
                display.print(src_call);
            }

            display.setFont();
            display.setTextColor(WHITE);
            // if(selTab<10)
            // 	display.setCursor(121, 0);
            // else
            // 	display.setCursor(115, 0);
            // display.print(selTab);

            if (mode == 1)
            {
                display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(40, 18);
                display.print("TNC2 RAW");

                display.setFont();
                display.setCursor(2, 30);
                display.setTextColor(WHITE);
                display.print(line);

                display.display();
                return;
            }

            if (aprs.packettype & T_TELEMETRY)
            {
                bool show = false;
                int idx = tlmList_Find((char *)src_call.c_str());
                if (idx < 0)
                {
                    idx = tlmListOld();
                    if (idx > -1)
                        memset(&Telemetry[idx], 0, sizeof(Telemetry_struct));
                }
                if (idx > -1)
                {
                    Telemetry[idx].time = now();
                    strcpy(Telemetry[idx].callsign, (char *)src_call.c_str());

                    // for (int i = 0; i < 3; i++) Telemetry[idx].UNIT[i][5] = 0;
                    if (aprs.flags & F_UNIT)
                    {
                        memcpy(Telemetry[idx].UNIT, aprs.tlm_unit.val, sizeof(Telemetry[idx].UNIT));
                    }
                    else if (aprs.flags & F_PARM)
                    {
                        memcpy(Telemetry[idx].PARM, aprs.tlm_parm.val, sizeof(Telemetry[idx].PARM));
                    }
                    else if (aprs.flags & F_EQNS)
                    {
                        Telemetry[idx].EQNS_FLAG = true;
                        for (int i = 0; i < 15; i++)
                            Telemetry[idx].EQNS[i] = aprs.tlm_eqns.val[i];
                    }
                    else if (aprs.flags & F_BITS)
                    {
                        Telemetry[idx].BITS_FLAG = aprs.telemetry.bitsFlag;
                    }
                    else if (aprs.flags & F_TLM)
                    {
                        for (int i = 0; i < 5; i++)
                            Telemetry[idx].RAW[i] = aprs.telemetry.val[i];
                        Telemetry[idx].BITS = aprs.telemetry.bits;
                        show = true;
                    }

                    for (int i = 0; i < 4; i++)
                    { // Cut length
                        if (strstr(Telemetry[idx].PARM[i], "RxTraffic") != 0)
                            sprintf(Telemetry[idx].PARM[i], "RX");
                        if (strstr(Telemetry[idx].PARM[i], "TxTraffic") != 0)
                            sprintf(Telemetry[idx].PARM[i], "TX");
                        if (strstr(Telemetry[idx].PARM[i], "RxDrop") != 0)
                            sprintf(Telemetry[idx].PARM[i], "DROP");
                        Telemetry[idx].PARM[i][6] = 0;
                        Telemetry[idx].UNIT[i][3] = 0;
                        for (int a = 0; a < 3; a++)
                        {
                            if (Telemetry[idx].UNIT[i][a] == '/')
                                Telemetry[idx].UNIT[i][a] = 0;
                        }
                    }

                    for (int i = 0; i < 5; i++)
                    {
                        if (Telemetry[idx].PARM[i][0] == 0)
                        {
                            sprintf(Telemetry[idx].PARM[i], "CH%d", i + 1);
                        }
                    }
                }
                if (show || filter == false)
                {
                    display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                    display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                    display.setTextColor(BLACK);
                    display.setCursor(40, 18);
                    display.print("TELEMETRY");
                    display.setFont();
                    display.setTextColor(WHITE);
                    display.setCursor(2, 28);
                    display.print(Telemetry[idx].PARM[0]);
                    display.print(":");

                    if (Telemetry[idx].EQNS_FLAG == true)
                    {
                        for (int i = 0; i < 5; i++)
                        {
                            // a*v^2+b*v+c
                            Telemetry[idx].VAL[i] = (Telemetry[idx].EQNS[(i * 3) + 0] * pow(Telemetry[idx].RAW[i], 2));
                            Telemetry[idx].VAL[i] += Telemetry[idx].EQNS[(i * 3) + 1] * Telemetry[idx].RAW[i];
                            Telemetry[idx].VAL[i] += Telemetry[idx].EQNS[(i * 3) + 2];
                        }
                    }
                    else
                    {
                        for (int i = 0; i < 5; i++)
                        {
                            // a*v^2+b*v+c
                            Telemetry[idx].VAL[i] = Telemetry[idx].RAW[i];
                        }
                    }

                    if (fmod(Telemetry[idx].VAL[0], 1) == 0)
                        display.print(Telemetry[idx].VAL[0], 0);
                    else
                        display.print(Telemetry[idx].VAL[0], 1);
                    display.print(Telemetry[idx].UNIT[0]);
                    display.setCursor(65, 28);
                    display.print(Telemetry[idx].PARM[1]);
                    display.print(":");
                    if (fmod(Telemetry[idx].VAL[1], 1) == 0)
                        display.print(Telemetry[idx].VAL[1], 0);
                    else
                        display.print(Telemetry[idx].VAL[1], 1);
                    display.print(Telemetry[idx].UNIT[1]);
                    display.setCursor(2, 37);
                    display.print(Telemetry[idx].PARM[2]);
                    display.print(":");
                    if (fmod(Telemetry[idx].VAL[2], 1) == 0)
                        display.print(Telemetry[idx].VAL[2], 0);
                    else
                        display.print(Telemetry[idx].VAL[2], 1);
                    display.print(Telemetry[idx].UNIT[2]);
                    display.setCursor(65, 37);
                    display.print(Telemetry[idx].PARM[3]);
                    display.print(":");
                    if (fmod(Telemetry[idx].VAL[3], 1) == 0)
                        display.print(Telemetry[idx].VAL[3], 0);
                    else
                        display.print(Telemetry[idx].VAL[3], 1);
                    display.print(Telemetry[idx].UNIT[3]);
                    display.setCursor(2, 46);
                    display.print(Telemetry[idx].PARM[4]);
                    display.print(":");
                    display.print(Telemetry[idx].VAL[4], 1);
                    display.print(Telemetry[idx].UNIT[4]);

                    display.setCursor(4, 55);
                    display.print("BIT");
                    uint8_t bit = Telemetry[idx].BITS;
                    for (int i = 0; i < 8; i++)
                    {
                        if (bit & 0x80)
                        {
                            display.fillCircle(30 + (i * 12), 58, 3, WHITE);
                        }
                        else
                        {
                            display.drawCircle(30 + (i * 12), 58, 3, WHITE);
                        }
                        bit <<= 1;
                    }
                    // display.print(Telemetry[idx].BITS, BIN);

                    // display.setFont();
                    // display.setCursor(2, 30);
                    // memset(&text[0], 0, sizeof(text));
                    // memcpy(&text[0], aprs.comment, aprs.comment_len);
                    // display.setTextColor(WHITE);
                    // display.print(aprs.comment);
                    display.display();
                }
                return;
            }
            else if (aprs.packettype & T_STATUS)
            {
                display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(48, 18);
                display.print("STATUS");

                display.setFont();
                display.setCursor(2, 30);
                // memset(&text[0], 0, sizeof(text));
                // memcpy(&text[0], aprs.comment, aprs.comment_len);
                display.setTextColor(WHITE);
                display.print(aprs.comment);
                display.display();
                return;
            }
            else if (aprs.packettype & T_QUERY)
            {
                display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(48, 18);
                display.print("?QUERY?");
                // memset(&text[0], 0, sizeof(text));
                // memcpy(&text[0], aprs.comment, aprs.comment_len);
                display.setFont();
                display.setTextColor(WHITE);
                display.setCursor(2, 30);
                display.print(aprs.comment);
                display.display();
                return;
            }
            else if (aprs.packettype & T_MESSAGE)
            {
                if (aprs.msg.is_ack == 1)
                {
                }
                else if (aprs.msg.is_rej == 1)
                {
                }
                else
                {
                    display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                    display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                    display.setTextColor(BLACK);
                    display.setCursor(48, 18);
                    display.print("MESSAGE");
                    display.setCursor(108, 18);
                    display.print("{");
                    strncpy(&text[0], aprs.msg.msgid, aprs.msg.msgid_len);
                    int msgid = atoi(text);
                    display.print(msgid, DEC);
                    display.print("}");
                    // memset(&text[0], 0, sizeof(text));
                    // memcpy(&text[0], aprs.comment, aprs.comment_len);
                    display.setFont();
                    display.setTextColor(WHITE);
                    display.setCursor(2, 30);
                    display.print("To: ");
                    strncpy(&text[0], aprs.dstname, aprs.dstname_len);
                    display.print(text);
                    String mycall;
                    if (config.aprs_ssid == 0)
                        mycall = config.aprs_mycall;
                    else
                        mycall = config.aprs_mycall + String("-") + String(config.aprs_ssid, DEC);
                    if (strcmp(mycall.c_str(), text) == 0)
                    {
                        display.setCursor(2, 54);
                        display.print("ACK:");
                        display.println(msgid);
                        sendIsAckMsg(src_call, msgid);
                        // client.println(raw);
                        // SerialTNC.println("}" + raw);
                        // if (slot == 0) {
                        //	client.println(raw);
                        //}
                        // else {
                        //	SerialTNC.println("}" + raw);
                        //}
                    }
                    strncpy(&text[0], aprs.msg.body, aprs.msg.body_len);
                    display.setCursor(2, 40);
                    display.print("Msg: ");
                    display.println(text);

                    display.display();
                }
                return;
            }
            display.setFont();
            display.drawFastHLine(0, 16, 128, WHITE);
            display.drawFastVLine(48, 16, 48, WHITE);
            x = 8;

            if (aprs.srcname_len > 0)
            {
                x += 9;
                display.fillRoundRect(51, 16, 77, 9, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(53, x);
                display.print("By " + src_call);
                display.setTextColor(WHITE);
                // x += 9;
            }
            if (aprs.packettype & T_WX)
            {
                // Serial.println("WX Display");
                if (aprs.wx_report.flags & W_TEMP)
                {
                    display.setCursor(58, x += 10);
                    display.drawYBitmap(51, x, &Temperature_Symbol[0], 5, 8, WHITE);
                    display.printf("%.1fC", aprs.wx_report.temp);
                }
                if (aprs.wx_report.flags & W_HUM)
                {
                    display.setCursor(102, x);
                    display.drawYBitmap(95, x, &Humidity_Symbol[0], 5, 8, WHITE);
                    display.printf("%d%%", aprs.wx_report.humidity);
                }
                if (aprs.wx_report.flags & W_BAR)
                {
                    display.setCursor(58, x += 9);
                    display.drawYBitmap(51, x, &Pressure_Symbol[0], 5, 8, WHITE);
                    display.printf("%.1fhPa", aprs.wx_report.pressure);
                }
                if (aprs.wx_report.flags & W_R24H)
                {
                    // if (aprs.wx_report.rain_1h > 0) {
                    display.setCursor(58, x += 9);
                    display.drawYBitmap(51, x, &Rain_Symbol[0], 5, 8, WHITE);
                    display.printf("%.1fmm.", aprs.wx_report.rain_24h);
                    //}
                }
                if (aprs.wx_report.flags & W_PAR)
                {
                    // if (aprs.wx_report.luminosity > 10) {
                    display.setCursor(51, x += 9);
                    display.printf("%c", 0x0f);
                    display.setCursor(58, x);
                    display.printf("%dW/m", aprs.wx_report.luminosity);
                    if (aprs.wx_report.flags & W_UV)
                    {
                        display.printf(" UV%d", aprs.wx_report.uv);
                    }
                    //}
                }
                if (aprs.wx_report.flags & W_WS)
                {
                    display.setCursor(58, x += 9);
                    display.drawYBitmap(51, x, &Wind_Symbol[0], 5, 8, WHITE);
                    // int dirIdx=map(aprs.wx_report.wind_dir, -180, 180, 0, 8); ((angle+22)/45)%8]
                    int dirIdx = ((aprs.wx_report.wind_dir + 22) / 45) % 8;
                    if (dirIdx > 8)
                        dirIdx = 8;
                    display.printf("%.1fkPh(%s)", aprs.wx_report.wind_speed, directions[dirIdx]);
                }
                // Serial.printf("%.1fkPh(%d)", aprs.wx_report.wind_speed, aprs.wx_report.wind_dir);
                if (aprs.flags & F_HASPOS)
                {
                    // Serial.println("POS Display");
                    double lat, lon;
                    if ((config.mygps == true) && gps.location.isValid())
                    {
                        lat = gps.location.lat();
                        lon = gps.location.lng();
                    }
                    else
                    {
                        lat = config.gps_lat;
                        lon = config.gps_lon;
                    }
                    double dtmp = aprsParse.direction(lon, lat, aprs.lng, aprs.lat);
                    double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                    // if (config.h_up == true) {
                    // 	//double course = gps.course.deg();
                    // 	double course = SB_HEADING;
                    // 	if (dtmp >= course) {
                    // 		dtmp -= course;
                    // 	}
                    // 	else {
                    // 		double diff = dtmp - course;
                    // 		dtmp = diff + 360.0F;
                    // 	}
                    // 	compass_label(25, 37, 15, course, WHITE);
                    // 	display.setCursor(0, 17);
                    // 	display.printf("H");
                    // }
                    // else {
                    compass_label(25, 37, 15, 0.0F, WHITE);
                    //}
                    // compass_label(25, 37, 15, 0.0F, WHITE);
                    compass_arrow(25, 37, 12, dtmp, WHITE);
                    display.drawFastHLine(1, 63, 45, WHITE);
                    display.drawFastVLine(1, 58, 5, WHITE);
                    display.drawFastVLine(46, 58, 5, WHITE);
                    display.setCursor(4, 55);
                    if (dist > 999)
                        display.printf("%.fKm", dist);
                    else
                        display.printf("%.1fKm", dist);
                }
                else
                {
                    display.setCursor(20, 30);
                    display.printf("NO\nPOSITION");
                }
            }
            else if (aprs.flags & F_HASPOS)
            {
                // display.setCursor(50, x += 10);
                // display.printf("LAT %.5f\n", aprs.lat);
                // display.setCursor(51, x+=9);
                // display.printf("LNG %.4f\n", aprs.lng);
                String str;
                int l = 0;
                display.setCursor(50, x += 10);
                display.print("LAT:");
                str = String(aprs.lat, 5);
                l = str.length() * 6;
                display.setCursor(128 - l, x);
                display.print(str);

                display.setCursor(50, x += 9);
                display.print("LON:");
                str = String(aprs.lng, 5);
                l = str.length() * 6;
                display.setCursor(128 - l, x);
                display.print(str);

                double lat, lon;
                if ((config.mygps == true) && gps.location.isValid())
                {
                    lat = gps.location.lat();
                    lon = gps.location.lng();
                }
                else
                {
                    lat = config.gps_lat;
                    lon = config.gps_lon;
                }
                double dtmp = aprsParse.direction(lon, lat, aprs.lng, aprs.lat);
                double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                // if (config.h_up == true) {
                // 	//double course = gps.course.deg();
                // 	double course = SB_HEADING;
                // 	if (dtmp>=course) {
                // 		dtmp -= course;
                // 	}
                // 	else {
                // 		double diff = dtmp-course;
                // 		dtmp = diff+360.0F;
                // 	}
                // 	compass_label(25, 37, 15, course, WHITE);
                // 	display.setCursor(0, 17);
                // 	display.printf("H");
                // }
                // else {
                compass_label(25, 37, 15, 0.0F, WHITE);
                //}
                compass_arrow(25, 37, 12, dtmp, WHITE);
                display.drawFastHLine(1, 55, 45, WHITE);
                display.drawFastVLine(1, 55, 5, WHITE);
                display.drawFastVLine(46, 55, 5, WHITE);
                display.setCursor(4, 57);
                if (dist > 999)
                    display.printf("%.fKm", dist);
                else
                    display.printf("%.1fKm", dist);
                if (aprs.flags & F_CSRSPD)
                {
                    display.setCursor(51, x += 9);
                    // display.printf("SPD %d/", aprs.course);
                    // display.setCursor(50, x += 9);
                    display.printf("SPD %.1fkPh\n", aprs.speed);
                    int dirIdx = ((aprs.course + 22) / 45) % 8;
                    if (dirIdx > 8)
                        dirIdx = 8;
                    display.setCursor(51, x += 9);
                    display.printf("CSD %d(%s)", aprs.course, directions[dirIdx]);
                }
                if (aprs.flags & F_ALT)
                {
                    display.setCursor(51, x += 9);
                    display.printf("ALT %.1fM\n", aprs.altitude);
                }
                if (aprs.flags & F_PHG)
                {
                    int power, height, gain;
                    unsigned char tmp;
                    power = (int)aprs.phg[0] - 0x30;
                    power *= power;
                    height = (int)aprs.phg[1] - 0x30;
                    height = 10 << (height + 1);
                    height = height / 3.2808;
                    gain = (int)aprs.phg[2] - 0x30;
                    display.setCursor(51, x += 9);
                    display.printf("PHG %dM.\n", height);
                    display.setCursor(51, x += 9);
                    display.printf("PWR %dWatt\n", power);
                    display.setCursor(51, x += 9);
                    display.printf("ANT %ddBi\n", gain);
                }
                if (aprs.flags & F_RNG)
                {
                    display.setCursor(51, x += 9);
                    display.printf("RNG %dKm\n", aprs.radio_range);
                }
                /*if (aprs.comment_len > 0) {
                    display.setCursor(0, 56);
                    display.print(aprs.comment);
                }*/
            }
            display.display();
        }
    }
}

void statisticsDisp()
{

    // uint8 wifi = 0, i;
    int x;
    String str;
    // display.fillRect(0, 16, 128, 10, WHITE);
    // display.drawLine(0, 16, 0, 63, WHITE);
    // display.drawLine(127, 16, 127, 63, WHITE);
    // display.drawLine(0, 63, 127, 63, WHITE);
    // display.fillRect(1, 25, 126, 38, BLACK);
    // display.setTextColor(BLACK);
    // display.setCursor(30, 17);
    // display.print("STATISTICS");
    // display.setCursor(108, 17);
    // display.print("1/5");
    // display.setTextColor(WHITE);

    display.fillRect(0, 0, 128, 15, WHITE);
    display.drawRect(0, 16, 128, 48, WHITE);
    display.fillRect(1, 17, 126, 46, BLACK);

    display.setCursor(5, 7);
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLACK);
    display.print("STATISTIC");
    display.setFont();
    display.setCursor(108, 7);
    display.print("1/4");
    display.setTextColor(WHITE);

    display.setCursor(3, 18);
    display.print("ALL DATA");
    str = String(status.allCount, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 18);
    display.print(str);

    display.setCursor(3, 26);
    display.print("DIGI RPT");
    str = String(status.digiCount, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 26);
    display.print(str);

    display.setCursor(3, 35);
    display.print("RF->INET");
    str = String(status.rf2inet, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 35);
    display.print(str);

    display.setCursor(3, 44);
    display.print("INET->RF");
    str = String(status.inet2rf, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 44);
    display.print(str);

    display.setCursor(3, 53);
    display.print("ERROR/DROP");
    str = String(status.errorCount + status.dropCount, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 53);
    display.print(str);

    display.display();
}

void pkgLastDisp()
{

    uint8_t k = 0;
    int i;
    // char list[4];
    int x, y;
    String str;
    // String times;
    // pkgListType *ptr[100];

    display.fillRect(0, 0, 128, 15, WHITE);
    display.drawRect(0, 16, 128, 48, WHITE);
    display.fillRect(1, 17, 126, 46, BLACK);

    display.setCursor(15, 7);
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLACK);
    display.print("STATION");
    display.setFont();
    display.setCursor(108, 7);
    display.print("2/4");
    display.setTextColor(WHITE);

    // display.fillRect(0, 16, 128, 10, WHITE);
    // display.drawLine(0, 16, 0, 63, WHITE);
    // display.drawLine(127, 16, 127, 63, WHITE);
    // display.drawLine(0, 63, 127, 63, WHITE);
    // display.fillRect(1, 25, 126, 38, BLACK);
    // display.setTextColor(BLACK);
    // display.setCursor(27, 17);
    // display.print("LAST STATIONS");
    // display.setCursor(108, 17);
    // display.print("2/5");
    // display.setTextColor(WHITE);

    sort(pkgList, PKGLISTSIZE);
    k = 0;
    for (i = 0; i < PKGLISTSIZE; i++)
    {
        if (pkgList[i].time > 0)
        {
            y = 18 + (k * 9);
            // display.drawBitmap(3, y, &SYMBOL[0][0], 11, 6, WHITE);
            display.fillRoundRect(2, y, 7, 8, 2, WHITE);
            display.setCursor(3, y);
            pkgList[i].calsign[10] = 0;
            display.setTextColor(BLACK);
            switch (pkgList[i].type)
            {
            case PKG_OBJECT:
                display.print("O");
                break;
            case PKG_ITEM:
                display.print("I");
                break;
            case PKG_MESSAGE:
                display.print("M");
                break;
            case PKG_WX:
                display.print("W");
                break;
            case PKG_TELEMETRY:
                display.print("T");
                break;
            case PKG_QUERY:
                display.print("Q");
                break;
            case PKG_STATUS:
                display.print("S");
                break;
            default:
                display.print("*");
                break;
            }
            display.setTextColor(WHITE);
            display.setCursor(10, y);
            display.print(pkgList[i].calsign);
            display.setCursor(126 - 48, y);
            // display.printf("%02d:%02d:%02d", hour(pkgList[i].time), minute(pkgList[i].time), second(pkgList[i].time));

            // time_t tm = pkgList[i].time;
            struct tm tmstruct;
            localtime_r(&pkgList[i].time, &tmstruct);
            String str = String(tmstruct.tm_hour, DEC) + ":" + String(tmstruct.tm_min, DEC) + ":" + String(tmstruct.tm_sec, DEC);
            display.print(str);
            // str = String(hour(pkgList[i].time),DEC) + ":" + String(minute(pkgList[i].time), DEC) + ":" + String(second(pkgList[i].time), DEC);
            ////str = String(pkgList[pkgLast_array[i]].time, DEC);
            // x = str.length() * 6;
            // display.setCursor(126 - x, y);
            // display.print(str);
            k++;
            if (k >= 5)
                break;
        }
    }
    display.display();
}

void pkgCountDisp()
{

    // uint8 wifi = 0, k = 0, l;
    uint k = 0;
    int i;
    // char list[4];
    int x, y;
    String str;
    // String times;
    // pkgListType *ptr[100];

    display.fillRect(0, 0, 128, 15, WHITE);
    display.drawRect(0, 16, 128, 48, WHITE);
    display.fillRect(1, 17, 126, 46, BLACK);

    display.setCursor(20, 7);
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLACK);
    display.print("TOP PKG");
    display.setFont();
    display.setCursor(108, 7);
    display.print("3/4");
    display.setTextColor(WHITE);

    // display.setCursor(3, 18);

    // display.fillRect(0, 16, 128, 10, WHITE);
    // display.drawLine(0, 16, 0, 63, WHITE);
    // display.drawLine(127, 16, 127, 63, WHITE);
    // display.drawLine(0, 63, 127, 63, WHITE);
    // display.fillRect(1, 25, 126, 38, BLACK);
    // display.setTextColor(BLACK);
    // display.setCursor(30, 17);
    // display.print("TOP PACKAGE");
    // display.setCursor(108, 17);
    // display.print("3/5");
    // display.setTextColor(WHITE);

    sortPkgDesc(pkgList, PKGLISTSIZE);
    k = 0;
    for (i = 0; i < PKGLISTSIZE; i++)
    {
        if (pkgList[i].time > 0)
        {
            y = 18 + (k * 9);
            // display.drawBitmapV(2, y-1, &SYMBOL[pkgList[i].symbol][0], 11, 8, WHITE);
            pkgList[i].calsign[10] = 0;
            display.fillRoundRect(2, y, 7, 8, 2, WHITE);
            display.setCursor(3, y);
            pkgList[i].calsign[10] = 0;
            display.setTextColor(BLACK);
            switch (pkgList[i].type)
            {
            case PKG_OBJECT:
                display.print("O");
                break;
            case PKG_ITEM:
                display.print("I");
                break;
            case PKG_MESSAGE:
                display.print("M");
                break;
            case PKG_WX:
                display.print("W");
                break;
            case PKG_TELEMETRY:
                display.print("T");
                break;
            case PKG_QUERY:
                display.print("Q");
                break;
            case PKG_STATUS:
                display.print("S");
                break;
            default:
                display.print("*");
                break;
            }
            display.setTextColor(WHITE);
            display.setCursor(10, y);
            display.print(pkgList[i].calsign);
            str = String(pkgList[i].pkg, DEC);
            x = str.length() * 6;
            display.setCursor(126 - x, y);
            display.print(str);
            k++;
            if (k >= 5)
                break;
        }
    }
    display.display();
}

void systemDisp()
{

    // uint8 wifi = 0, k = 0, l;
    // char i;
    // char list[4];
    int x;
    String str;
    time_t upTime = now() - systemUptime;
    // String times;
    // pkgListType *ptr[100];

    // display.fillRect(0, 16, 128, 10, WHITE);
    // display.drawLine(0, 16, 0, 63, WHITE);
    display.fillRect(0, 0, 128, 15, WHITE);
    // display.drawLine(0, 16, 0, 63, WHITE);

    // display.drawLine(127, 16, 127, 63, WHITE);
    // display.drawLine(0, 63, 127, 63, WHITE);
    // display.fillRect(1, 25, 126, 38, BLACK);
    display.drawRect(0, 16, 128, 48, WHITE);
    display.fillRect(1, 17, 126, 46, BLACK);
    // display.fillRoundRect(1, 17, 126, 46, 2, WHITE);

    display.setCursor(20, 7);
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLACK);
    display.print("SYSTEM");
    display.setFont();
    display.setCursor(108, 7);
    display.print("4/4");
    display.setTextColor(WHITE);

    display.setCursor(3, 18);
    // display.print("HMEM:");
    display.print("MAC");
    // str = String(ESP.getFreeHeap(), DEC)+"Byte";
    str = String(WiFi.macAddress());
    x = str.length() * 6;
    display.setCursor(126 - x, 18);
    display.print(str);

    display.setCursor(3, 26);
    display.print("IP:");
    str = String(WiFi.localIP().toString());
    x = str.length() * 6;
    display.setCursor(126 - x, 26);
    display.print(str);

    display.setCursor(3, 35);
    display.print("UPTIME:");
    str = String(day(upTime) - 1, DEC) + "D " + String(hour(upTime), DEC) + ":" + String(minute(upTime), DEC) + ":" + String(second(upTime), DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 35);
    display.print(str);

    display.setCursor(3, 44);
    display.print("WiFi RSSI:");
    str = String(WiFi.RSSI()) + "dBm";
    // str = String(str_status[WiFi.status()]);
    x = str.length() * 6;
    display.setCursor(126 - x, 44);
    display.print(str);

    display.setCursor(3, 53);
    display.print("Firmware:");
    str = "V" + String(VERSION);
    x = str.length() * 6;
    display.setCursor(126 - x, 53);
    display.print(str);

    display.display();
}
