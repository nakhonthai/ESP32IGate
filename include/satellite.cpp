/* ====================================================================

   Copyright (c) 2019-2021 Thorsten Godau (https://github.com/dl9sec)
   All rights reserved.


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

   3. Neither the name of the author(s) nor the names of any contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   ====================================================================*/

#include <Arduino.h>
#include <AioP13.h>
#include <time.h>
#include "satellite.h"

extern TinyGPSPlus gps;

#define MAP_MAXX 1150
#define MAP_MAXY 609

// const char *tleName = "ISS (ZARYA)";
// const char *tlel1   = "1 25544U 98067A   21320.51955234  .00001288  00000+0  31985-4 0  9990";
// const char *tlel2   = "2 25544  51.6447 309.4881 0004694 203.6966 299.8876 15.48582035312205";

// For testing purpose (geostationary, no motion)
// const char *tleName = "ES'HAIL 2";
// const char *tlel1   = "1 43700U 18090A   21320.51254296  .00000150  00000+0  00000+0 0  9998";
// const char *tlel2   = "2 43700   0.0138 278.3980 0002418 337.0092  10.7288  1.00272495 10898";

const char *pcMyName = "HS5TQA-11"; // Observer name
double dMyLAT = 10.2617;            // 12.9782;            // Latitude (Breitengrad): N -> +, S -> -
double dMyLON = 101.4673;           // 100.4657;           // Longitude (Längengrad): E -> +, W -> -
double dMyALT = 1.0;                // Altitude ASL (m)

double dfreqRX = 145.825; // Nominal downlink frequency
double dfreqTX = 145.825; // Nominal uplink frequency

int iYear = 2021; // Set start year
int iMonth = 11;  // Set start month
int iDay = 18;    // Set start day
int iHour = 23;   // Set start hour
int iMinute = 8;  // Set start minute
int iSecond = 2;  // Set start second

// Expecting the ISS to be at 289,61° elevation and 20,12° azimuth (Gpredict)
// Result for ESP32 will be 289,74° elevation and 20,44° azimuth.
// Result for UNO will be 289,70° elevation and 20,75° azimuth.
// Expecting the sun to be at -60.79° elevation and 0.86° azimuth (https://www.sunearthtools.com/dp/tools/pos_sun.php)
// Result for ESP32 will be -60.79° elevation and 0.89° azimuth.
// Result for UNO will be -60.79° elevation and 0.94° azimuth.

double dSatLAT = 0; // Satellite latitude
double dSatLON = 0; // Satellite longitude
double dSatAZ = 0;  // Satellite azimuth
double dSatEL = 0;  // Satellite elevation

double dSunLAT = 0; // Sun latitude
double dSunLON = 0; // Sun longitude
double dSunAZ = 0;  // Sun azimuth
double dSunEL = 0;  // Sun elevation

int ixQTH = 0; // Map pixel coordinate x of QTH
int iyQTH = 0; // Map pixel coordinate y of QTH
int ixSAT = 0; // Map pixel coordinate x of satellite
int iySAT = 0; // Map pixel coordinate y of satellite
int ixSUN = 0; // Map pixel coordinate x of sun
int iySUN = 0; // Map pixel coordinate y of sun

char acBuffer[P13DateTime::ascii_str_len + 1]; // Buffer for ASCII time

int aiSatFP[32][2]; // Array for storing the satellite footprint map coordinates
int aiSunFP[32][2]; // Array for storing the sunlight footprint map coordinates

satelliteType satelliteList[SAT_NUM];

// extern RTC_DATA_ATTR bool satelliteMode;
extern RTC_DATA_ATTR satelliteType satelliteActive;

/// Degrees to radians.
#define DEG2RAD(x) (x / 360 * 2 * PI)
/// Radians to degrees.
#define RAD2DEG(x) (x * (180 / PI))

int satPredict(int periodSec)
{
  int i;

#ifdef ARDUINO_ARCH_ESP32
  // Serial.setDebugOutput(false);
#endif

  struct tm tmstruct;
  time_t nowTime;

  // localtime_r(&pkgList[i].time, &tmstruct);
  //  do{
  delay(2000);
  time(&nowTime);

  // nowTime-=25200;
  localtime_r(&nowTime, &tmstruct);
  // getLocalTime(&tmstruct, 3000);

  iYear = (tmstruct.tm_year) + 1900;
  iMonth = tmstruct.tm_mon + 1;
  iDay = tmstruct.tm_mday;
  iHour = tmstruct.tm_hour;
  iMinute = tmstruct.tm_min;
  iSecond = tmstruct.tm_sec;
  Serial.printf("Start Date: %04d-%02d-%02d %02d:%02d:%02d\n", iYear, iMonth, iDay, iHour, iMinute, iSecond);
  // Serial.printf("Now Date: %02d%02d %02d:%02d", (tmstruct.tm_mon+1),tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);

  if (gps.location.isValid())
  {
    dMyLAT = gps.location.lat();
    dMyLON = gps.location.lng();
    dMyALT = gps.altitude.meters();
  }

  P13Sun Sun;                                                       // Create object for the sun
  P13DateTime MyTime(iYear, iMonth, iDay, iHour, iMinute, iSecond); // Set start time for the prediction
  P13Observer MyQTH(pcMyName, dMyLAT, dMyLON, dMyALT);              // Set observer coordinates

  P13Satellite MySAT(tleName[0], tlel1[0], tlel2[0]); // Create ISS data from TLE

  // latlon2xy(ixQTH, iyQTH, dMyLAT, dMyLON, MAP_MAXX, MAP_MAXY); // Get x/y for the pixel map

  // Serial.printf("\r\nRunning on ESP32\r\nPrediction for %s at %s (MAP %dx%d: x = %d,y = %d):\r\n\r\n", MySAT.c_ccSatName, MyQTH.c_ccObsName, MAP_MAXX, MAP_MAXY, ixQTH, iyQTH);

  double im = (double)1 / 1440;
  int sateNum = 0;
  for (int i = 0; i < SAT_NUM; i++)
  {
    satelliteList[i].num = 0;
    satelliteList[i].timeStamp = 0;
    satelliteList[i].dist = 2000;
  }

  for (int s = 0; s < SAT_NUM; s++)
  {
    MyTime.settime(iYear, iMonth, iDay, iHour, iMinute, iSecond);
    MySAT.tle(tleName[s], tlel1[s], tlel2[s]);
    // Serial.printf("\r\nPrediction for %s at %s :\r\n", MySAT.c_ccSatName, MyQTH.c_ccObsName);
    for (int i = 0; i < periodSec / 60; i++)
    {
      MyTime.ascii(acBuffer);            // Get time for prediction as ASCII string
      MySAT.predict(MyTime);             // Predict ISS for specific time
      MySAT.latlon(dSatLAT, dSatLON);    // Get the rectangular coordinates
      MySAT.elaz(MyQTH, dSatEL, dSatAZ); // Get azimut and elevation for MyQTH
      MyTime.add(im);

      // latlon2xy(ixSAT, iySAT, dSatLAT, dSatLON, MAP_MAXX, MAP_MAXY); // Get x/y for the pixel map
      if (dSatEL > 10)
      {
        double dist = gps.distanceBetween(dMyLAT, dMyLON, dSatLAT, dSatLON) / cos(DEG2RAD(dSatEL));
        dist /= 1000.0f;
        if (dist < satelliteList[sateNum].dist)
        {
          satelliteList[sateNum].dist = dist;
          satelliteList[sateNum].num = s;
          satelliteList[sateNum].freqRX = MySAT.doppler(dfreqRX, P13_FRX);
          satelliteList[sateNum].freqTX = MySAT.doppler(dfreqTX, P13_FTX);
          satelliteList[sateNum].dSatLAT = dSatLAT;
          satelliteList[sateNum].dSatLON = dSatLON;
          satelliteList[sateNum].dSatAZ = dSatAZ;
          satelliteList[sateNum].dSatEL = dSatEL;
          int cYear, cMonth, cDay, cHour, cMinute, cSecond;
          MyTime.gettime(cYear, cMonth, cDay, cHour, cMinute, cSecond);
          tmElements_t timeinfo;

          timeinfo.Year = (cYear)-1970;
          timeinfo.Month = cMonth;
          timeinfo.Day = cDay;
          timeinfo.Hour = cHour;
          timeinfo.Minute = cMinute;
          timeinfo.Second = cSecond;
          time_t timeStamp = makeTime(timeinfo);
          satelliteList[sateNum].timeStamp = timeStamp;
          // Serial.printf("timeStamp %i\n",timeStamp);
          //  Serial.printf("%s -> Lat: %.4f Lon: %.4f Az: %.2f El: %.2f Dist:%0.1fKm. ", acBuffer, dSatLAT, dSatLON, dSatAZ, dSatEL,dist/1000);
          //  Serial.printf("RX: %.6f MHz, TX: %.6f MHz Path:%s\r\n", MySAT.doppler(dfreqRX, P13_FRX), MySAT.doppler(dfreqTX, P13_FTX),path[s]);
        }
        // Serial.printf("%s -> Lat: %.4f Lon: %.4f Az: %.2f El: %.2f Dist:%0.1fKm. ", acBuffer, dSatLAT, dSatLON, dSatAZ, dSatEL,dist);
        // Serial.printf("RX: %.6f MHz, TX: %.6f MHz Path:%s\r\n", MySAT.doppler(dfreqRX, P13_FRX), MySAT.doppler(dfreqTX, P13_FTX),path[s]);
      }
    }
    if (satelliteList[sateNum].timeStamp > 0)
      sateNum++;
    if (sateNum > sizeof(satelliteList))
      break;
  }

  int nowSat = -1;
  time_t firstTime = nowTime + periodSec;
  double nearDist = 2000;
  for (int s = 0; s < sateNum; s++)
  {
    if (satelliteList[s].timeStamp > 0)
    {
      if (satelliteList[s].dist < nearDist)
      {
        nearDist = satelliteList[s].dist;
        // if (satelliteList[s].timeStamp < firstTime)
        // {
        //   firstTime = satelliteList[s].timeStamp;
          nowSat = s;
      }
      time_t timeStamp = satelliteList[s].timeStamp + 25200;
      localtime_r(&timeStamp, &tmstruct);
      iYear = (tmstruct.tm_year) + 1900;
      iMonth = tmstruct.tm_mon + 1;
      iDay = tmstruct.tm_mday;
      iHour = tmstruct.tm_hour;
      iMinute = tmstruct.tm_min;
      iSecond = tmstruct.tm_sec;
      sprintf(acBuffer, "%04d-%02d-%02d %02d:%02d:%02d", iYear, iMonth, iDay, iHour, iMinute, iSecond);
      Serial.printf("[%s] %s -> Lat: %.4f Lon: %.4f Az: %.2f El: %.2f Dist:%0.1fKm. ", tleName[satelliteList[s].num], acBuffer, satelliteList[s].dSatLAT, satelliteList[s].dSatLON, satelliteList[s].dSatAZ, satelliteList[s].dSatEL, satelliteList[s].dist);
      Serial.printf("RX: %.6f MHz, TX: %.6f MHz Path:%s\r\n", satelliteList[s].freqRX, satelliteList[s].freqRX, path[satelliteList[s].num]);
    }
  }

  if (nowSat > -1)
  {
    memcpy(&satelliteActive, &satelliteList[nowSat], sizeof(satelliteType));
    time_t nextTime = satelliteList[nowSat].timeStamp - nowTime;
    Serial.printf("Found Satellite %s in comming %d Sec.", tleName[satelliteList[nowSat].num], nextTime);
    Serial.printf("RX: %.6f MHz, TX: %.6f MHz Path:%s\r\n", satelliteList[nowSat].freqRX, satelliteList[nowSat].freqRX, path[satelliteList[nowSat].num]);
    return nowSat;
  }
  return -1;
  // Calcualte footprint
  //   Serial.printf("Satellite footprint map coordinates:\n\r");

  // MySAT.footprint(aiSatFP, (sizeof(aiSatFP)/sizeof(int)/2), MAP_MAXX, MAP_MAXY, dSatLAT, dSatLON);

  // for (i = 0; i < (sizeof(aiSatFP)/sizeof(int)/2); i++)
  // {
  //     Serial.printf("%2d: x = %d, y = %d\r\n", i, aiSatFP[i][0], aiSatFP[i][1]);
  // }

  // // Predict sun
  // Sun.predict(MyTime);                // Predict ISS for specific time
  // Sun.latlon(dSunLAT, dSunLON);       // Get the rectangular coordinates
  // Sun.elaz(MyQTH, dSunEL, dSunAZ);    // Get azimut and elevation for MyQTH

  // latlon2xy(ixSUN, iySUN, dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY);

  //   Serial.printf("\r\nSun -> Lat: %.4f Lon: %.4f (MAP %dx%d: x = %d,y = %d) Az: %.2f El: %.2f\r\n\r\n", dSunLAT, dSunLON, MAP_MAXX, MAP_MAXY, ixSUN, iySUN, dSunAZ, dSunEL);

  // // Calcualte sunlight footprint
  //   Serial.printf("Sunlight footprint map coordinates:\n\r");

  // Sun.footprint(aiSunFP, (sizeof(aiSunFP)/sizeof(int)/2), MAP_MAXX, MAP_MAXY, dSunLAT, dSunLON);

  // for (i = 0; i < (sizeof(aiSunFP)/sizeof(int)/2); i++)
  // {
  //     Serial.printf("%2d: x = %d, y = %d\r\n", i, aiSunFP[i][0], aiSunFP[i][1]);
  // }

  //   Serial.printf("\r\nFinished\n\r");
}
