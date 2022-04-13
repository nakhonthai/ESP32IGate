#include "igate.h"

extern WiFiClient aprsClient;

int igateProcess(AX25Msg &Packet)
{
    int idx, j;
    uint8_t ctmp;
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
    header += String(F(":"));
    aprsClient.print(header);
    aprsClient.write(&Packet.info[0], Packet.len); // info binary write aprsc support
    aprsClient.println();
    return 1;
}