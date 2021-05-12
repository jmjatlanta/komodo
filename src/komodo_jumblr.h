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
#pragma once

/*
 z_exportkey "zaddr"
 z_exportwallet "filename"
 z_getoperationstatus (["operationid", ... ])
 z_gettotalbalance ( minconf )
 z_importkey "zkey" ( rescan )
 z_importwallet "filename"
 z_listaddresses
 z_sendmany "fromaddress" [{"address":... ,"amount":..., "memo":"<hex>"},...] ( minconf ) ( fee )
 */

#include "komodo_defs.h"

struct jumblr_item
{
    UT_hash_handle hh;
    int64_t amount,fee,txfee; // fee and txfee not really used (yet)
    uint32_t spent,pad;
    char opid[66],src[128],dest[128],status;
};

char *jumblr_issuemethod(char *userpass,char *method,char *params,uint16_t port);

char *jumblr_importaddress(char *address);

char *jumblr_validateaddress(char *addr);

int32_t Jumblr_secretaddrfind(char *searchaddr);

int32_t Jumblr_secretaddradd(char *secretaddr);

int32_t Jumblr_depositaddradd(char *depositaddr);

int32_t Jumblr_secretaddr(char *secretaddr);

int32_t jumblr_addresstype(char *addr);

struct jumblr_item *jumblr_opidfind(char *opid);

struct jumblr_item *jumblr_opidadd(char *opid);

char *jumblr_zgetnewaddress();

char *jumblr_zlistoperationids();

char *jumblr_zgetoperationresult(char *opid);

char *jumblr_zgetoperationstatus(char *opid);

char *jumblr_sendt_to_z(char *taddr,char *zaddr,double amount);

char *jumblr_sendz_to_z(char *zaddrS,char *zaddrD,double amount);

char *jumblr_sendz_to_t(char *zaddr,char *taddr,double amount);

char *jumblr_zlistaddresses();

char *jumblr_zlistreceivedbyaddress(char *addr);

char *jumblr_getreceivedbyaddress(char *addr);

char *jumblr_importprivkey(char *wifstr);

char *jumblr_zgetbalance(char *addr);

char *jumblr_listunspent(char *coinaddr);

char *jumblr_gettransaction(char *txidstr);

int32_t jumblr_numvins(bits256 txid);

int64_t jumblr_receivedby(char *addr);

int64_t jumblr_balance(char *addr);

int32_t jumblr_itemset(struct jumblr_item *ptr,cJSON *item,char *status);

void jumblr_opidupdate(struct jumblr_item *ptr);

void jumblr_prune(struct jumblr_item *ptr);

void jumblr_zaddrinit(char *zaddr);

void jumblr_opidsupdate();

uint64_t jumblr_increment(uint8_t r,int32_t height,uint64_t total,uint64_t biggest,uint64_t medium, uint64_t smallest);

void jumblr_iteration();
