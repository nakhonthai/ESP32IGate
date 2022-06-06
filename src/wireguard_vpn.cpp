//==============================================================================
// Wireguard VPN Client demo for LwIP/ESP32     
//==============================================================================

//==============================================================================
//  Includes
//==============================================================================
#include <Arduino.h>
#include "main.h"

//ใช้ตัวแปรโกลบอลในไฟล์ main.cpp
extern Configuration config;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "wireguardif.h"
#include "wireguard.h"

#include "wireguard_vpn.h"
//==============================================================================
//  Defines
//==============================================================================
#define CMP_NAME "WG_VPN"

#if !defined(WG_CLIENT_PRIVATE_KEY) || !defined(WG_PEER_PUBLIC_KEY)
#error "Please update configuratiuon with your VPN-specific keys!"
#endif

//==============================================================================
//  Local types
//==============================================================================

//==============================================================================
//  Local data
//==============================================================================
static struct netif wg_netif_struct = {0};
static struct netif *wg_netif = NULL;
static uint8_t wireguard_peer_index_local = WIREGUARDIF_INVALID_INDEX;

//==============================================================================
//  Exported data
//==============================================================================

//==============================================================================
//  Local functions
//==============================================================================

//==============================================================================
//  Exported functions
//==============================================================================
bool wireguard_active()
{
    if(wg_netif!=NULL) return true;
    return false;
}

void wireguard_remove()
{

 if(wg_netif!=NULL){
        wireguardif_disconnect(wg_netif, wireguard_peer_index_local);
        wireguardif_remove_peer(wg_netif, wireguard_peer_index_local);
     //netif_set_down(wg_netif);
     //netif_remove(&wg_netif_struct);
 }
}

void wireguard_setup()
{
    struct wireguardif_init_data wg;
    struct wireguardif_peer peer;
    ip_addr_t ipaddr = WG_LOCAL_ADDRESS;
    ip_addr_t netmask = WG_LOCAL_NETMASK;
    ip_addr_t gateway = WG_GATEWAY_ADDRESS;
    ip_addr_t peer_address = WG_PEER_ADDRESS;

    ipaddr_aton(config.wg_local_address,&ipaddr);
    ipaddr_aton(config.wg_netmask_address,&netmask);
    ipaddr_aton(config.wg_gw_address,&gateway);
    ipaddr_aton(config.wg_peer_address,&peer_address);

    // Setup the WireGuard device structure
    // wg.private_key = WG_CLIENT_PRIVATE_KEY;
    // wg.listen_port = WG_CLIENT_PORT;
    wg.private_key = config.wg_private_key;
    wg.listen_port = config.wg_port+1;
    wg.bind_netif = NULL; // NB! not working on ESP32 even if set!

    if(wg_netif==NULL){
    // Register the new WireGuard network interface with lwIP
    wg_netif = netif_add(&wg_netif_struct, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gateway), &wg, &wireguardif_init, &ip_input);

    // Mark the interface as administratively up, link up flag is set automatically when peer connects
    netif_set_up(wg_netif);
    }

    // Initialise the first WireGuard peer structure
    wireguardif_peer_init(&peer);
    // peer.public_key = WG_PEER_PUBLIC_KEY;
    peer.public_key = config.wg_public_key;
    peer.preshared_key = NULL;
    // Allow all IPs through tunnel
    //peer.allowed_ip = IPADDR4_INIT_BYTES(0, 0, 0, 0);
    IP_ADDR4(&peer.allowed_ip, 0, 0, 0, 0);
    IP_ADDR4(&peer.allowed_mask, 0, 0, 0, 0);

    // If we know the endpoint's address can add here
    ip_addr_set(&peer.endpoint_ip, &peer_address);
    //peer.endport_port = WG_PEER_PORT;
    peer.endport_port = config.wg_port;

    // Register the new WireGuard peer with the netwok interface
    wireguardif_add_peer(wg_netif, &peer, &wireguard_peer_index_local);

    if ((wireguard_peer_index_local != WIREGUARDIF_INVALID_INDEX) && !ip_addr_isany(&peer.endpoint_ip))
    {
        // Start outbound connection to peer
        wireguardif_connect(wg_netif, wireguard_peer_index_local);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus