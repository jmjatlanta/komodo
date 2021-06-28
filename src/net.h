// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include "bloom.h"
#include "compat.h"
#include "hash.h"
#include "limitedmap.h"
#include "mruset.h"
#include "netbase.h"
#include "protocol.h"
#include "random.h"
#include "streams.h"
#include "sync.h"
#include "uint256.h"
#include "utilstrencodings.h"
#include "util.h"

#include <deque>
#include <stdint.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>
#include <boost/signals2/signal.hpp>

class CAddrMan;
class CBlockIndex;
class CScheduler;
class CNode;

namespace boost {
    class thread_group;
} // namespace boost

/** Time between pings automatically sent out for latency probing and keepalive (in seconds). */
static const int PING_INTERVAL = 2 * 60;
/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static const int TIMEOUT_INTERVAL = 20 * 60;
/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
/** The maximum number of new addresses to accumulate before announcing. */
static const unsigned int MAX_ADDR_TO_SEND = 1000;
/** Maximum length of incoming protocol messages (no message over 2 MiB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = (_MAX_BLOCK_SIZE + 24); // 24 is msgheader size
/** Maximum length of strSubVer in `version` message */
static const unsigned int MAX_SUBVERSION_LENGTH = 256;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** The maximum number of entries in mapAskFor */
static const size_t MAPASKFOR_MAX_SZ = MAX_INV_SZ;
/** The maximum number of entries in setAskFor (larger due to getdata latency)*/
static const size_t SETASKFOR_MAX_SZ = 2 * MAX_INV_SZ;
/** The maximum number of peer connections to maintain. */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 384;
/** The period before a network upgrade activates, where connections to upgrading peers are preferred (in blocks). */
static const int NETWORK_UPGRADE_PEER_PREFERENCE_BLOCK_PERIOD = 24 * 24 * 3;

/***
 * Returns the maximum bytes received by a node before it is considered a "flood"
 * @returns the size in bytes
 */
unsigned int ReceiveFloodSize();
/***
 * Returns the max size of the send buffer
 * @returns the size in bytes
 */
unsigned int SendBufferSize();
/****
 * Add a destination to the list of "one shot"s 
 * (will attempt a connect, retrieve response from "getaddr", and disconnect)
 * @param strDest the destination
 */
void AddOneShot(const std::string& strDest);
/****
 * Tells the address manager that the service is connected
 * @param addr the service
 */
void AddressCurrentlyConnected(const CService& addr);
/***
 * Finde the node related to the given IP address
 * @param ip the address
 * @returns the node (or nullptr if not found)
 */
CNode* FindNode(const CNetAddr& ip);
/****
 * Finds the first node of a given subnet
 * @param subNet the subnet
 * @returns the node (or nullptr if not found)
 */
CNode* FindNode(const CSubNet& subNet);
/***
 * Finds the node with the given name
 * @param addrName the name
 * @returns the node (or nullptr if not found)
 */
CNode* FindNode(const std::string& addrName);
/****
 * Finds the node related to the given CService
 * @param ip the CService
 * @returns the node (or nullptr if not found)
 */
CNode* FindNode(const CService& ip);
/***
 * Attempt to connect to a node
 * @param[in, out] addrConnect the address object (if filled in, indicates we have connected before)
 * @param[in] pszDest the address in string form (i.e. mydomain.com:2345)
 * @returns the node
 */
CNode* ConnectNode(CAddress addrConnect, const char *pszDest = NULL);
/****
 * @brief Open a connection to a remote node
 * NOTE: if successful, this moves the passed grant to the constructed node
 * @param addrConnect the remote node address
 * @param grantOutbound
 * @param strDet the remote address as a string (i.e. "mydomain.com:2345")
 * @param fOneShot true to connect, retrieve the "getaddr" response, and disconnect
 * @returns true on success
 */
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound = NULL, const char *strDest = NULL, bool fOneShot = false);
/*****
 * Gets the P2P listening port of the local node
 * @returns the local P2P port
 */
unsigned short GetListenPort();
/****
 * Bind this service onto a port
 * @param bindAddr the address (includes the port)
 * @param strError where to put any error message
 * @param fWhitelisted true if this socket is whitelisted
 * @returns true on success (false will include a message in strError)
 */
bool BindListenPort(const CService &bindAddr, std::string& strError, bool fWhitelisted = false);
/****
 * Starts the P2P service
 * @param threadGroup
 * @param scheduler
 */
void StartNode(boost::thread_group& threadGroup, CScheduler& scheduler);
/***
 * Stops the P2P service
 * @returns true
 */
bool StopNode();
/***
 * Send the data in the node's send buffer
 * @param pnode the node
 */
void SocketSendData(CNode *pnode);

typedef int NodeId;

class CNodeStats;
/***
 * Gather stats from all nodes
 * @param[out] where to put the results
 */
void CopyNodeStats(std::vector<CNodeStats>& vstats);

struct CombinerAll
{
    typedef bool result_type;

    template<typename I>
    bool operator()(I first, I last) const
    {
        while (first != last) {
            if (!(*first)) return false;
            ++first;
        }
        return true;
    }
};

// Signals for message handling
struct CNodeSignals
{
    boost::signals2::signal<int ()> GetHeight;
    boost::signals2::signal<bool (CNode*), CombinerAll> ProcessMessages;
    boost::signals2::signal<bool (CNode*, bool), CombinerAll> SendMessages;
    boost::signals2::signal<void (NodeId, const CNode*)> InitializeNode;
    boost::signals2::signal<void (NodeId)> FinalizeNode;
};

/***
 * Get the internal CNodeSignals struct
 * @returns the CNodeSignals struct as a reference
 */
CNodeSignals& GetNodeSignals();


enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_UPNP,   // unused (was: address reported by UPnP)
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

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
/****
 * Add an address to the list of locals
 * @param addr the address
 * @param nScore the score
 * @returns true on success
 */
bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
/****
 * Add an address to the list of locals
 * @param addr the address (default port assumed)
 * @param nScore the score
 * @returns true on success
 */
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
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
CAddress GetLocalAddress(const CNetAddr *paddrPeer = NULL);


extern bool fDiscover;
extern bool fListen;
extern uint64_t nLocalServices;
extern uint64_t nLocalHostNonce;
extern CAddrMan addrman;
/** Maximum number of connections to simultaneously allow (aka connection slots) */
extern int nMaxConnections;

extern std::vector<CNode*> vNodes;
extern CCriticalSection cs_vNodes;
extern std::map<CInv, CDataStream> mapRelay;
extern std::deque<std::pair<int64_t, CInv> > vRelayExpiration;
extern CCriticalSection cs_mapRelay;
extern limitedmap<CInv, int64_t> mapAlreadyAskedFor;

extern std::vector<std::string> vAddedNodes;
extern CCriticalSection cs_vAddedNodes;

extern NodeId nLastNodeId;
extern CCriticalSection cs_nLastNodeId;

/** Subversion as sent to the P2P network in `version` messages */
extern std::string strSubVersion;

struct LocalServiceInfo {
    int nScore;
    int nPort;
};

extern CCriticalSection cs_mapLocalHost;
extern std::map<CNetAddr, LocalServiceInfo> mapLocalHost;

class CNodeStats
{
public:
    NodeId nodeid;
    uint64_t nServices;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    int64_t nTimeOffset;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    int nStartingHeight;
    uint64_t nSendBytes;
    uint64_t nRecvBytes;
    bool fWhitelisted;
    double dPingTime;
    double dPingWait;
    std::string addrLocal;
    // Address of this peer
    CAddress addr;
    // Bind address of our side of the connection
    // CAddress addrBind; // https://github.com/bitcoin/bitcoin/commit/a7e3c2814c8e49197889a4679461be42254e5c51
    uint32_t m_mapped_as;
};

class CNetMessage {
public:
    bool in_data;                   // parsing header (false) or data (true)

    CDataStream hdrbuf;             // partially received header
    CMessageHeader hdr;             // complete header
    unsigned int nHdrPos;

    CDataStream vRecv;              // received message data
    unsigned int nDataPos;

    int64_t nTime;                  // time (in microseconds) of message receipt.

    CNetMessage(const CMessageHeader::MessageStartChars& pchMessageStartIn, int nTypeIn, int nVersionIn) : hdrbuf(nTypeIn, nVersionIn), hdr(pchMessageStartIn), vRecv(nTypeIn, nVersionIn) {
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        nTime = 0;
    }

    bool complete() const
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }

    void SetVersion(int nVersionIn)
    {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }

    int readHeader(const char *pch, unsigned int nBytes);
    int readData(const char *pch, unsigned int nBytes);
};

/** Information about a peer */
class CNode
{
public:
    // socket
    uint64_t nServices;
    SOCKET hSocket;
    CDataStream ssSend;
    size_t nSendSize; // total size of all vSendMsg entries
    size_t nSendOffset; // offset inside the first vSendMsg already sent
    uint64_t nSendBytes;
    std::deque<CSerializeData> vSendMsg;
    CCriticalSection cs_vSend;

    std::deque<CInv> vRecvGetData;
    std::deque<CNetMessage> vRecvMsg;
    CCriticalSection cs_vRecvMsg;
    uint64_t nRecvBytes;
    int nRecvVersion;

    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    int64_t nTimeOffset;
    uint32_t prevtimes[16];
    // Address of this peer
    CAddress addr;
    // Bind address of our side of the connection
    // const CAddress addrBind; // https://github.com/bitcoin/bitcoin/commit/a7e3c2814c8e49197889a4679461be42254e5c51
    std::string addrName;
    CService addrLocal;
    int nVersion;
    int lasthdrsreq,sendhdrsreq;
    // strSubVer is whatever byte array we read from the wire. However, this field is intended
    // to be printed out, displayed to humans in various forms and so on. So we sanitize it and
    // store the sanitized version in cleanSubVer. The original should be used when dealing with
    // the network or wire types and the cleaned string used when displayed or logged.
    std::string strSubVer, cleanSubVer;
    bool fWhitelisted; // This peer can bypass DoS banning.
    bool fOneShot;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fSuccessfullyConnected;
    bool fDisconnect;
    // We use fRelayTxes for two purposes -
    // a) it allows us to not relay tx invs before receiving the peer's version message
    // b) the peer may tell us in its version message that we should not relay tx invs
    //    until it has initialized its bloom filter.
    bool fRelayTxes;
    bool fSentAddr;
    CSemaphoreGrant grantOutbound;
    CCriticalSection cs_filter;
    CBloomFilter* pfilter;
    int nRefCount;
    NodeId id;
protected:

    // Denial-of-service detection/prevention
    // Key is IP address, value is banned-until-time
    static std::map<CSubNet, int64_t> setBanned;
    static CCriticalSection cs_setBanned;

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    static std::vector<CSubNet> vWhitelistedRange;
    static CCriticalSection cs_vWhitelistedRange;

    // Basic fuzz-testing
    void Fuzz(int nChance); // modifies ssSend

public:
    uint256 hashContinue;
    int nStartingHeight;

    // flood relay
    std::vector<CAddress> vAddrToSend;
    CRollingBloomFilter addrKnown;
    bool fGetAddr;
    std::set<uint256> setKnown;

    // inventory based relay
    mruset<CInv> setInventoryKnown;
    std::vector<CInv> vInventoryToSend;
    CCriticalSection cs_inventory;
    std::set<uint256> setAskFor;
    std::multimap<int64_t, CInv> mapAskFor;

    // Ping time measurement:
    // The pong reply we're expecting, or 0 if no pong expected.
    uint64_t nPingNonceSent;
    // Time (in usec) the last ping was sent, or 0 if no ping was ever sent.
    int64_t nPingUsecStart;
    // Last measured round-trip time.
    int64_t nPingUsecTime;
    // Best measured round-trip time.
    int64_t nMinPingUsecTime;
    // Whether a ping is requested.
    bool fPingQueued;

    CNode(SOCKET hSocketIn, const CAddress &addrIn, const std::string &addrNameIn = "", bool fInboundIn = false);
    ~CNode();

private:
    // Network usage totals
    static CCriticalSection cs_totalBytesRecv;
    static CCriticalSection cs_totalBytesSent;
    static uint64_t nTotalBytesRecv;
    static uint64_t nTotalBytesSent;

    CNode(const CNode&);
    void operator=(const CNode&);

public:

    NodeId GetId() const {
      return id;
    }

    int GetRefCount()
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    // requires LOCK(cs_vRecvMsg)
    unsigned int GetTotalRecvSize()
    {
        unsigned int total = 0;
        BOOST_FOREACH(const CNetMessage &msg, vRecvMsg)
            total += msg.vRecv.size() + 24;
        return total;
    }

    // requires LOCK(cs_vRecvMsg)
    bool ReceiveMsgBytes(const char *pch, unsigned int nBytes);

    // requires LOCK(cs_vRecvMsg)
    void SetRecvVersion(int nVersionIn)
    {
        nRecvVersion = nVersionIn;
        BOOST_FOREACH(CNetMessage &msg, vRecvMsg)
            msg.SetVersion(nVersionIn);
    }

    CNode* AddRef()
    {
        nRefCount++;
        return this;
    }

    void Release()
    {
        nRefCount--;
    }



    void AddAddressKnown(const CAddress& addr)
    {
        addrKnown.insert(addr.GetKey());
    }

    void PushAddress(const CAddress& addr)
    {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        if (addr.IsValid() && !addrKnown.contains(addr.GetKey())) {
            if (vAddrToSend.size() >= MAX_ADDR_TO_SEND) {
                vAddrToSend[insecure_rand() % vAddrToSend.size()] = addr;
            } else { 
                vAddrToSend.push_back(addr);
            }
        }
    }


    void AddInventoryKnown(const CInv& inv)
    {
        {
            LOCK(cs_inventory);
            setInventoryKnown.insert(inv);
        }
    }

    void PushInventory(const CInv& inv)
    {
        {
            LOCK(cs_inventory);
            if (!setInventoryKnown.count(inv))
                vInventoryToSend.push_back(inv);
        }
    }

    void AskFor(const CInv& inv);

    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void BeginMessage(const char* pszCommand) EXCLUSIVE_LOCK_FUNCTION(cs_vSend);

    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void AbortMessage() UNLOCK_FUNCTION(cs_vSend);

    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void EndMessage() UNLOCK_FUNCTION(cs_vSend);

    void PushVersion();


    void PushMessage(const char* pszCommand)
    {
        //fprintf(stderr,"push.(%s)\n",pszCommand);
        try
        {
            BeginMessage(pszCommand);
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1>
    void PushMessage(const char* pszCommand, const T1& a1)
    {
        //fprintf(stderr,"push.(%s)\n",pszCommand);
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8, const T9& a9)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    void CloseSocketDisconnect();

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
    static void ClearBanned(); // needed for unit testing
    static bool IsBanned(CNetAddr ip);
    static bool IsBanned(CSubNet subnet);
    static void Ban(const CNetAddr &ip, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    static void Ban(const CSubNet &subNet, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    static bool Unban(const CNetAddr &ip);
    static bool Unban(const CSubNet &ip);
    static void GetBanned(std::map<CSubNet, int64_t> &banmap);

    void copyStats(CNodeStats &stats, const std::vector<bool> &m_asmap);

    static bool IsWhitelistedRange(const CNetAddr &ip);
    static void AddWhitelistedRange(const CSubNet &subnet);

    // Network stats
    static void RecordBytesRecv(uint64_t bytes);
    static void RecordBytesSent(uint64_t bytes);

    static uint64_t GetTotalBytesRecv();
    static uint64_t GetTotalBytesSent();
};

class CTransaction;
void RelayTransaction(const CTransaction& tx);
void RelayTransaction(const CTransaction& tx, const CDataStream& ss);

/** Access to the (IP) address database (peers.dat) */
class CAddrDB
{
private:
    boost::filesystem::path pathAddr;
public:
    CAddrDB();
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
};

#endif // BITCOIN_NET_H
