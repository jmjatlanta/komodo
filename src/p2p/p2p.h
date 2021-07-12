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
#include <boost/thread/thread.hpp>
#include <mutex>
#include <atomic>
#include "scheduler.h"
#include "limitedmap.h"
#include "p2p/addrman.h"
#include "p2p/p2p_parameters.h"
#include "p2p/node.h"
#include "chain_status.h"
#include "chainparams.h"

struct ListenSocket {
    SOCKET socket;
    bool whitelisted;

    ListenSocket(SOCKET socket, bool whitelisted) : socket(socket), whitelisted(whitelisted) {}
};

struct LocalServiceInfo {
    int nScore;
    int nPort;
};

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_UPNP,   // unused (was: address reported by UPnP)
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

/***
 * Manages p2p connectivity
 */
class P2P
{
public:
    /***
     * ctor
     */
    P2P(const P2PParameters& params, ChainStatus chainStatus);
    /***
     * Shuts down the network
     */
    ~P2P();
    /****
     * @returns the p2p parameters
     */
    const P2PParameters& GetParams() { return params; }
    const CChainParams GetChainParams() { return chainStatus.Params(); }
    /***
     * Does the heavy lifting to get the P2P network started
     * @param threadGroup the thread group
     * @param scheduler the scheduler
     */
    void StartNode(boost::thread_group& threadGroup, CScheduler& scheduler);
    /****
     * Add an address to the list of locals
     * @param addr the address
     * @param nScore the score
     * @returns true on success
     */
    bool AddLocal(const CService& addr, int nScore);
    /****
     * Add an address to the list of locals
     * @param addr the address (default port assumed)
     * @param nScore the score
     * @returns true on success
     */
    bool AddLocal(const CNetAddr &addr, int nScore);
    void ProcessOneShot();
    bool BindListenPort(const CService &addrBind, std::string& strError, bool fWhitelisted);
    /****
     * @brief Open a connection to a remote node
     * NOTE: if successful, this moves the passed grant to the constructed node
     * @param addrConnect the remote node address
     * @param grantOutbound
     * @param strDet the remote address as a string (i.e. "mydomain.com:2345")
     * @param fOneShot true to connect, retrieve the "getaddr" response, and disconnect
     * @returns true on success
     */
    bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound = NULL, 
            const char *strDest = NULL, bool fOneShot = false);
    CNode* FindNode(const CNetAddr& ip);
    CNode* FindNode(const CSubNet& subNet);
    CNode* FindNode(const std::string& addrName);
    CNode* FindNode(const CService& addr);
    size_t GetNumberConnected() { return nodes.size(); }
    std::vector<CNode*> GetNodes() { return nodes; }
    /****
     * Add a destination to the list of "one shot"s 
     * (will attempt a connect, retrieve response from "getaddr", and disconnect)
     * @param strDest the destination
     */
    void AddOneShot(const std::string& in);
    /****
     * Add a subnet to the whitelist
     * @param subnet the subnet
     */
    void AddWhitelistedRange(const CSubNet &subnet);
    /****
     * A node received a message that should be processed
     */
    void NotifyMessageReceived();
    /***
     * Retrieve the height
     * @returns the height
     */
    int GetHeight();
    /****
     * Retrieve the signals
     * @returns the signals
     */
    CNodeSignals& GetNodeSignals()
    {
        return g_signals;
    }
    /*****
     * Is our peer's addrLocal potentially useful as an external IP source?
     * (IsRoutable and !IsLimited)
     * @param pnode the node to examine
     * @returns true if pnode is a good node
     */
    bool IsPeerAddrLocalGood(CNode *pnode);
    /****
     * pushes our own address to a peer
     * @param pnode where to send our address
     */
    void AdvertizeLocal(CNode *pnode);
    /****
     * Sets a network's limited flag
     * @param net the network
     * @param fLimited true if this network should be marked limited
     */
    void SetLimited(enum Network net, bool fLimited = true);
    /****
     * Return a network's limited flag
     * @param net the network to examine
     * @returns true if the network is marked as limited
     */
    bool IsLimited(enum Network net);
    /*****
     * Return a network's limited flag
     * @param addr the address that belongs to a network
     * @returns true if the network is marked as limited
     */
    bool IsLimited(const CNetAddr& addr);
    /***
     * Remove an address from the list of locals
     * @param addr the address to remove
     * @returns true
     */
    bool RemoveLocal(const CService& addr);
    /****
     * Increase score for an address
     * @param addr the address
     * @returns false if address not found
     */
    bool SeenLocal(const CService& addr);

    /***
     * Determine if a particular network can probably be connected to
     * @param net the network
     * @returns true if the network can be connected to
     */
    bool IsReachable(enum Network net);
    /****
     * Determine if a particular address belongs to a network that 
     * can probably be connected to
     * @param addr the address
     * @returns true if the network of the address can probably be connected to
     */
    bool IsReachable(const CNetAddr &addr);
    /****
     * Get best local address for a particular peer as a CAddress
     * Otherwise, return the unroutable 0.0.0.0 but filled in with
     * the normal parameters, since the IP may be changed to a useful
     * one by discovery.
     */
    CAddress GetLocalAddress(const CNetAddr *paddrPeer = nullptr);
    /****
     * Tells the address manager that the service is connected
     * @param addr the service
     */
    void AddressCurrentlyConnected(const CService& addr);   
    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    void ClearBanned(); // needed for unit testing
    bool IsBanned(CNetAddr ip);
    bool IsBanned(CSubNet subnet);
    void Ban(const CNetAddr &ip, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    void Ban(const CSubNet &subNet, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    bool Unban(const CNetAddr &ip);
    bool Unban(const CSubNet &ip);
    void GetBanned(std::map<CSubNet, int64_t> &banmap); 
    /***
     * What incoming transmission size is considered a flood?
     * (can be controlled by -maxreceivebuffer command line option)
     * @returns the size in bytes
     */
    uint32_t ReceiveFloodSize();
    /***
     * Returns the max size of the send buffer
     * @returns the size in bytes
     */
    unsigned int SendBufferSize();    
    /*****
     * Gets the P2P listening port of the local node
     * @returns the local P2P port
     */
    uint16_t GetListenPort();

    uint64_t GetTotalBytesRecv();
    uint64_t GetTotalBytesSent();
    std::vector<CNodeStats> GetNodeStats();
    void RelayTransaction(const CTransaction& tx, const CDataStream& ss);
    void RelayTransaction(const CTransaction& tx);
    NodeId GetNextNodeId();
    /***
     * Add a node to the list of nodes to be connected to
     * @param in the address
     * @returns false if already exists in the list
     */
    bool AddNode(const std::string& in);
    /****
     * Remove a node from the list of nodes that should be connected to
     * @param in the address to be removed from the list
     * @returns false if address does not exist in the list
     */
    bool RemoveNode(const std::string& in);
    /****
     * Determine if a node exists in the list of nodes that should be connected to
     * @param in the address
     * @returns true if the address already exists in the collection
     */
    bool NodeExists(const std::string& in);
    /****
     * Retrieve the list of nodes that should be connected to
     * (normally added via --addnode on the command line)
     * @returns the addresses
     */
    std::set<std::string> GetAddedNodes();
public:
    std::mutex cs_mapLocalHost;
    std::map<CNetAddr, LocalServiceInfo> mapLocalHost;    
    CCriticalSection cs_vNodes; // allows async access to nodes collection
    limitedmap<CInv, int64_t> mapAlreadyAskedFor{P2PParameters::MAX_INV_SZ};       
    CAddrMan addrman;
    CCriticalSection cs_mapRelay;
    std::map<CInv, CDataStream> mapRelay;    
   
private:
    P2PParameters params;
    ChainStatus chainStatus;
    bool addressesInitialized = false; // if peers.dat has been read
    CSemaphore* semOutbound = nullptr; // semaphore to control outbound network traffic
    std::deque<std::string> vOneShots; // collections of addresses where we just want to query and hang up
    std::mutex cs_vOneShots; // safely access vOneShots asynchronously
    std::vector<ListenSocket> vhListenSocket; // (local) listen sockets
    std::vector<CNode*> nodes;
    std::list<CNode*> vNodesDisconnected; // nodes that have been disconnected
    std::set<CNetAddr> setservAddNodeAddresses;
    std::mutex cs_setservAddNodeAddresses;
    std::set<std::string> vAddedNodes;
    std::mutex cs_vAddedNodes;    
    CNodeSignals g_signals; // signals for message handling
    boost::condition_variable messageHandlerCondition; // triggered when a message is received
    // Whitelisted ranges: Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<CSubNet> vWhitelistedRange;
    std::mutex cs_vWhitelistedRange;    
    // Denial-of-service detection/prevention
    // Key is IP address, value is banned-until-time
    std::map<CSubNet, int64_t> setBanned;
    std::mutex cs_setBanned;
    bool vfLimited[NET_MAX] = {};   
    std::deque<std::pair<int64_t, CInv> > vRelayExpiration;  
    std::atomic<NodeId> lastNodeId;
private:
    void Discover(boost::thread_group& threadGroup);
    bool AttemptToEvictConnection(bool fPreferNewConnection);
    // async funcs set up by StartNode
    void ThreadDNSAddressSeed();
    void ThreadSocketHandler();    
    void ThreadOpenAddedConnections();
    void ThreadOpenConnections();
    void ThreadMessageHandler();
    void DumpAddresses();
    /***
     * Attempt to connect to a node
     * @param[in, out] addrConnect the address object (if filled in, indicates we have connected before)
     * @param[in] pszDest the address in string form (i.e. mydomain.com:2345)
     * @returns the node
     */
    CNode* ConnectNode(CAddress addrConnect, const char *pszDest = NULL);    
    /***
     * Determine if this address is within a whitelisted range
     * @param ip the address
     * @returns true if whitelisted
     */
    bool IsWhitelistedRange(const CNetAddr &ip);


    int GetnScore(const CService& addr);   
    /***
     * @param addr the address
     * @returns true if address is local
     */
    bool IsLocal(const CService& addr);
    /*****
     * Find the "best" local address to communicate with a destination
     * @param[out] addr the "best" address
     * @param[in] paddrPeer the destination (can be nullptr)
     * @returns true if found a local address
     */
    bool GetLocal(CService &addr, const CNetAddr *paddrPeer = NULL);
    /***
     * Listen on a particular socket
     * @param hListenSocket the socket
     */    
    void AcceptConnection(const ListenSocket& hListenSocket); 

};
