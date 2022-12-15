#include "igate.h"

extern WiFiClient aprsClient;
extern Configuration config;

int igateProcess(AX25Msg &Packet)
{
    int idx, j;
    String header;

    j = 0;
    if (Packet.len < 2)
    {
        // digiLog.ErPkts++;
        return 0; // NO INFO DATA
    }

    for (idx = 0; idx < Packet.rpt_count; idx++)
    {
        if (!strncmp(&Packet.rpt_list[idx].call[0], "RFONLY", 6))
        {
            // digiLog.DropRx++;
            return 0;
        }
    }

    for (idx = 0; idx < Packet.rpt_count; idx++)
    {
        if (!strncmp(&Packet.rpt_list[idx].call[0], "TCPIP", 5))
        {
            // digiLog.DropRx++;
            return 0;
        }
    }

    header = String(Packet.src.call);
    if (Packet.src.ssid > 0)
    {
        header += String(F("-"));
        header += String(Packet.src.ssid);
    }
    header += String(F(">"));
    header += String(Packet.dst.call);
    if (Packet.dst.ssid > 0)
    {
        header += String(F("-"));
        header += String(Packet.dst.ssid);
    }

    //Add Path
    for (int i = 0; i < Packet.rpt_count; i++)
    {
        header += String(",");
        header += String(Packet.rpt_list[i].call);
        if (Packet.rpt_list[i].ssid > 0)
        {
            header += String("-");
            header += String(Packet.rpt_list[i].ssid);
        }
        if (Packet.rpt_flags & (1 << i))
            header += "*";
    }

    //Add qAR,callSSID
    header += String(F(",qAR,"));
    if(strlen(config.aprs_object)>=3){
        header += String(config.aprs_object);
    }else{
        header += String(config.aprs_mycall);    
        if (config.aprs_ssid > 0)
        {
            header += String(F("-"));
            header += String(config.aprs_ssid);
        }
    }

    //Add Infomation
    header += String(F(":"));
    uint8_t Raw[500];
    memset(Raw,0,sizeof(Raw));
    size_t hSize=strlen(header.c_str());
    memcpy(&Raw[0],header.c_str(),hSize);
    memcpy(&Raw[hSize],&Packet.info[0], Packet.len);
    aprsClient.write(&Raw[0], hSize+Packet.len); // info binary write aprsc support
    aprsClient.println();
    return 1;
}