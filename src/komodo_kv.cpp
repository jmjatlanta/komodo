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
#include "komodo_kv.h"
#include "komodo_extern_globals.h"
#include "komodo_curve25519.h" // signing and verifying
#include "komodo_utils.h" // iguana_rwnum

/***
 * Compare two values
 * @param refvalue value A
 * @param refvaluesize the length of value A
 * @param value value B
 * @param valuesize the length of value B
 * @returns -1 if size mismatch or one of the values is null, 0 if both values are equal, < 0 if A < B, >0 if A > B
 */
int32_t KV::cmp(uint8_t *refvalue,uint16_t refvaluesize,uint8_t *value,uint16_t valuesize)
{
    if ( refvalue == 0 && value == 0 )
        return(0);
    else if ( refvalue == 0 || value == 0 )
        return(-1);
    else if ( refvaluesize != valuesize )
        return(-1);

    return memcmp(refvalue,value,valuesize);
}

/***
 * Retrieve number of days stored in flags
 * @param flags where the number of days is stored (using bitwise manipulation)
 * @returns number of days (max 365)
 */
int32_t KV::numdays(uint32_t flags)
{
    int32_t numdays = ((flags>>2)&0x3ff)+1;
    if (numdays > 365 )
        numdays = 365;
    return numdays;
}

/****
 * @brief get duration via flags and block counts
 * @param flags where number of days is stored
 * @returns the duration
 */
int32_t KV::duration(uint32_t flags)
{
    return(numdays(flags) * KOMODO_KVDURATION);
}

/*****
 * @brief calculate fee
 * @param flags to calculate number of days
 * @param opretlen the size of the data
 * @param keylen the key length
 * @returns the fee 
 */
uint64_t KV::fee(uint32_t flags,int32_t opretlen,int32_t keylen)
{
    if ( keylen > 32 )
        keylen = 32;

    int32_t num_days = numdays(flags);

    uint64_t fee = num_days*(opretlen * opretlen / keylen);
    if ( fee < 100000 )
        fee = 100000;

    return fee;
}

/*****
 * @brief search hash table for key
 * @param[out] pubkeyp the public key of the found value
 * @param[in] current_height current chain height (to check expiry)
 * @param[out] flagsp the flags of the found value
 * @param[out] heightp the height of the found value
 * @param[out] value the value
 * @param[in] key the key to search for
 * @param[in] keylen the key length
 * @returns -1 if not found, otherwise the size of `value`
 */
int32_t KV::search(uint256 *pubkeyp,int32_t current_height,uint32_t *flagsp,
        int32_t *heightp,uint8_t value[IGUANA_MAXSCRIPTSIZE],uint8_t *key,int32_t keylen)
{
    int32_t duration;
    int32_t retval = -1;

    *heightp = -1;
    *flagsp = 0;

    memset(pubkeyp,0,sizeof(*pubkeyp));
    std::lock_guard<std::mutex> lock(kv_mutex);
    komodo_kv *ptr;
    HASH_FIND(hh,KOMODO_KV,key,keylen,ptr);
    if ( ptr != 0 )
    {
        duration = this->duration(ptr->flags);
        if ( current_height > (ptr->height + duration) ) // entry expired, remove from collection
        {
            HASH_DELETE(hh,KOMODO_KV,ptr);
            if ( ptr->value != 0 )
                free(ptr->value);
            if ( ptr->key != 0 )
                free(ptr->key);
            free(ptr);
        }
        else
        {
            *heightp = ptr->height;
            *flagsp = ptr->flags;
            for (int32_t i = 0; i<32; i++)
            {
                ((uint8_t *)pubkeyp)[i] = ((uint8_t *)&ptr->pubkey)[31-i];
            }
            memcpy(pubkeyp,&ptr->pubkey,sizeof(*pubkeyp));
            if ( (retval= ptr->valuesize) > 0 )
                memcpy(value,ptr->value,retval);
        }
    }
    // TODO: If retval < 0, search rawmempool
    return(retval);
}

/****
 * @brief add or update a kv entry
 * @param opretbuf the entry to update
 * @param opretlen the length of `opretbuf`
 * @param value the fee
 */
void KV::update(uint8_t *opretbuf,int32_t opretlen,uint64_t value)
{
    if ( ASSETCHAINS_SYMBOL[0] == 0 ) // disable KV for KMD
        return;

    // break apart opretbuf
    uint16_t keylen;
    uint16_t valuesize;
    int32_t height;
    uint32_t flags;
    iguana_rwnum(0,&opretbuf[1],sizeof(keylen),&keylen);
    iguana_rwnum(0,&opretbuf[3],sizeof(valuesize),&valuesize);
    iguana_rwnum(0,&opretbuf[5],sizeof(height),&height);
    iguana_rwnum(0,&opretbuf[9],sizeof(flags),&flags);
    uint8_t *key = &opretbuf[13];
    if ( keylen+13 > opretlen ) // embedded keylength invalid
        return;
    uint8_t *valueptr = &key[keylen];
    if ( value >= this->fee(flags, opretlen, keylen) ) // the given fee covers the required fee
    {
        bool is_new_entry = false;
        int32_t coresize = (int32_t)(sizeof(flags)+sizeof(height)+sizeof(keylen)+sizeof(valuesize)+keylen+valuesize+1);
        if ( opretlen == coresize || opretlen == coresize+sizeof(uint256) || opretlen == coresize+2*sizeof(uint256) )
        {
            // opretlen seems to be the right size
            uint256 pubkey;
            uint256 sig;
            if ( opretlen >= coresize+sizeof(uint256) ) // do we have a public key?
            {
                for (uint8_t i = 0; i < 32; i++)
                    ((uint8_t *)&pubkey)[i] = opretbuf[coresize+i];
            }
            if ( opretlen == coresize+sizeof(uint256)*2 ) // Do we have a signature?
            {
                for (uint8_t i=0; i<32; i++)
                    ((uint8_t *)&sig)[i] = opretbuf[coresize+sizeof(uint256)+i];
            }
            uint8_t keyvalue[IGUANA_MAXSCRIPTSIZE*8]; 
            memcpy(keyvalue,key,keylen);
            uint256 refpubkey;
            int32_t kvheight;
            int32_t refvaluesize = search((uint256 *)&refpubkey,height,&flags,&kvheight,&keyvalue[keylen],key,keylen);
            if ( refvaluesize >= 0 )
            {
                static uint256 zeroes;
                if ( memcmp(&zeroes,&refpubkey,sizeof(refpubkey)) != 0 )
                {
                    if ( sigverify(keyvalue,keylen+refvaluesize,refpubkey,sig) < 0 )
                    {
                        return;
                    }
                }
            }
            std::lock_guard<std::mutex> lock(kv_mutex);
            komodo_kv *ptr;
            HASH_FIND(hh,KOMODO_KV,key,keylen,ptr);
            if ( ptr != nullptr )
            {
                {
                    char *tstr = (char *)"transfer:";
                    char *transferpubstr = (char *)&valueptr[strlen(tstr)];
                    if ( strncmp(tstr,(char *)valueptr,strlen(tstr)) == 0 && is_hexstr(transferpubstr,0) == 64 )
                    {
                        printf("transfer.(%s) to [%s]? ishex.%d\n",key,transferpubstr,is_hexstr(transferpubstr,0));
                        for (uint8_t i=0; i<32; i++)
                            ((uint8_t *)&pubkey)[31-i] = _decode_hex(&transferpubstr[i*2]);
                    }
                }
            }
            else // we did not find the komodo_kv entry, add it
            {
                ptr = (komodo_kv *)calloc(1,sizeof(*ptr));
                ptr->key = (uint8_t *)calloc(1,keylen);
                ptr->keylen = keylen;
                memcpy(ptr->key,key,keylen);
                is_new_entry = true;
                HASH_ADD_KEYPTR(hh,KOMODO_KV,ptr->key,ptr->keylen,ptr);
            }
            if ( is_new_entry || (ptr->flags & KOMODO_KVPROTECTED) == 0 )
            {
                if ( ptr->value != 0 )
                    free(ptr->value), ptr->value = 0;
                ptr->valuesize = valuesize;
                if ( ptr->valuesize != 0 )
                {
                    ptr->value = (uint8_t *)calloc(1,valuesize);
                    memcpy(ptr->value,valueptr,valuesize);
                }
            } 
            else 
                fprintf(stderr,"newflag.%s zero or protected %d\n",is_new_entry?"true":"false",
                        (ptr->flags & KOMODO_KVPROTECTED));
            memcpy(&ptr->pubkey,&pubkey,sizeof(ptr->pubkey));
            ptr->height = height;
            ptr->flags = flags; // jl777 used to or in KVPROTECTED
        } 
        else 
            fprintf(stderr,"KV update size mismatch %d vs %d\n",opretlen,coresize);
    } 
    else 
        fprintf(stderr,"not enough fee\n");
}

/****
 * @brief get public/private keys based on passed in information
 * @param[out] pubkeyp the public key
 * @param[in] passphrase the passphrase
 * @returns the private key
 */
uint256 KV::privkey(uint256 *pubkeyp,char *passphrase)
{
    uint256 privkey;
    conv_NXTpassword((uint8_t *)&privkey,(uint8_t *)pubkeyp,(uint8_t *)passphrase,(int32_t)strlen(passphrase));
    return privkey;
}

/***
 * @brief sign data with a private key
 * @param[in] buf the data to be signed
 * @param[in] len the length of the data in `buf`
 * @param[in] _privkey the private key
 * @returns the signature
 */
uint256 KV::sig(uint8_t *buf,int32_t len,uint256 _privkey)
{
    bits256 privkey;
    memcpy(&privkey,&_privkey,sizeof(privkey));

    bits256 hash;
    vcalc_sha256(0,hash.bytes,buf,len);

    bits256 otherpub = curve25519(hash,curve25519_basepoint9());
    bits256 pubkey = curve25519(privkey,curve25519_basepoint9());
    bits256 sig = curve25519_shared(privkey,otherpub);
    bits256 checksig = curve25519_shared(hash,pubkey);
    uint256 usig;
    memcpy(&usig,&sig,sizeof(usig));
    return usig;
}

/*****
 * @brief verify signature
 * @param buf the data that was signed
 * @param len the length of `buf`
 * @param _pubkey the signer's public key
 * @param sig the given signature
 * @returns 0 if signature matches, -1 otherwise
 */
bool KV::sigverify(uint8_t *buf,int32_t len,uint256 _pubkey,uint256 sig)
{
    static uint256 zeroes;

    bits256 pubkey;
    memcpy(&pubkey,&_pubkey,sizeof(pubkey));

    if ( memcmp(&pubkey,&zeroes,sizeof(pubkey)) != 0 ) // we have a legit pubkey
    {
        bits256 hash;
        vcalc_sha256(0,hash.bytes,buf,len);
        bits256 checksig;
        checksig = curve25519_shared(hash,pubkey);

        if ( memcmp(&checksig,&sig,sizeof(sig)) != 0 )
            return false;
    }
    else
        return false; // something wrong with pubkey
    return true;
}
