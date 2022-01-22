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

#include <WiFiClientSecure.h>

#include "AFSK.h"

//#define SA818
#define DEBUG_TNC

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

#ifdef SA818
#define VBAT_PIN 35
#define WIRE 4
#define POWER_PIN 12
#define PULLDOWN_PIN 27
#define SQL_PIN 33
HardwareSerial SerialRF(1);
#endif

time_t systemUptime = 0;
time_t wifiUptime = 0;

boolean KISS = false;
bool aprsUpdate = false;

boolean gotPacket = false;
AX25Msg incomingPacket;

cppQueue PacketBuffer(sizeof(AX25Msg), 5, IMPLEMENTATION); // Instantiate queue

statusType status;
digiTLMType digiTLM;

Configuration config;

pkgListType pkgList[PKGLISTSIZE];

TaskHandle_t taskNetworkHandle;
TaskHandle_t taskAPRSHandle;

HardwareSerial SerialTNC(2);

BluetoothSerial SerialBT;

// Set your Static IP address for wifi AP
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 254);
IPAddress subnet(255, 255, 255, 0);

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
    config.aprs_beacon = 30;
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
    sprintf(config.aprs_path, "WIDE1-1");
    sprintf(config.aprs_comment, "ESP32 Internet Gateway");
    sprintf(config.tnc_comment, "ESP32 Build in TNC");
    sprintf(config.aprs_filter, "g/HS*/E2*");
    sprintf(config.tnc_path, "WIDE1-1");
    saveEEPROM();
}

unsigned long NTP_Timeout;
unsigned long pingTimeout;

const char *lastTitle = "LAST HERT";

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

void pkgListUpdate(char *call, bool type)
{
    char i = pkgList_Find(call);
    if (i != 255)
    { // Found call in old pkg
        pkgList[(uint)i].time = now();
        pkgList[(uint)i].pkg++;
        pkgList[(uint)i].type = type;
        // Serial.print("Update: ");
    }
    else
    {
        i = pkgListOld();
        pkgList[(uint)i].time = now();
        pkgList[(uint)i].pkg = 1;
        pkgList[(uint)i].type = type;
        strcpy(pkgList[(uint)i].calsign, call);
        // strcpy(pkgList[(uint)i].ssid, &ssid[0]);
        pkgList[(uint)i].calsign[10] = 0;
        // Serial.print("NEW: ");
    }
}

uint8_t *packetData;
//ฟังชั่นถูกเรียกมาจาก ax25_decode
void aprs_msg_callback(struct AX25Msg *msg)
{
    AX25Msg pkg;

    memcpy(&pkg, msg, sizeof(AX25Msg));
    PacketBuffer.push(&pkg); //ใส่แพ็จเก็จจาก TNC ลงคิวบัพเฟอร์
}

void printTime()
{
    Serial.print("[");
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print(":");
    Serial.print(second());
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
void SA818_INIT(uint8_t HL)
{

    pinMode(0, INPUT);
    pinMode(POWER_PIN, OUTPUT);
    pinMode(PULLDOWN_PIN, OUTPUT);
    pinMode(SQL_PIN, INPUT_PULLUP);

    SerialRF.begin(9600, SERIAL_8N1, 14, 13);

    digitalWrite(PULLDOWN_PIN, HIGH);
    digitalWrite(POWER_PIN, LOW);
    delay(500);
    // AT+DMOSETGROUP=1,144.3900,144.3900,0,1,0,0
    SerialRF.println("AT+DMOSETGROUP=0,144.3900,144.3900,0,1,0,0");
    delay(10);
    SerialRF.println("AT+DMOAUTOPOWCONTR=1");
    delay(10);
    SerialRF.println("AT+DMOSETVOLUME=9");
    delay(10);
    SerialRF.println("AT+DMOSETVOX=0");
    delay(10);
    SerialRF.println("AT+DMOSETMIC=8,0,0");
    delay(100);
    // AFSK_TimerEnable(true);
    digitalWrite(POWER_PIN, HL);
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
            Serial.println("SA818 Activated");
#endif
        }
    }
    else
    {
        Serial.println("SA818 deActive");
        digitalWrite(POWER_PIN, LOW);
        digitalWrite(PULLDOWN_PIN, LOW);
        delay(500);
        SA818_INIT(LOW);
    }
    // SerialGPS.print("$PMTK161,0*28\r\n");
    // AFSK_TimerEnable(false);
}
#endif

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
        if (!aprsClient.connect(config.aprs_host, config.aprs_port)) //เชื่อมต่อกับเซิร์ฟเวอร์ TCP
        {
            // Serial.print(".");
            delay(100);
            cnt++;
            if (cnt > 50) //วนร้องขอการเชื่อมต่อ 50 ครั้ง ถ้าไม่ได้ให้รีเทิร์นฟังค์ชั่นเป็น False
                return false;
        }
        //ขอเชื่อมต่อกับ aprsc
        if (config.aprs_ssid == 0)
            login = "user " + String(config.aprs_mycall) + " pass " + String(config.aprs_passcode) + " vers ESP32IGate V" + String(VERSION) + " filter " + String(config.aprs_filter);
        else
            login = "user " + String(config.aprs_mycall) + "-" + String(config.aprs_ssid) + " pass " + String(config.aprs_passcode) + " vers ESP32IGate V" + String(VERSION) + " filter " + String(config.aprs_filter);
        aprsClient.println(login);
        // Serial.println(login);
        // Serial.println("Success");
        delay(500);
    }
    return true;
}

void setup()
{
    byte *ptr;
    pinMode(0, INPUT_PULLUP); // BOOT Button
    // Set up serial port
    Serial.begin(9600); // debug
    Serial.setRxBufferSize(256);
    SerialTNC.begin(9600, SERIAL_8N1, 16, 17);
    SerialTNC.setRxBufferSize(500);

    Serial.println();
    Serial.println("Start ESP32IGate V" + String(VERSION));
    Serial.println("Push BOOT for Factory Default config.");

    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println(F("failed to initialise EEPROM")); // delay(100000);
    }

    delay(3000);
    if (digitalRead(0) == LOW) {
		defaultConfig();
	    Serial.println("Manual Default configure!");
		delay(3000);
	}

    //ตรวจสอบคอนฟิกซ์ผิดพลาด
    ptr = (byte *)&config;
    EEPROM.readBytes(1, ptr, sizeof(Configuration));
    uint8_t chkSum = checkSum(ptr, sizeof(Configuration));
    Serial.printf("EEPROM Check %0Xh=%0Xh(%dByte)\n", EEPROM.read(0), chkSum, sizeof(Configuration));
    if (EEPROM.read(0) != chkSum)
    {
        Serial.println("Config EEPROM Error!");
        defaultConfig();
    }

#ifdef SA818
    SA818_INIT(LOW);
#endif

    enableLoopWDT();
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
        1);              /* Core where the task should run */

    // Task 2
    xTaskCreatePinnedToCore(
        taskNetwork,        /* Function to implement the task */
        "taskNetwork",      /* Name of the task */
        16384,              /* Stack size in words */
        NULL,               /* Task input parameter */
        1,                  /* Priority of the task */
        &taskNetworkHandle, /* Task handle. */
        0);                 /* Core where the task should run */
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

    *DD = (int)DD_DDDDD;                       //сделали из 37.45545 это 37 т.е. Градусы
    *MM = (int)((DD_DDDDD - *DD) * 60);        //получили минуты
    *SS = ((DD_DDDDD - *DD) * 60 - *MM) * 100; //получили секунды
}

String send_fix_location()
{
    String tnc2Raw = "";
    int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
    char strtmp[300], loc[30];
    memset(strtmp, 0, 300);
    DD_DDDDDtoDDMMSS(config.gps_lat, &lat_dd, &lat_mm, &lat_ss);
    DD_DDDDDtoDDMMSS(config.gps_lon, &lon_dd, &lon_mm, &lon_ss);
    sprintf(loc, "=%02d%02d.%02dN%c%03d%02d.%02dE%c", lat_dd, lat_mm, lat_ss, config.aprs_table, lon_dd, lon_mm, lon_ss, config.aprs_symbol);
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

int processPacket(String &tnc2)
{
    if (incomingPacket.len < 5)
        return 0;
    tnc2 = String(incomingPacket.src.call);
    if (incomingPacket.src.ssid > 0)
    {
        tnc2 += String(F("-"));
        tnc2 += String(incomingPacket.src.ssid);
    }
    tnc2 += String(F(">"));
    tnc2 += String(incomingPacket.dst.call);
    if (incomingPacket.dst.ssid > 0)
    {
        tnc2 += String(F("-"));
        tnc2 += String(incomingPacket.dst.ssid);
    }
    for (int i = 0; i < incomingPacket.rpt_count; i++)
    {
        tnc2 += String(",");
        char *rptcall = incomingPacket.rpt_list[i].call;
        tnc2 += String(rptcall);
        if (incomingPacket.rpt_list[i].ssid > 0)
        {
            tnc2 += String("-");
            char rssid[3];
            itoa(incomingPacket.rpt_list[i].ssid, rssid, 3);
            rssid[2] = 0;
            tnc2 += String(rssid);
        }
        if (incomingPacket.rpt_flags & (1 << i))
            tnc2 += "*";
    }
    tnc2 += String(F(":"));
    tnc2 += String((const char *)incomingPacket.info);
    tnc2 += String("\n");

#ifdef DEBUG_TNC
    Serial.printf("[%d] ", ++pkgTNC_count);
    Serial.print(tnc2);
#endif
    return tnc2.length();
}

long sendTimer = 0;
bool AFSKInitAct = false;
int btn_count = 0;
void loop()
{
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (AFSKInitAct == true)
    {
        AFSK_Poll();
    }
}

void taskAPRS(void *pvParameters)
{
    //	long start, stop;
    char *raw;
    char *str;
    Serial.println("Task APRS has been start");
    PacketBuffer.clean();

    APRS_init();
    APRS_setCallsign(config.aprs_mycall, config.aprs_ssid);
    APRS_setPath1(config.aprs_path, 1);
    APRS_setPreamble(300);
    APRS_setTail(0);
    sendTimer = millis() - (config.aprs_beacon * 1000) + 30000;
    AFSKInitAct = true;
    for (;;)
    {
        long now = millis();
        // wdtSensorTimer = now;
        time_t timeStamp;
        time(&timeStamp);
        vTaskDelay(10 / portTICK_PERIOD_MS);

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
                //Serial.printf("btn_count=%dms\n", btn_count * 10);
                if (btn_count > 1000) // Push BOOT 10sec to Factory Default
                {
                    defaultConfig();
                    Serial.println("SYSTEM REBOOT NOW!");
                    esp_restart();
                }
                else if (btn_count > 10) // Push BOOT >100mS to PTT Fix location
                {
                    if (config.tnc)
                    {
                        String tnc2Raw = send_fix_location();
                        APRS_sendTNC2Pkt(tnc2Raw); // Send packet to RF
#ifdef DEBUG_TNC
                        Serial.println("Manual TX: " + tnc2Raw);
#endif
                    }
                }
                btn_count = 0;
            }
        }
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
#ifdef SA818
            SA818_CHECK();
#endif
            if (AFSKInitAct == true)
            {
                if (config.tnc)
                {
                    String tnc2Raw = send_fix_location();
                    if (aprsClient.connected())
                        aprsClient.println(tnc2Raw); // Send packet to Inet
                    APRS_sendTNC2Pkt(tnc2Raw);       // Send packet to RF
#ifdef DEBUG_TNC
                    Serial.println("TX: " + tnc2Raw);
#endif
                }
            }
            // send_fix_location();
            //  APRS_setCallsign(config.aprs_mycall, config.aprs_ssid);
            //  	APRS_setPath1("WIDE1", 1);
            //  	APRS_setPreamble(350);
            //  	APRS_setTail(50);
            // APRS_sendTNC2Pkt("HS5TQA-6>APE32I,TRACE2-2:=1343.76N/10026.06E&ESP32 APRS Internet Gateway");
        }

        // IGate
        if (config.tnc)
        {
            if (PacketBuffer.getCount() > 0)
            {
                String tnc2;
                //นำข้อมูลแพ็จเกจจาก TNC ออกจากคิว
                PacketBuffer.pop(&incomingPacket);
                processPacket(tnc2);
                // ESP_BT.println(tnc2);
                status.allCount++;

                // String tnc2 = SerialTNC.readStringUntil('\n');
                if (config.rf2inet && aprsClient.connected())
                {
                    int start_val = tnc2.indexOf(">", 0); // หาตำแหน่งแรกของ >
                    if (start_val > 3)
                    {
                        // str=(char *)malloc(tnc2.length());
                        raw = (char *)malloc(tnc2.length() + 20);
                        status.tncCount++;
                        if (tnc2.indexOf("RFONLY", 10) > 0)
                        {
                            status.dropCount++;
                            digiTLM.DROP++;
                        }
                        else
                        {
                            str = (char *)malloc(tnc2.length());
                            tnc2.toCharArray(&str[0], tnc2.length());
                            int i = tnc2.indexOf(":");
                            int t = tnc2.indexOf("TCPIP*", 5);

                            if (i > 10)
                            {
                                if (t > 0)
                                    str[t - 1] = 0;
                                else
                                    str[i] = 0;
                                if (config.aprs_ssid == 0)
                                    sprintf(raw, "%s,qAR,%s:%s", &str[0], config.aprs_mycall, &str[i + 1]);
                                else
                                    sprintf(raw, "%s,qAR,%s-%d:%s", &str[0], config.aprs_mycall, config.aprs_ssid, &str[i + 1]);
                                // sprintf(raw, "%s", &str[0]);
                                tnc2 = String(raw);
                                aprsClient.println(tnc2);
                                status.rf2inet++;
                                digiTLM.RF2INET++;
                                digiTLM.TX++;

#ifdef DEBUG
                                printTime();
                                Serial.print("RF->INET ");
                                Serial.println(tnc2);
#endif
                            }
                            else
                            {
                                status.errorCount++;
                                digiTLM.DROP++;
                            }
                            free(str);
                        }
                        memset(&raw[0], 0, sizeof(raw));
                        tnc2.toCharArray(&raw[0], start_val + 1);
                        raw[start_val + 1] = 0;
                        pkgListUpdate(&raw[0], 1);
                        free(raw);
                        // #ifdef DEBUG
                        // 								printTime();
                        // 								Serial.print("TNC ");
                        // 								Serial.println(tnc2);
                        // #endif
                    }
                    else
                    {
                        status.errorCount++;
                    }
                }
            }
        }
    }
}

int mqttRetry = 0;
long wifiTTL = 0;

void taskNetwork(void *pvParameters)
{
    char *raw;
    // char *str;
    int c = 0;
    Serial.println("Task Network has been start");

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
        //กำหนดค่าการทำงานไวไฟเป็นแอสเซสพ้อย
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
        SerialBT.begin("ESP32TNC");
    }

    webService();
    for (;;)
    {
        // wdtNetworkTimer = millis();
        vTaskDelay(1 / portTICK_PERIOD_MS);
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
                    Serial.print("WiFi connecting..");
                    // udp.endPacket();
                    WiFi.disconnect();
                    WiFi.setTxPower(WIFI_POWER_19_5dBm);
                    WiFi.setHostname("ESP32IGate");
                    WiFi.begin(config.wifi_ssid, config.wifi_pass);
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
                        continue;
                    }

                    Serial.println("WiFi connected");
                    Serial.println("IP address: ");
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
                    configTime(3600 * timeZone, 0, "203.150.19.26", "1.pool.ntp.org");
                    vTaskDelay(3000 / portTICK_PERIOD_MS);
                    time_t systemTime;
                    time(&systemTime);
                    setTime(systemTime);
                    if (systemUptime == 0)
                    {
                        systemUptime = now();
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
                            String line = aprsClient.readStringUntil('\n'); //อ่านค่าที่ Server ตอบหลับมาทีละบรรทัด
#ifdef DEBUG_IS
                            printTime();
                            Serial.print("APRS-IS ");
                            Serial.println(line);
#endif
                            status.isCount++;
                            int start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
                            if (start_val > 3)
                            {
                                raw = (char *)malloc(line.length() + 1);
                                String src_call = line.substring(0, start_val);
                                String msg_call = "::" + src_call;

                                status.allCount++;
                                digiTLM.RX++;
                                if (config.tnc && config.inet2rf)
                                {
                                    if (line.indexOf(msg_call) <= 0) // src callsign = msg callsign
                                    {
                                        if (line.indexOf(":T#") < 0)
                                        {
                                            if (line.indexOf("::") > 0)
                                            { // message only
                                                raw[0] = '}';
                                                line.toCharArray(&raw[1], line.length());
                                                // tncTxEnable = false;
                                                SerialTNC.flush();
                                                SerialTNC.println(raw);
                                                APRS_sendTNC2Pkt(line); // Send out RF by TNC build in
                                                // tncTxEnable = true;
                                                status.inet2rf++;
                                                digiTLM.INET2RF++;
                                                printTime();
#ifdef DEBUG
                                                Serial.print("INET->RF ");
                                                Serial.println(raw);
#endif
                                            }
                                        }
                                    }
                                    else
                                    {
                                        digiTLM.DROP++;
                                        Serial.print("INET Message TELEMETRY from ");
                                        Serial.println(src_call);
                                    }
                                }

                                memset(&raw[0], 0, sizeof(raw));
                                line.toCharArray(&raw[0], start_val + 1);
                                raw[start_val + 1] = 0;
                                pkgListUpdate(&raw[0], 0);
                                free(raw);
                            }
                        }
                    }
                }

                if (millis() > pingTimeout)
                {
                    pingTimeout = millis() + 300000;
                    Serial.print("Ping to " + WiFi.gatewayIP().toString());
                    if (ping_start(WiFi.gatewayIP(), 3, 0, 0, 10) == true)
                    {
                        Serial.println(" Success!!");
                    }
                    else
                    {
                        Serial.println(" Fail!");
                        WiFi.disconnect();
                        wifiTTL = 0;
                    }
                }
            }
        }
    }
}
