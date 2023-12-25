#ifndef SATELLITE_H
#define SATELLITE_H

#include <Arduino.h>
#include <TimeLib.h>
#include <TinyGPSPlus.h>

#define SAT_NUM 8

const char tleName[SAT_NUM][12] = {"ISS","LAPAN-A2","PCSAT-1","LilacSat-2","PSAT2-1","TIANHE","MAYA-2","GuaraniSat"};
const char tlel1[SAT_NUM][71]   = {"1 25544U 98067A   22261.04293564  .00008098  00000-0  14900-3 0  9998",
                        "1 40931U 15052B   22262.05105120  .00000979  00000+0  46897-4 0  9990",
                        "1 26931U 01043C   22253.17772512  .00000053  00000-0  51957-4 0  9994",
                        "1 40908U 15049K   22253.23547515  .00003186  00000-0  16796-3 0  9996",                        
                        "1 44354U 19036R   22262.16123351  .00100925 -41642-5  75191-3 0  9991",
                        "1 48274U 21035A   22262.22216296  .00012690  00000+0  14216-3 0  9994",
                        "1 47929U 98067SF  22187.50330417  .06258577  12351-4  62901-3 0  9991",
                        "1 47931U 98067SH  22185.91280436  .06972440  12364-4  70134-3 0  9995"};
const char tlel2[SAT_NUM][71]   = {"2 25544  51.6433 236.5547 0002449 262.4353 208.9254 15.50138447359588",
                        "2 40931   5.9978 221.3765 0012965  52.2213 307.9104 14.76861461377208",
                        "2 26931  67.0509 199.7051 0006278 257.4400 102.5999 14.30730289 93685",
                        "2 40908  97.5201 253.0141 0016907  83.6898 333.7412 15.16184430385336",
                        "2 44354  28.5237 244.2031 0195261 314.1499  44.3197 15.49939514179173",
                        "2 48274  41.4739   6.0007 0005661 339.8123 103.4975 15.62472632 79479",
                        "2 47929  51.6123 198.3492 0003382 301.7247  58.3448 16.37241531 74823",
                        "2 47931  51.6128 206.4904 0003538 317.0160  43.0588 16.37231343 74610"};

const double freqUplink[SAT_NUM]={145.825,145.825,145.827,144.350,145.825,145.825,145.825,145.825};
const double freqDownlink[SAT_NUM]={145.825,145.825,145.827,144.390,145.825,145.825,145.825,145.825};

const char path[SAT_NUM][15] ={"RS0ISS","YBOX-1","W3ADO-1","BJ1SI","PSAT2-1","WIDE1-1","JG6YMY","JG6YMZ"};

typedef struct satelliteStruct {
	int num;
    double dist;
	float freqRX;
    float freqTX;
	time_t timeStamp;
    double dSatLAT = 0; // Satellite latitude
double dSatLON = 0; // Satellite longitude
double dSatAZ = 0;  // Satellite azimuth
double dSatEL = 0;  // Satellite elevation
}satelliteType;

int satPredict(int periodSec);

#endif