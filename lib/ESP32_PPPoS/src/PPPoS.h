#ifndef _PPPOS_H_
#define _PPPOS_H_

#ifndef ESP32
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#include <Arduino.h>
#include <WiFi.h>

class PPPoS {
public:

    void begin(Stream* stream);

    bool connect(const char* apn = "", const char* user = "", const char* pass = "");

    bool status() {
        return mConnected;
    }

    void end();

private:
    bool waitResponse(String& data);

public:
    Stream* mStream;

    bool mFirstStart = false;
    bool mConnected = false;
    bool mStarted = false;
};

#endif
