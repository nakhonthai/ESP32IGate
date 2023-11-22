/*
 Name:		ESP32 APRS Internet Gateway
 Created:	1-Nov-2021 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#ifndef MAIN_H
#define MAIN_H

#define VERSION "1.0"
#define VERSION_BUILD ' '

// #define DEBUG
// #define DEBUG_IS
#define WX

#define OLED
//  #define SDCARD
//  #define BLUETOOTH

#define WIFI_OFF_FIX 0
#define WIFI_AP_FIX 1
#define WIFI_STA_FIX 2
#define WIFI_AP_STA_FIX 3

#define IMPLEMENTATION FIFO

#define TZ 0	 // (utc+) TZ in hours
#define DST_MN 0 // use 60mn for summer time in some countries
#define TZ_MN ((TZ) * 60)
#define TZ_SEC ((TZ) * 3600)
#define DST_SEC ((DST_MN) * 60)

#define FORMAT_SPIFFS_IF_FAILED true

#ifdef BOARD_HAS_PSRAM
#define TLMLISTSIZE 10
#define PKGLISTSIZE 100
#define PKGTXSIZE 100
#else
#define TLMLISTSIZE 10
#define PKGLISTSIZE 30
#define PKGTXSIZE 5
#endif

#define FILTER_ALL 0				// Packet is disable all packet
#define FILTER_OBJECT (1 << 0)		// packet is an object
#define FILTER_ITEM (1 << 1)		// packet is an item
#define FILTER_MESSAGE (1 << 2)		// packet is a message
#define FILTER_WX (1 << 3)			// packet is WX data
#define FILTER_TELEMETRY (1 << 4)	// packet is telemetry
#define FILTER_QUERY (1 << 5)		// packet is a query
#define FILTER_STATUS (1 << 6)		// packet is status
#define FILTER_POSITION (1 << 7)	// packet is postion
#define FILTER_BUOY (1 << 8)		// packet is buoy
#define FILTER_MICE (1 << 9)		// packet is MIC-E
#define FILTER_THIRDPARTY (1 << 10) // packet is 3rd-party packet from INET2RF

#define RF_NONE 0
#define RF_SA868_VHF 1 // G-NiceRF SA818,SA868 VHF band 134~174 MHz
#define RF_SA868_UHF 2 // G-NiceRF SA818,SA868 UHF band 400~470 MHz
#define RF_SA868_350 3 // G-NiceRF SA818,SA868 350 band frequency：320-400MHz
#define RF_SR_1WV 4	   // SUNRISE SR110V,FRS-1WV VHF band 136~174 MHz
#define RF_SR_1WU 5	   // SUNRISE SR110U,FRS-1WU UHF band 400~470 MHz
#define RF_SR_1W350 6  // SUNRISE SR350P 350 band frequency：350-390MHz
#define RF_SR_2WVS 7   // SUNRISE SR120V,SR_2WVS VHF band 136~174 MHz
#define RF_SR_2WUS 8   // SUNRISE SR120U,SR_2WUS UHF band 400~470 MHz

#define TXCH_TCP 0
#define TXCH_RF 1
#define TXCH_DIGI 2
#define TXCH_3PTY 3

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include "soc/rtc_wdt.h"
#include <AX25.h>
#include "weather.h"

#include "HardwareSerial.h"
#include "EEPROM.h"
typedef struct wifi_struct
{
	bool enable;
	char wifi_ssid[32];
	char wifi_pass[63];
} wifiSTA;

typedef struct Config_Struct
{
	float timeZone;
	bool synctime;
	bool title;

	// WiFi/BT/RF
	char wifi_mode; // WIFI_AP,WIFI_STA,WIFI_AP_STA,WIFI_OFF
	char wifi_power;
	//--WiFi Client
	wifiSTA wifi_sta[5];
	// bool wifi_client;
	// char wifi_ssid[32];
	// char wifi_pass[63];
	//--WiFi AP
	// bool wifi_ap;
	char wifi_ap_ch;
	char wifi_ap_ssid[32];
	char wifi_ap_pass[63];

	//--Blue Tooth
	bool bt_slave;
	bool bt_master;
	char bt_mode;
	char bt_uuid[37];
	char bt_uuid_rx[37];
	char bt_uuid_tx[37];
	char bt_name[20];
	uint32_t bt_pin;
	char bt_power;

	//--RF Module
	bool rf_en;
	uint8_t rf_type;
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
	uint8_t mic;

	// IGATE
	bool igate_en;
	bool rf2inet;
	bool inet2rf;
	bool igate_loc2rf;
	bool igate_loc2inet;
	uint16_t rf2inetFilter;
	uint16_t inet2rfFilter;
	//--APRS-IS
	uint8_t aprs_ssid;
	uint16_t aprs_port;
	char aprs_mycall[10];
	char aprs_host[20];
	char aprs_passcode[6];
	char aprs_moniCall[10];
	char aprs_filter[30];
	//--Position
	bool igate_bcn;
	bool igate_gps;
	bool igate_timestamp;
	float igate_lat;
	float igate_lon;
	float igate_alt;
	uint16_t igate_interval;
	char igate_symbol[3] = "N&";
	char igate_object[10];
	char igate_phg[8];
	char igate_path[72];
	char igate_comment[50];
	//--Filter

	// DIGI REPEATER
	bool digi_en;
	bool digi_loc2rf;
	bool digi_loc2inet;
	bool digi_timestamp;
	uint8_t digi_ssid;
	char digi_mycall[10];
	char digi_path[72];
	uint16_t digi_delay; // ms
	uint16_t digiFilter;
	//--Position
	bool digi_bcn;
	bool digi_compress = false;
	bool digi_altitude = false;
	bool digi_gps;
	float digi_lat;
	float digi_lon;
	float digi_alt;
	uint16_t digi_interval;
	char digi_symbol[3] = "N&";
	char digi_phg[8];
	char digi_comment[50];

	// WX
	bool wx_en;
	bool wx_2rf;
	bool wx_2inet;
	bool wx_timestamp;
	uint8_t wx_ssid;
	char wx_mycall[10];
	char wx_path[72];
	bool wx_gps;
	float wx_lat;
	float wx_lon;
	float wx_alt;
	uint16_t wx_interval;
	char wx_object[10];
	char wx_comment[50];
	uint8_t wx_mode;
	uint32_t wx_flage;

	// OLED DISPLAY
	bool oled_enable;
	int oled_timeout;
	unsigned char dim;
	unsigned char contrast;
	unsigned char startup;

	// Display
	unsigned int dispDelay;
	unsigned int filterDistant;
	bool h_up = true;
	bool tx_display = true;
	bool rx_display = true;
	uint16_t dispFilter;
	bool dispRF;
	bool dispINET;

	// AFSK,TNC
	bool audio_hpf;
	bool audio_bpf;
	uint8_t preamble;
	uint16_t tx_timeslot;
	char ntp_host[20];

	// VPN wiregurad
	bool vpn;
	bool modem;
	uint16_t wg_port;
	char wg_peer_address[16];
	char wg_local_address[16];
	char wg_netmask_address[16];
	char wg_gw_address[16];
	char wg_public_key[45];
	char wg_private_key[45];

	char http_username[32];
	char http_password[64];

	char path[4][15];

	// GNSS
	bool gnss_enable;
	unsigned long gnss_baudrate;
	int8_t gnss_rx_gpio;
	int8_t gnss_tx_gpio;
	int8_t gnss_pps_gpio;

	unsigned long rf_baudrate;
	int8_t rf_tx_gpio = 13;
	int8_t rf_rx_gpio = 14;
	int8_t rf_sql_gpio = 33;
	int8_t rf_pd_gpio = 27;
	int8_t rf_pwr_gpio = 12;
	int8_t rf_ptt_gpio = 32;
	bool rf_sql_active = 0;
	bool rf_pd_active = 1;
	bool rf_pwr_active = 0;
	bool rf_ptt_active = 0;
	uint8_t adc_atten = 0;
	uint16_t adc_dc_offset;	

	bool i2c_enable;
	int8_t i2c_sda_pin = -1;
	int8_t i2c_sck_pin = -1;
	uint32_t i2c_freq = 400000;
	bool i2c1_enable;
	int8_t i2c1_sda_pin = -1;
	int8_t i2c1_sck_pin = -1;
	uint32_t i2c1_freq = 100000;

	bool onewire_enable = false;
	int8_t onewire_gpio = -1;

	bool modbus_enable = false;
	unsigned long modbus_baudrate;
	int8_t modbus_tx_gpio = -1;
	int8_t modbus_rx_gpio = -1;
	int8_t modbus_de_gpio = -1;

	bool counter0_enable = false;
	bool counter0_active = 0;
	int8_t counter0_gpio = -1;

	bool counter1_enable = false;
	bool counter1_active = 0;
	int8_t counter1_gpio = -1;

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

typedef struct
{
	time_t time;
	char calsign[11];
	char ssid[5];
	bool channel;
	unsigned int pkg;
	uint16_t type;
	uint8_t symbol;
	int16_t audio_level;
	char raw[300];
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
	uint32_t txCount;
	uint32_t rxCount;
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

typedef struct txDispStruct
{
	uint8_t tx_ch;
	char name[12];
	char info[50];
} txDisp;

#ifdef OLED
const unsigned char LOGO[] PROGMEM =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
		0xC0, 0x00, 0x00, 0xC0, 0xC0, 0xE0, 0xE0, 0xF0, 0xF8, 0xF8,
		0xFC, 0xFC, 0xFE, 0x7E, 0x7F, 0x7C, 0x70, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xE0, 0xE0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF8, 0xF8, 0xF8, 0xFC,
		0x7C, 0x3C, 0x3C, 0x18, 0x83, 0x9F, 0x9F, 0x9F, 0x8F, 0x8F,
		0xCC, 0x41, 0x63, 0x13, 0x01, 0x81, 0xE1, 0x21, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
		0x07, 0x07, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x08, 0x08, 0x08, 0x09, 0x19, 0x19, 0x09, 0x08, 0x08, 0x0C,
		0x04, 0x86, 0x83, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x8C, 0x80, 0x84, 0xC2, 0x80, 0x07, 0x08,
		0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xF8, 0x3E, 0x43, 0x7D,
		0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E,
		0x7E, 0x7E, 0x7E, 0x7E, 0x79, 0x07, 0x7C, 0xF0, 0xC0, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xC3, 0xFF,
		0xFF, 0xFF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xFF, 0xFF, 0xFF,
		0xE7, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x03, 0x1F, 0x3F, 0x3F, 0x3F, 0x1F, 0x03, 0x03, 0x03, 0x03,
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x1F,
		0x3F, 0x3F, 0x3F, 0x1F, 0x03, 0x00, 0x00, 0x00};
#endif

const char PARM[] = {"PARM.RF->INET,INET->RF,DigiRpt,TX2RF,DropRx"};
const char UNIT[] = {"UNIT.Pkts,Pkts,Pkts,Pkts,Pkts"};
const char EQNS[] = {"EQNS.0,1,0,0,1,0,0,1,0,0,1,0,0,1,0"};

const float ctcss[] = {0, 67, 71.9, 74.4, 77, 79.7, 82.5, 85.4, 88.5, 91.5, 94.8, 97.4, 100, 103.5, 107.2, 110.9, 114.8, 118.8, 123, 127.3, 131.8, 136.5, 141.3, 146.2, 151.4, 156.7, 162.2, 167.9, 173.8, 179.9, 186.2, 192.8, 203.5, 210.7, 218.1, 225.7, 233.6, 241.8, 250.3};
const float wifiPwr[12][2] = {{-4, -1}, {8, 2}, {20, 5}, {28, 7}, {34, 8.5}, {44, 11}, {52, 13}, {60, 15}, {68, 17}, {74, 18.5}, {76, 19}, {78, 19.5}};
const char RF_TYPE[9][11] = {"NONE", "SA868_VHF", "SA868_UHF", "SA868_350", "SR110V_VHF", "SR110U_UHF", "SR350P", "SR120V_VHF", "SR120U_UHF"};
const unsigned long baudrate[] = {2400, 4800, 9600, 19200, 2880, 38400, 57600, 76800, 115200, 230400, 460800, 576000, 921600};

uint8_t checkSum(uint8_t *ptr, size_t count);
void saveEEPROM();
void defaultConfig();
String getValue(String data, char separator, int index);
boolean isValidNumber(String str);
void taskGPS(void *pvParameters);
void taskAPRS(void *pvParameters);
void taskNetwork(void *pvParameters);
void taskTNC(void *pvParameters);
void sort(pkgListType a[], int size);
void sortPkgDesc(pkgListType a[], int size);
int processPacket(String &tnc2);
int digiProcess(AX25Msg &Packet);
void printTime();
int popTNC2Raw(int &ret);
int pushTNC2Raw(int raw);
int pkgListUpdate(char *call, char *raw, uint16_t type);
pkgListType getPkgList(int idx);
String myBeacon(String Path);
int tlmList_Find(char *call);
int tlmListOld();
TelemetryType getTlmList(int idx);
void powerSave();
void powerWakeup();
bool powerStatus();
int packet2Raw(String &tnc2, AX25Msg &Packet);
bool waitResponse(String &data, String rsp = "\r\n", uint32_t timeout = 1000);
String sendIsAckMsg(String toCallSign, char *msgId);
bool pkgTxPush(const char *info, size_t len, int dly);
bool pkgTxUpdate(const char *info, int delay);
void dispWindow(String line, uint8_t mode, bool filter);
void systemDisp();
void pkgCountDisp();
void pkgLastDisp();
void statisticsDisp();
String getTimeStamp();
void DD_DDDDDtoDDMMSS(float DD_DDDDD, int *DD, int *MM, int *SS);

#endif