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
bool getCSV2Wx()
{
    bool ret = false;
    String stream, weatherRaw;
    int st = 0;
    if (Serial.available() > 30)
    {
        stream = Serial.readString();
        // Serial.println(stream);
        delay(100);
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

            weatherUpdate = false;
            //wxTimeout = millis();
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
                    //status.txCount++;
                    aprsClient.println(strData); // Send packet to Inet
                }
            }
            // Serial.printf("APRS: %s\r\n",strData);
        }
        Serial.flush();
    }
    // ModbusSerial.flush();
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
        memcpy(object, config.wx_object,strlen(config.wx_object));
        object[9] = 0;
        String timeStamp=getTimeStamp();
        sprintf(obj, ";%s*%s", object,timeStamp);
    }
    else
    {
        if(config.wx_timestamp){
            String timeStamp=getTimeStamp();
            sprintf(obj,"/%s",timeStamp.c_str());
        }else{
            sprintf(obj, "!");
            obj[1] = 0;
        }
    }
    if (config.wx_ssid == 0){
        if (config.wx_path < 5)
        {
            if (config.wx_path > 0)
                sprintf(strtmp, "%s>APE32I-%d:", config.wx_mycall,config.wx_path);
            else
                sprintf(strtmp, "%s>APE32I:", config.wx_mycall);
        }
        else
        {
            sprintf(strtmp, "%s>APE32I,%s:", config.wx_mycall,getPath(config.igate_path).c_str());
        }
    }else{
        if (config.wx_path < 5)
        {
            if (config.wx_path > 0)
                sprintf(strtmp, "%s-%d>APE32I-%d:", config.wx_mycall, config.wx_ssid,config.wx_path);
            else
                sprintf(strtmp, "%s-%d>APE32I:", config.wx_mycall, config.wx_ssid);
        }
        else
        {
            sprintf(strtmp, "%s-%d>APE32I,%s:", config.wx_mycall, config.wx_ssid,getPath(config.igate_path).c_str());
        }
    }

    strcat(strData, strtmp);
    strcat(strData, obj);

    sprintf(strtmp, "%02d%02d.%02dN/%03d%02d.%02dE_", lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss);
    strcat(strData, strtmp);

    if(config.wx_flage & WX_WIND_DIR){
        sprintf(strtmp, "%03u/", weather.winddirection);
    }else{
        sprintf(strtmp, ".../");
    }
    strcat(strData, strtmp);
    if(config.wx_flage & WX_WIND_SPD){
        sprintf(strtmp, "%03u", (unsigned int)(weather.windspeed * 0.621));
    }else{
        sprintf(strtmp, "...");
    }
    strcat(strData, strtmp);
    if(config.wx_flage & WX_WIND_GUST){
        sprintf(strtmp, "g%03u", (unsigned int)(weather.windgust * 0.621));
    }else{
        sprintf(strtmp, "g...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_TEMP){
        unsigned int tempF = (unsigned int)((weather.temperature * 9 / 5) + 32);
        sprintf(strtmp, "t%03u", tempF);
    }else{
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

    if(config.wx_flage & WX_RAIN){
        sprintf(strtmp, "r%03u", rain);
    }else{
        sprintf(strtmp, "r...");
    }
    strcat(strData, strtmp);
    if(config.wx_flage & WX_RAIN24HR){
        sprintf(strtmp, "p%03u", rain24);
    }else{
        sprintf(strtmp, "p...");
    }
    strcat(strData, strtmp);
    if(config.wx_flage & WX_RAIN_GMT){
        sprintf(strtmp, "P%03u", rainGMT);
    }else{
        sprintf(strtmp, "P...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_HUMIDITY){
        sprintf(strtmp, "h%02u", (unsigned int)weather.humidity);
    }else{
        sprintf(strtmp, "h..");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_BARO){
        sprintf(strtmp, "b%05u", (unsigned int)(weather.barometric * 10));
    }else{
        sprintf(strtmp, "b.....");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_LUMINOSITY){
        if (weather.solar < 1000)
            sprintf(strtmp, "L%03u", (unsigned int)weather.solar);
        else
            sprintf(strtmp, "l%03u", (unsigned int)weather.solar - 1000);
        strcat(strData, strtmp);
    }

    if(config.wx_flage & WX_SNOW){
        sprintf(strtmp, "S%03u", (unsigned int)(weather.snow * 10));
    }else{
        sprintf(strtmp, "S...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_SOIL_TEMP){
        sprintf(strtmp,"m%03u",(int)((weather.soil_temp*9/5)+32));
    }else{
        sprintf(strtmp, "m...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_SOIL_MOISTURE){
        sprintf(strtmp,"M%03u",(unsigned int)(weather.soil_moisture*10));
    }else{
        sprintf(strtmp, "M...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_WATER_TEMP){
        sprintf(strtmp,"w%03u",(int)((weather.water_temp*9/5)+32));
    }else{
        sprintf(strtmp, "w...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_WATER_TDS){
        sprintf(strtmp,"W%03u",(int)weather.water_tds);
    }else{
        sprintf(strtmp, "W...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_WATER_LEVEL){
        sprintf(strtmp,"v%03u",(int)weather.water_level);
    }else{
        sprintf(strtmp, "v...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_PM25){
        sprintf(strtmp,"o%03u",(int)weather.pm25);
    }else{
        sprintf(strtmp, "o...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_PM100){
        sprintf(strtmp,"O%03u",(int)weather.pm100);
    }else{
        sprintf(strtmp, "O...");
    }
    strcat(strData, strtmp);

    if(config.wx_flage & WX_CO2){
        if (weather.co2 < 10000)
            sprintf(strtmp, "x%04u", (unsigned int)weather.co2);
        else
            sprintf(strtmp, "X%06u", (unsigned int)weather.co2 - 1000);
        strcat(strData, strtmp);
    }

    sprintf(strtmp, " BAT:%0.2fV/%dmA", weather.vbat, (int)(weather.ibat * 1000));
    strcat(strData, strtmp);
    // sprintf(strtmp,"/%0.1fmA",weather.ibat*1000);
    // strcat(strData,strtmp);

    i = strlen(strData);
    return i;
    //c...s...g...t...r...p...P...h..b.....L...S..m...M...w...W....v...o...O...x....
}