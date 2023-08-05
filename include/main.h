/*
 Name:		ESP32 APRS Internet Gateway
 Created:	1-Nov-2021 14:27:23
 Author:	HS5TQA/Atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
 Support in LINE Group APRS Only
*/

#ifndef MAIN_H
#define MAIN_H

#define VERSION "0.9"

//#define DEBUG
//#define DEBUG_IS
#define WX

#define OLED
//#define SDCARD
//#define SA818
//#define SR_FRS
#define SR_RF_2WVS

#ifdef SR_FRS
#ifndef SA818
#define SA818
#endif
#endif

#ifdef SR_RF_2WVS
#ifndef SA818
#define SA818
#endif
#endif

#define WIFI_OFF_FIX 0
#define WIFI_AP_FIX 1
#define WIFI_STA_FIX 2
#define WIFI_AP_STA_FIX 3

#define IMPLEMENTATION FIFO

#define TZ 0	 // (utc+) TZ in hours
#define DST_MN 0 // use 60mn for summer time in some countries
#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)

#define FORMAT_SPIFFS_IF_FAILED true

#define PKGLISTSIZE 10
#define PKGTXSIZE 5

const int timeZone = 7; // Bangkok

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include "soc/rtc_wdt.h"
#include <AX25.h>

#include "HardwareSerial.h"
#include "EEPROM.h"

enum M17Flags
{
	DISCONNECTED = 1 << 0,
	CONNECTING = 1 << 1,
	M17_AUTH = 1 << 2,
	M17_CONF = 1 << 3,
	M17_OPTS = 1 << 4,
	CONNECTED_RW = 1 << 5,
	CONNECTED_RO = 1 << 6
};

typedef struct Config_Struct
{
	bool synctime;
	bool aprs;
	bool wifi_client;
	bool wifi;
	char wifi_mode; // WIFI_AP,WIFI_STA,WIFI_AP_STA,WIFI_OFF
	char wifi_ch;
	float gps_lat;
	float gps_lon;
	float gps_alt;
	uint8_t aprs_ssid;
	uint16_t aprs_port;
	uint8_t aprs_moniSSID;
	uint32_t api_id;
	uint16_t mqtt_port;
	bool tnc;
	bool rf2inet;
	bool inet2rf;
	bool tnc_digi = false;
	bool tnc_telemetry = false;
	int tnc_beacon = 0;
	int aprs_beacon;
	char aprs_table;
	char aprs_symbol;
	char aprs_mycall[10];
	char aprs_host[20];
	char aprs_passcode[10];
	char aprs_moniCall[10];
	char aprs_filter[30];
	char aprs_comment[50];
	char aprs_path[72];
	char wifi_ssid[32];
	char wifi_pass[63];
	char wifi_ap_ssid[32];
	char wifi_ap_pass[63];
	char tnc_path[50];
	char tnc_btext[50];
	char tnc_comment[50];
	char aprs_object[10];
	char mqtt_host[20];
	char mqtt_user[10];
	char mqtt_pass[10];
	char wifi_power;
	uint16_t tx_timeslot;
	uint16_t digi_delay;
	bool input_hpf;
#ifdef SA818
	float freq_rx;
	float freq_tx;
	int offset_rx;
	int offset_tx;
	int tone_rx;
	int tone_tx;
	uint8_t band;
	uint8_t sql_level;
	bool rf_power;
	uint8_t volume;
#endif
	bool vpn;
	bool modem;
	uint16_t wg_port;
	char wg_peer_address[16];
	char wg_local_address[16];
	char wg_netmask_address[16];
	char wg_gw_address[16];
	char wg_public_key[45];
	char wg_private_key[45];
	int8_t timeZone;
	bool oled_enable;
	int oled_timeout;
	unsigned int dispDelay;
	bool dispTNC;
	bool dispINET;
	bool filterMessage;
	bool filterStatus;
	bool filterTelemetry;
	bool filterWeather;
	bool filterTracker;
	bool filterMove;
	bool filterPosition;
	unsigned int filterDistant;
	bool mygps;

} Configuration;

typedef struct igateTLM_struct
{
	uint16_t Sequence;
	unsigned long ParmTimeout;
	unsigned long TeleTimeout;
	uint8_t RF2INET;
	uint8_t INET2RF;
	uint8_t RX;
	uint8_t TX;
	uint8_t DROP;
} igateTLMType;

typedef struct pkgListStruct
{
	time_t time;
	char calsign[11];
	char ssid[5];
	unsigned int pkg;
	uint8_t type;
	uint8_t symbol;
} pkgListType;

typedef struct statisticStruct
{
	uint32_t allCount;
	uint32_t tncCount;
	uint32_t isCount;
	uint32_t locationCount;
	uint32_t wxCount;
	uint32_t digiCount;
	uint32_t errorCount;
	uint32_t dropCount;
	uint32_t rf2inet;
	uint32_t inet2rf;
} statusType;

typedef struct digiTLM_struct
{
	unsigned int Sequence;
	unsigned int ParmTimeout;
	unsigned int TeleTimeout;
	unsigned char RxPkts;
	unsigned char TxPkts;
	unsigned char DropRx;
	unsigned char ErPkts;
} digiTLMType;

#define TLMLISTSIZE 20

typedef struct Telemetry_struct
{
	time_t time;
	char callsign[10];
	char PARM[5][10];
	char UNIT[5][10];
	float VAL[5];
	float RAW[5];
	float EQNS[15];
	uint8_t BITS;
	uint8_t BITS_FLAG;
	bool EQNS_FLAG;
} TelemetryType;

typedef struct txQueue_struct
{
	bool Active;
	long timeStamp;
	int Delay;
	char Info[300];
} txQueueType; 

typedef struct Weather_Struct {
	unsigned long int timeStamp;
	unsigned int winddirection;
	float windspeed;
	float windgust;
	float temperature;
	float rain;
	float rain24hr;
	float humidity;
	float barometric;
	unsigned int solar;
	float soitemp;
	unsigned int soihum; //0-100%
	unsigned int water;   //centiment
	float vbat;
	float vsolar;
	float ibat;
	float pbat;
}WeatherData;

const char PARM[] = {"PARM.RF->INET,INET->RF,DigiRpt,TX2RF,DropRx"};
const char UNIT[] = {"UNIT.Pkts,Pkts,Pkts,Pkts,Pkts"};
const char EQNS[] = {"EQNS.0,1,0,0,1,0,0,1,0,0,1,0,0,1,0"};

const float ctcss[] = {0, 67, 71.9, 74.4, 77, 79.7, 82.5, 85.4, 88.5, 91.5, 94.8, 97.4, 100, 103.5, 107.2, 110.9, 114.8, 118.8, 123, 127.3, 131.8, 136.5, 141.3, 146.2, 151.4, 156.7, 162.2, 167.9, 173.8, 179.9, 186.2, 192.8, 203.5, 210.7, 218.1, 225.7, 233.6, 241.8, 250.3};
const float wifiPwr[12][2] = {{-4, -1}, {8, 2}, {20, 5}, {28, 7}, {34, 8.5}, {44, 11}, {52, 13}, {60, 15}, {68, 17}, {74, 18.5}, {76, 19}, {78, 19.5}};

void saveEEPROM();
void defaultConfig();
String getValue(String data, char separator, int index);
boolean isValidNumber(String str);
void taskAPRS(void *pvParameters);
void taskNetwork(void *pvParameters);
void sort(pkgListType a[], int size);
void sortPkgDesc(pkgListType a[], int size);
int processPacket(String &tnc2);
String send_fix_location();
int digiProcess(AX25Msg &Packet);
void printTime();
bool pkgTxUpdate(const char *info, int delay);
void dispWindow(String line, uint8_t mode, bool filter);
void systemDisp();
void pkgCountDisp();
void pkgLastDisp();
void statisticsDisp();

#endif