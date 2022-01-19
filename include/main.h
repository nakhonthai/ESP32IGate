/*
 Name:		ESP32 APRS Internet Gateway
 Created:	1-Nov-2021 14:27:23
 Author:	HS5TQA/Atten
 Support IS: host:aprs.dprns.com port:14580
 Support IS monitor: http://aprs.dprns.com:14501
 Support in LINE Group APRS Only
*/

#ifndef MAIN_H
#define MAIN_H

#define VERSION "0.1"

#define DEBUG
//#define DEBUG_IS

//#define SDCARD

#define WIFI_OFF_FIX	0
#define WIFI_AP_FIX		1
#define WIFI_STA_FIX	2
#define WIFI_AP_STA_FIX	3

#define	IMPLEMENTATION	FIFO

#define TZ              0       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

#define FORMAT_SPIFFS_IF_FAILED true

#define PKGLISTSIZE 10

const int timeZone = 7;  // Bangkok

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include "soc/rtc_wdt.h"

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

typedef struct Config_Struct {
	bool synctime;
	bool aprs;
	bool wifi_client;
	bool wifi;
	char wifi_mode; //WIFI_AP,WIFI_STA,WIFI_AP_STA,WIFI_OFF
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
	char wifi_ssid[20];
	char wifi_pass[15];
	char wifi_ap_ssid[20];
	char wifi_ap_pass[15];
	char tnc_path[50];
	char tnc_btext[50];
	char tnc_comment[50];
	char aprs_object[10];
	char mqtt_host[20];
	char mqtt_user[10];
	char mqtt_pass[10];
}Configuration;

typedef struct digiTLM_struct {
	uint16_t Sequence;
	unsigned long ParmTimeout;
	unsigned long TeleTimeout;
	uint8_t RF2INET;
	uint8_t INET2RF;
	uint8_t RX;
	uint8_t TX;
	uint8_t DROP;
}digiTLMType;

typedef struct pkgListStruct {
	time_t time;
	char calsign[11];
	char ssid[5];
	unsigned int pkg;
	bool type;
	uint8_t symbol;
}pkgListType;

typedef struct statisticStruct {
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
}statusType;

const char PARM[] = { "PARM.RF->INET,INET->RF,RxPkts,TxPkts,IGateDropRx" };
const char UNIT[] = { "UNIT.Pkts,Pkts,Pkts,Pkts,Pkts" };
const char EQNS[] = { "EQNS.0,1,0,0,1,0,0,1,0,0,1,0,0,1,0" };

void saveEEPROM();
void defaultConfig();
String getValue(String data, char separator, int index);
boolean isValidNumber(String str);
void taskAPRS(void * pvParameters);
void taskNetwork(void * pvParameters);
void sort(pkgListType a[], int size);
void sortPkgDesc(pkgListType a[], int size);
int processPacket(String &tnc2);

#endif