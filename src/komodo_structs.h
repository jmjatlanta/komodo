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

#include "komodo_defs.h"

#include "uthash.h"
#include "utlist.h"

/*#ifdef _WIN32
#define PACKED
#else
#define PACKED __attribute__((packed))
#endif*/

#ifndef KOMODO_STRUCTS_H
#define KOMODO_STRUCTS_H

#define GENESIS_NBITS 0x1f00ffff
#define KOMODO_MINRATIFY ((height < 90000) ? 7 : 11)
#define KOMODO_NOTARIES_HARDCODED 180000 // DONT CHANGE
#define KOMODO_MAXBLOCKS 250000 // DONT CHANGE

#define KOMODO_EVENT_RATIFY 'P'
#define KOMODO_EVENT_NOTARIZED 'N'
#define KOMODO_EVENT_KMDHEIGHT 'K'
#define KOMODO_EVENT_REWIND 'B'
#define KOMODO_EVENT_PRICEFEED 'V'
#define KOMODO_EVENT_OPRETURN 'R'
#define KOMODO_OPRETURN_DEPOSIT 'D'
#define KOMODO_OPRETURN_ISSUED 'I' // assetchain
#define KOMODO_OPRETURN_WITHDRAW 'W' // assetchain
#define KOMODO_OPRETURN_REDEEMED 'X'

#define KOMODO_KVPROTECTED 1
#define KOMODO_KVBINARY 2
#define KOMODO_KVDURATION 1440
#define KOMODO_ASSETCHAIN_MAXLEN 65

#ifndef _BITS256
#define _BITS256
    union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
    typedef union _bits256 bits256;
#endif    

union _bits320 { uint8_t bytes[40]; uint16_t ushorts[20]; uint32_t uints[10]; uint64_t ulongs[5]; uint64_t txid; };
typedef union _bits320 bits320;

struct komodo_kv { UT_hash_handle hh; bits256 pubkey; uint8_t *key,*value; int32_t height; uint32_t flags; uint16_t keylen,valuesize; };

struct pax_transaction
{
    UT_hash_handle hh;
    uint256 txid;
    uint64_t komodoshis,fiatoshis,validated;
    int32_t marked,height,otherheight,approved,didstats,ready;
    uint16_t vout;
    char symbol[KOMODO_ASSETCHAIN_MAXLEN],source[KOMODO_ASSETCHAIN_MAXLEN],coinaddr[64]; uint8_t rmd160[20],type,buf[35];
};

struct knotary_entry { UT_hash_handle hh; uint8_t pubkey[33],notaryid; };
struct knotaries_entry { int32_t height,numnotaries; struct knotary_entry *Notaries; };
struct notarized_checkpoint
{
    uint256 notarized_hash,notarized_desttxid,MoM,MoMoM;
    int32_t nHeight,notarized_height,MoMdepth,MoMoMdepth,MoMoMoffset,kmdstarti,kmdendi;
};

struct komodo_ccdataMoM
{
    uint256 MoM;
    int32_t MoMdepth,notarized_height,height,txi;
};

struct komodo_ccdata_entry { uint256 MoM; int32_t notarized_height,kmdheight,txi; char symbol[65]; };
struct komodo_ccdatapair { int32_t notarized_height,MoMoMoffset; };

struct komodo_ccdataMoMoM
{
    uint256 MoMoM;
    int32_t kmdstarti,kmdendi,MoMoMoffset,MoMoMdepth,numpairs,len;
    struct komodo_ccdatapair *pairs;
};

struct komodo_ccdata
{
    struct komodo_ccdata *next,*prev;
    struct komodo_ccdataMoM MoMdata;
    uint32_t CCid,len;
    char symbol[65];
};

/****
 * The komodo namespace will eventually house all komodo domain classes
 * For now, only event related classes are here.
 */

namespace komodo {

enum event_type
{
    EVENT_UNKNOWN,
    EVENT_RATIFY,
    EVENT_NOTARIZED,
    EVENT_KMDHEIGHT,
    EVENT_REWIND,
    EVENT_PRICEFEED,
    EVENT_OPRETURN,
    EVENT_PUBKEYS
};

/***
 * an event
 */
struct event
{
    event(event_type in) : type(in) {}
    struct komodo_event *related;
    uint16_t len;
    int32_t height;
    event_type type;
    uint8_t reorged;
    char symbol[KOMODO_ASSETCHAIN_MAXLEN];
    uint8_t* space;
    char to_id(event_type in)
    {
        switch(in)
        {
            case EVENT_RATIFY:
                return KOMODO_EVENT_RATIFY;
            case EVENT_NOTARIZED:
                return KOMODO_EVENT_NOTARIZED;
            case EVENT_KMDHEIGHT:
                return KOMODO_EVENT_KMDHEIGHT;
            case EVENT_REWIND:
                return KOMODO_EVENT_REWIND;
            case EVENT_PRICEFEED:
                return KOMODO_EVENT_PRICEFEED;
            case EVENT_OPRETURN:
                return KOMODO_EVENT_OPRETURN;
        }
        return '\0';
    }
    event_type to_event_type(char in)
    {
        switch (in)
        {
            case KOMODO_EVENT_RATIFY:
                return EVENT_RATIFY;
            case KOMODO_EVENT_NOTARIZED:
                return EVENT_NOTARIZED;
            case KOMODO_EVENT_KMDHEIGHT:
                return EVENT_KMDHEIGHT;
            case KOMODO_EVENT_REWIND:
                return EVENT_REWIND;
            case KOMODO_EVENT_PRICEFEED:
                return EVENT_PRICEFEED;
            case KOMODO_OPRETURN_DEPOSIT:
            case KOMODO_OPRETURN_ISSUED:
            case KOMODO_OPRETURN_REDEEMED:
            case KOMODO_OPRETURN_WITHDRAW:
                return EVENT_OPRETURN;
        }
        return EVENT_UNKNOWN;
    }
};

struct pricefeed_event : public event 
{
    pricefeed_event() : event(EVENT_PRICEFEED) {}
    uint8_t num; 
    uint32_t prices[35]; 
};

struct notarized_event : public event
{ 
    notarized_event(uint256 bh, uint256 txid, int32_t height, uint256 mom, int32_t depth, char* bytes) 
            : event(EVENT_NOTARIZED), blockhash(bh), desttxid(txid), 
            notarizedheight(height), MoM(mom), MoMdepth(depth) 
    {
        memcpy(dest, 0, 16);
        strncpy(dest, bytes, 15);
    }
    uint256 blockhash,desttxid,MoM; 
    int32_t notarizedheight,MoMdepth; 
    char dest[16]; 
};

struct pubkeys_event : public event 
{ 
    pubkeys_event() : event(EVENT_PUBKEYS) {}
    uint8_t num; 
    uint8_t pubkeys[64][33]; 
};

struct opreturn_event : public event
{ 
    enum opreturn_type
    {
        OPRETURN_DEPOSIT,
        OPRETURN_ISSUED,
        OPRETURN_WITHDRAW,
        OPRETURN_REDEEMED
    };
    opreturn_event() : event(EVENT_OPRETURN) {}
    uint256 txid; 
    uint64_t value; 
    uint16_t vout,oplen; 
    uint8_t* opret;
    opreturn_type return_type;
    char to_opreturn_id(opreturn_type in)
    {
        switch(in)
        {
            case OPRETURN_DEPOSIT:
                return KOMODO_OPRETURN_DEPOSIT;
            case OPRETURN_ISSUED:
                return KOMODO_OPRETURN_ISSUED;
            case OPRETURN_WITHDRAW:
                return KOMODO_OPRETURN_WITHDRAW;
            case OPRETURN_REDEEMED:
                return KOMODO_OPRETURN_REDEEMED;
        }
        return '\0';
    }
};

/****
 * Stores the current state
 */
struct state
{
    uint256 NOTARIZED_HASH,NOTARIZED_DESTTXID,MoM;
    int32_t SAVEDHEIGHT,CURRENT_HEIGHT,NOTARIZED_HEIGHT,MoMdepth;
    uint32_t SAVEDTIMESTAMP;
    uint64_t deposited,issued,withdrawn,approved,redeemed,shorted;
    struct notarized_checkpoint *NPOINTS; int32_t NUM_NPOINTS,last_NPOINTSi;
    std::list<std::shared_ptr<event>> events; // events in chronological order
    uint32_t RTbufs[64][3]; uint64_t RTmask;
    void add_event(std::shared_ptr<event> in)
    {
        if (ASSETCHAINS_SYMBOL[0] != 0)
        {
            std::lock_guard<std::mutex> lock(komodo_mutex);
            events.push_back(in);
        }
    }
};

} // namespace komodo

#endif /* KOMODO_STRUCTS_H */
