#include "config.h"
#include <esp_wifi.h>
#include <cstring>  // Required for memcmp()

// ------------------------------------------------------------------
// Frame Control Field Masks (IEEE 802.11)
// ------------------------------------------------------------------
#define FC_TYPE_MASK      0x000C
#define FC_SUBTYPE_MASK   0x00F0
#define FC_TYPE_MGMT      0x0000

// Shifted decimal values for cleaner logic reading
#define SUBTYPE_PROBE_REQ 4
#define SUBTYPE_BEACON    8
#define SUBTYPE_DISASSOC  10
#define SUBTYPE_AUTH      11
#define SUBTYPE_DEAUTH    12

/**
 * Promiscuous mode packet callback (runs in ISR context).
 * Increments global counters for later analysis by the main loop.
 */
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    // ← ADD THIS GUARD: rx_state != 0 means the packet failed checksum (corrupted)
    if (pkt->rx_ctrl.rx_state != 0) return;

    wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload;
    wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    uint16_t frame_ctrl = hdr->frame_ctrl;
    uint8_t frame_type = (frame_ctrl & FC_TYPE_MASK) >> 2;

    // Added incBroadcastDeauth to separate the weighted score from raw count
    int incMgmt = 0, incDeauth = 0, incBroadcastDeauth = 0, incProbe = 0, incBeacon = 0, incAuth = 0;

    // We are only interested in Management frames
    if (frame_type == 0) {   
        incMgmt = 1;
        uint8_t subtype = (frame_ctrl & FC_SUBTYPE_MASK) >> 4;

        // --- Deauthentication (12) or Disassociation (10) ---
        if (subtype == SUBTYPE_DEAUTH || subtype == SUBTYPE_DISASSOC) {
            incDeauth = 1; // Always exactly 1 so baseline math isn't poisoned

            // Check if destination is broadcast
            const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            if (memcmp(hdr->addr1, broadcast_mac, 6) == 0) {
                incBroadcastDeauth = 1;  // Track broadcast separately
            }
        }
        // --- Probe Request (4) ---
        else if (subtype == SUBTYPE_PROBE_REQ) {
            incProbe = 1;
        }
        // --- Beacon (8) ---
        else if (subtype == SUBTYPE_BEACON) {
            incBeacon = 1;
        }
        // --- Authentication (11) ---
        else if (subtype == SUBTYPE_AUTH) {
            incAuth = 1;
        }
    }

    // ----- Critical Section: Protect shared counters -----
    portENTER_CRITICAL_ISR(&countMux);
    packetCount++;           
    mgmtCount += incMgmt;
    deauthCount += incDeauth; 
    broadcastDeauthCount += incBroadcastDeauth; // Add new broadcast counter
    probeCount += incProbe;
    beaconCount += incBeacon;
    authCount += incAuth;
    portEXIT_CRITICAL_ISR(&countMux);
}