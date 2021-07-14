#pragma once
/******************************************************************************
 * Copyright © 2021 The Komodo Core Developers.                               *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Komodo software, including this file may be copied, modified, propagated   *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#include <cstdint>
#include "protocol.h" // enum NODE_NETWORK

/****
 * A structure that contains the parameters of the P2P network
 */
struct P2PParameters
{
    bool showAlerts = false; // show alerts that come in from P2P connections
    bool discover = true; // this node should attempt to discover itself
    bool nspvProcessing = false; // handle NSPV processing
    uint64_t localServices = NODE_NETWORK; // bits denote services offered
    uint32_t maxOutboundConnections = 16; // maximum number of outbound connections
    uint32_t maxPeerConnections = 384; // maximum number of peer connections
    int maxInboundFromIP = 5; // maximum inbound connections from 1 IP address
    int dumpAddressesInterval = 900; // dump addresses to peer.dat every 900 seconds (15 minutes)
    bool allowDNSSeeding = true; // allow DNS lookups when looking for seeds
    bool dnsSeedingExplicitlySet = false; // the -dnsseed parameter was explicitly used
    bool listen = true; // listen for p2p connections
    const uint32_t networkUpgragedPeerPreferenceBlockPeriod = 24 * 24 * 3; // period for preferring upgraded clients
    const int pingInterval = 2 * 60;
    /***
     * Port the service will run on. Defaults to 7770 (or 17770 for testnet).
     */
    uint16_t port = 7770;
    /*****
     * Maximum per-connection receive buffer, in kb. Defaults to 5000 (~5MB)
     */
    uint32_t maxReceiveBuffer = 5 * 1000;
    /***
     * Max size of send buffer, in kb. Defaults to 1000 (~1MB)
     */
    uint32_t maxSendBuffer = 1000;
    /** Time after which to disconnect, after waiting for a ping response (or inactivity). */
    const int timeoutInterval = 20 * 60;
    /** The maximum number of entries in an 'inv' protocol message */
    static const uint32_t MAX_INV_SZ = 50000;
    /** The maximum number of new addresses to accumulate before announcing. */
    const unsigned int MAX_ADDR_TO_SEND = 1000;
    /** The maximum number of entries in mapAskFor */
    const size_t MAPASKFOR_MAX_SZ = MAX_INV_SZ;
    /** The maximum number of entries in setAskFor (larger due to getdata latency)*/
    const size_t SETASKFOR_MAX_SZ = 2 * MAX_INV_SZ;    
    /** 
     * Maximum length of incoming protocol messages (no message over 2 MiB is currently acceptable). 
     * NOTE: kept static to satisfiy a precompiler requirement in main.h
     */
    static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = (_MAX_BLOCK_SIZE + 24); // 24 is msgheader size    
    /** Maximum length of strSubVer in `version` message */
    static const uint16_t MAX_SUBVERSION_LENGTH = 256;
    /** Subversion as sent to the P2P network in `version` messages */
    std::string subVersion;
    uint32_t banTimeSecs = 60*60*24; // default to 24 hour ban
    // these are undocumented and only used for testing
    uint16_t fuzzMessageTest = 0;
    uint16_t dropMessageTest = 0;
};
