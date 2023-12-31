/*
 Name:		ESP32IGate
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#include "weather.h"

extern Configuration config;
extern WiFiClient aprsClient;

WeatherData weather;

bool weatherUpdate = false;
bool getCSV2Wx(String stream)
{
    bool ret = false;
    String weatherRaw;
    int st = 0;
    // stream = Serial.readString();
    //  Serial.println(stream);
    // delay(100);
    st = stream.indexOf("DATA:");
    // Serial.printf("Find ModBus > %d",st);
    // Serial.println(stream);
    if (st >= 0)
    {
        // String data = stream.substring(st+5);
        weatherRaw = stream.substring(st + 5);
        weatherUpdate = true;
        // Serial.println("Weather DATA: " + weatherRaw);
    }
    // else
    // {
    //     st = stream.indexOf("CLK?");
    //     if (st >= 0)
    //     {
    //         struct tm br_time;
    //         getLocalTime(&br_time, 5000);
    //         char strtmp[30];
    //         Serial.printf(strtmp, "#CLK=%02d/%02d/%02d,%02d:%02d:%02d\r\n", br_time.tm_year - 2000, br_time.tm_mon, br_time.tm_mday, br_time.tm_hour, br_time.tm_min, br_time.tm_sec);
    //     }
    // }
    //CSV Format DATA:date,time,rain(mm),ws(kph),wd(deg),solar(w/m),barometric(hpa/10),temp(c),humidity(%RH),vbat(V),vsolar(V),ibat(A),pbat(W),windgust(kph)
    if (weatherUpdate)
    {
        // String value;
        float rainSample = getValue(weatherRaw, ',', 2).toFloat();
        weather.rain += rainSample;
        weather.rain24hr += rainSample;
        weather.windspeed = getValue(weatherRaw, ',', 3).toFloat();
        weather.winddirection = getValue(weatherRaw, ',', 4).toInt();
        weather.solar = getValue(weatherRaw, ',', 5).toInt();
        weather.barometric = getValue(weatherRaw, ',', 6).toFloat() * 10.0F;
        weather.temperature = getValue(weatherRaw, ',', 7).toFloat();
        weather.humidity = getValue(weatherRaw, ',', 8).toFloat();
        weather.windgust = getValue(weatherRaw, ',', 13).toFloat();
        weather.vbat = getValue(weatherRaw, ',', 9).toFloat();
        weather.vsolar = getValue(weatherRaw, ',', 10).toFloat();
        weather.ibat = getValue(weatherRaw, ',', 11).toFloat();
        weather.pbat = getValue(weatherRaw, ',', 12).toInt();

        weather.visable|=WX_RAIN;
        weather.visable|=WX_RAIN24HR;
        weather.visable|=WX_WIND_SPD;
        weather.visable|=WX_WIND_DIR;
        weather.visable|=WX_LUMINOSITY;
        weather.visable|=WX_BARO;
        weather.visable|=WX_WIND_GUST;
        weather.visable|=WX_TEMP;
        weather.visable|=WX_HUMIDITY;

        weatherUpdate = false;
        // wxTimeout = millis();
        char strData[300];
        size_t lng = getRawWx(strData);

        if (config.wx_2rf)
        { // WX SEND POSITION TO RF
            pkgTxPush(strData, lng, 0);
        }
        if (config.wx_2inet)
        { // WX SEND TO APRS-IS
            if (aprsClient.connected())
            {
                // status.txCount++;
                aprsClient.println(strData); // Send packet to Inet
            }
        }
        // Serial.printf("APRS: %s\r\n",strData);
        return true;
    }
    return false;
    // ModbusSerial.flush();
}

bool state = true;
bool getM702Modbus(ModbusMaster &node)
{
uint8_t result;
  
  // Toggle the coil at address 0x0002 (Manual Load Control)
  result = node.writeSingleCoil(0x0002, state);
  state = !state;

  // Read 16 registers starting at 0x3100)
  result = node.readInputRegisters(0x0002, 7);
  if (result == node.ku8MBSuccess)
  {
    weather.co2 = node.getResponseBuffer(0x00); //Co2 PPM
    weather.ch2o = node.getResponseBuffer(0x02); //ug
    weather.pm25 = node.getResponseBuffer(0x06); //PM2.5ug
    weather.pm100 = node.getResponseBuffer(0x08); //PM10 ug
    weather.temperature = (float)node.getResponseBuffer(10)/10.0f; //C
    weather.humidity = (float)node.getResponseBuffer(12)/10.0f; //%RH
    weather.visable|=WX_CO2;
    weather.visable|=WX_PM25;
    weather.visable|=WX_PM100;
    weather.visable|=WX_TEMP;
    weather.visable|=WX_HUMIDITY;
    return true;
  }
  return false;
}

int getRawWx(char *strData)
{
    unsigned int i;

    int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
    char strtmp[300], obj[30];

    memset(&obj[0], 0, sizeof(obj));

    DD_DDDDDtoDDMMSS(config.wx_lat, &lat_dd, &lat_mm, &lat_ss);
    DD_DDDDDtoDDMMSS(config.wx_lon, &lon_dd, &lon_mm, &lon_ss);
    if (strlen(config.wx_object) >= 3)
    {
        char object[10];
        memset(object, 0x20, 10);
        memcpy(object, config.wx_object, strlen(config.wx_object));
        object[9] = 0;
        String timeStamp = getTimeStamp();
        sprintf(obj, ";%s*%s", object, timeStamp);
    }
    else
    {
        if (config.wx_timestamp)
        {
            String timeStamp = getTimeStamp();
            sprintf(obj, "/%s", timeStamp.c_str());
        }
        else
        {
            sprintf(obj, "!");
            obj[1] = 0;
        }
    }
    if (config.wx_ssid == 0)
    {
        if (config.wx_path < 5)
        {
            if (config.wx_path > 0)
                sprintf(strtmp, "%s>APE32I-%d:", config.wx_mycall, config.wx_path);
            else
                sprintf(strtmp, "%s>APE32I:", config.wx_mycall);
        }
        else
        {
            sprintf(strtmp, "%s>APE32I,%s:", config.wx_mycall, getPath(config.igate_path).c_str());
        }
    }
    else
    {
        if (config.wx_path < 5)
        {
            if (config.wx_path > 0)
                sprintf(strtmp, "%s-%d>APE32I-%d:", config.wx_mycall, config.wx_ssid, config.wx_path);
            else
                sprintf(strtmp, "%s-%d>APE32I:", config.wx_mycall, config.wx_ssid);
        }
        else
        {
            sprintf(strtmp, "%s-%d>APE32I,%s:", config.wx_mycall, config.wx_ssid, getPath(config.igate_path).c_str());
        }
    }

    strcat(strData, strtmp);
    strcat(strData, obj);

    sprintf(strtmp, "%02d%02d.%02dN/%03d%02d.%02dE_", lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss);
    strcat(strData, strtmp);

    if (config.wx_flage & WX_WIND_DIR)
    {
        if(weather.visable & WX_WIND_DIR)
            sprintf(strtmp, "%03u/", weather.winddirection);
        else
            sprintf(strtmp, ".../");
    }
    else
    {
        sprintf(strtmp, ".../");
    }
    strcat(strData, strtmp);
    if (config.wx_flage & WX_WIND_SPD)
    {
        if(weather.visable & WX_WIND_SPD)
            sprintf(strtmp, "%03u", (unsigned int)(weather.windspeed * 0.621));
        else
            sprintf(strtmp, "...");
        
    }
    else
    {
        sprintf(strtmp, "...");
    }
    strcat(strData, strtmp);

    if (config.wx_flage & WX_WIND_GUST)
    {
        sprintf(strtmp, "g%03u", (unsigned int)(weather.windgust * 0.621));
    }
    else
    {
        sprintf(strtmp, "g...");
    }
    strcat(strData, strtmp);

    if (config.wx_flage & WX_TEMP)
    {
        if(weather.visable & WX_TEMP){
            unsigned int tempF = (unsigned int)((weather.temperature * 9 / 5) + 32);
            sprintf(strtmp, "t%03u", tempF);
        }else{
            sprintf(strtmp, "t...");
        }
    }
    else
    {
        sprintf(strtmp, "t...");
    }
    strcat(strData, strtmp);

    unsigned int rain = (unsigned int)((weather.rain * 100.0F) / 25.6F);
    unsigned int rain24 = (unsigned int)((weather.rain24hr * 100.0F) / 25.6F);
    unsigned int rainGMT = (unsigned int)((weather.rainmidnight * 100.0F) / 25.6F);
    time_t now;
    time(&now);
    struct tm *info = gmtime(&now);
    if (info->tm_min == 0)
    {
        rain = 0;
        weather.rain = 0;
    }
    if (info->tm_hour == 0 && info->tm_min == 0)
    {
        rain24 = 0;
        weather.rain24hr = 0;
    }

    if (config.wx_flage & WX_RAIN)
    {
        if(weather.visable & WX_RAIN)
            sprintf(strtmp, "r%03u", rain);
        else
            sprintf(strtmp, "r...");
        
    }
    else
    {
        sprintf(strtmp, "r...");
    }
    strcat(strData, strtmp);
    if (config.wx_flage & WX_RAIN24HR)
    {
        if(weather.visable & WX_RAIN24HR)
            sprintf(strtmp, "p%03u", rain24);
        else
            sprintf(strtmp, "p...");
        
    }
    else
    {
        sprintf(strtmp, "p...");
    }
    strcat(strData, strtmp);

    if (config.wx_flage & WX_RAIN_GMT)
    {
        if(weather.visable & WX_RAIN_GMT)
            sprintf(strtmp, "P%03u", rainGMT);
        else
            sprintf(strtmp, "P...");
        
    }
    else
    {
        sprintf(strtmp, "P...");
    }
    strcat(strData, strtmp);

    if (config.wx_flage & WX_HUMIDITY)
    {
        if(weather.visable & WX_HUMIDITY)
            sprintf(strtmp, "h%02u", (unsigned int)weather.humidity);
        else
            sprintf(strtmp, "h..");
        
    }
    else
    {
        sprintf(strtmp, "h..");
    }
    strcat(strData, strtmp);

    if (config.wx_flage & WX_BARO)
    {
        if(weather.visable & WX_BARO)
            sprintf(strtmp, "b%05u", (unsigned int)(weather.barometric * 10));
        else
            sprintf(strtmp, "b.....");
        
    }
    else
    {
        sprintf(strtmp, "b.....");
    }
    strcat(strData, strtmp);

    if (config.wx_flage & WX_LUMINOSITY)
    {
        if(weather.visable & WX_LUMINOSITY){
            if (weather.solar < 1000)
                sprintf(strtmp, "L%03u", (unsigned int)weather.solar);
            else
                sprintf(strtmp, "l%03u", (unsigned int)weather.solar - 1000);
            strcat(strData, strtmp);
        }
    }

    if (config.wx_flage & WX_SNOW)
    {
        if(weather.visable & WX_SNOW){
            sprintf(strtmp, "S%03u", (unsigned int)(weather.snow * 10));
            strcat(strData, strtmp);
        }
    }
    

    if (config.wx_flage & WX_SOIL_TEMP)
    {
        if(weather.visable & WX_SOIL_TEMP){
            sprintf(strtmp, "m%03u", (int)((weather.soil_temp * 9 / 5) + 32));
            strcat(strData, strtmp);
        }        
    }

    if (config.wx_flage & WX_SOIL_MOISTURE)
    {
        if(weather.visable & WX_SOIL_MOISTURE){
            sprintf(strtmp, "M%03u", (unsigned int)(weather.soil_moisture * 10));
            strcat(strData, strtmp);
        }
        
    }

    if (config.wx_flage & WX_WATER_TEMP)
    {
        if(weather.visable & WX_WATER_TEMP){
            sprintf(strtmp, "w%03u", (int)((weather.water_temp * 9 / 5) + 32));
            strcat(strData, strtmp);
        }
        
    }

    if (config.wx_flage & WX_WATER_TDS)
    {
        if(weather.visable & WX_WATER_TDS){
            sprintf(strtmp, "W%03u", (int)weather.water_tds);
            strcat(strData, strtmp);
        }        
    }

    if (config.wx_flage & WX_WATER_LEVEL)
    {
        if(weather.visable & WX_WATER_LEVEL){
            sprintf(strtmp, "v%03u", (int)weather.water_level);
            strcat(strData, strtmp);
        } 
        
    }

    if (config.wx_flage & WX_PM25)
    {
        
        if(weather.visable & WX_PM25){
            sprintf(strtmp, "o%03u", (int)weather.pm25);
            strcat(strData, strtmp);
        } 
        
    }

    if (config.wx_flage & WX_PM100)
    {
        if(weather.visable & WX_PM100){
            sprintf(strtmp, "O%03u", (int)weather.pm100);
            strcat(strData, strtmp);
        }
        
    }    

    if (config.wx_flage & WX_CO2)
    {
        if(weather.visable & WX_CO2){
            if (weather.co2 < 10000)
                sprintf(strtmp, "x%04u", (unsigned int)weather.co2);
            else
                sprintf(strtmp, "X%06u", (unsigned int)weather.co2 - 1000);
            strcat(strData, strtmp);
        }
    }

    //sprintf(strtmp, " BAT:%0.2fV/%dmA", weather.vbat, (int)(weather.ibat * 1000));
    //strcat(strData, strtmp);
    // sprintf(strtmp,"/%0.1fmA",weather.ibat*1000);
    // strcat(strData,strtmp);
    // i06L...
    // i04S...
    // i03I...

    i = strlen(strData);
    return i;
    // c...s...g...t...r...p...P...h..b.....L...S..m...M...w...W....v...o...O...x....
}