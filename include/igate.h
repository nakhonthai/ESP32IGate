#ifndef IGATE_H
#define IGATE_H

#include <LibAPRSesp.h>
#include <WiFiClient.h>
#include <AX25.h>
#include "main.h"

int igateProcess(AX25Msg &Packet);

#endif