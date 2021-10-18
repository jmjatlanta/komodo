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
#include "komodo_jumblr.h"
#include "komodo_extern_globals.h"
#include "komodo_bitcoind.h" // komodo_issuemethod
#include "komodo_utils.h" // clonestr

char Jumblr_secretaddrs[JUMBLR_MAXSECRETADDRS][64],Jumblr_deposit[64];
int32_t Jumblr_numsecretaddrs; // if 0 -> run silent mode
jumblr_item *Jumblrs;

/*****
 * @brief make an RPC style method call
 * @param userpass credentials
 * @param method the method to call
 * @param params the method parameters (nullptr ok)
 * @param port the IP port
 * @returns the results in JSON format
 */
char *jumblr_issuemethod(const char *userpass,const char *method,const std::string& params,uint16_t port)
{
    cJSON *retjson,*resjson = 0; char *retstr;
    if ( (retstr= komodo_issuemethod(userpass,method,params.c_str(),port)) != 0 )
    {
        if ( (retjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( jobj(retjson,(char *)"result") != 0 )
                resjson = jduplicate(jobj(retjson,(char *)"result"));
            else if ( jobj(retjson,(char *)"error") != 0 )
                resjson = jduplicate(jobj(retjson,(char *)"error"));
            else
            {
                resjson = cJSON_CreateObject();
                jaddstr(resjson,(char *)"error",(char *)"cant parse return");
            }
            free_json(retjson);
        }
        free(retstr);
    }
    if ( resjson != 0 )
        return(jprint(resjson,1));
    else return(clonestr((char *)"{\"error\":\"unknown error\"}"));
}

char *jumblr_importaddress(char *address)
{
    std::string params = std::string("[\"") + address + "\", \"" + address + "\", false]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"importaddress",params,BITCOIND_RPCPORT);
}

char *jumblr_validateaddress(char *addr)
{
    std::string params = std::string("[\"") + addr + "\"]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"validateaddress",params,BITCOIND_RPCPORT));
}

int32_t Jumblr_secretaddrfind(char *searchaddr)
{
    for ( int32_t i=0; i<Jumblr_numsecretaddrs; i++)
    {
        if ( strcmp(searchaddr,Jumblr_secretaddrs[i]) == 0 )
            return i;
    }
    return -1;
}

int32_t Jumblr_secretaddradd(char *secretaddr) // external
{
    if ( secretaddr != 0 && secretaddr[0] != 0 )
    {
        if ( Jumblr_numsecretaddrs < JUMBLR_MAXSECRETADDRS )
        {
            if ( strcmp(Jumblr_deposit,secretaddr) != 0 ) // must be diferent than deposit address
            {
                int32_t ind;
                if ( (ind= Jumblr_secretaddrfind(secretaddr)) < 0 )
                {
                    // address does not already exist
                    ind = Jumblr_numsecretaddrs++;
                    safecopy(Jumblr_secretaddrs[ind],secretaddr,64);
                }
                return ind;
            } 
            else 
                return JUMBLR_ERROR_SECRETCANTBEDEPOSIT;
        } 
        else 
            return JUMBLR_ERROR_TOOMANYSECRETS;
    }
    else
    {
        memset(Jumblr_secretaddrs,0,sizeof(Jumblr_secretaddrs));
        Jumblr_numsecretaddrs = 0;
    }
    return Jumblr_numsecretaddrs;
}

int32_t Jumblr_depositaddradd(char *depositaddr)
{
    int32_t retval = JUMBLR_ERROR_DUPLICATEDEPOSIT; 

    if ( depositaddr == 0 )
        depositaddr = (char *)"";

    if ( Jumblr_secretaddrfind(depositaddr) < 0 )
    {
        char *retstr = jumblr_validateaddress(depositaddr);
        if ( retstr != nullptr )
        {
            cJSON *retjson = cJSON_Parse(retstr);
            if ( retjson != nullptr )
            {
                cJSON *ismine = jobj(retjson,(char *)"ismine");
                if ( ismine != nullptr && cJSON_IsTrue(ismine) != 0 )
                {
                    retval = 0;
                    safecopy(Jumblr_deposit,depositaddr,sizeof(Jumblr_deposit));
                }
                else
                {
                    retval = JUMBLR_ERROR_NOTINWALLET;
                    printf("%s not in wallet: ismine.%p %d %s\n",depositaddr,ismine,cJSON_IsTrue(ismine),jprint(retjson,0));
                }
                free_json(retjson);
            }
            free(retstr);
        }
    }
    return retval;
}

#ifdef _WIN32
void OS_randombytes(unsigned char *x,long xlen)
{
    HCRYPTPROV prov = 0;
    CryptAcquireContextW(&prov, NULL, NULL,PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    CryptGenRandom(prov, xlen, x);
    CryptReleaseContext(prov, 0);
}
#else
void OS_randombytes(unsigned char *x,long xlen);
#endif

/*****
 * @brief generate a secret address
 * @pre Requires secret addresses already stored
 * @param[out] secretaddr where to store the result
 * @returns the index of the address selected
 */
int32_t Jumblr_secretaddr(char *secretaddr)
{
    uint32_t r;
    if ( Jumblr_numsecretaddrs > 0 )
    {
        OS_randombytes((uint8_t *)&r,sizeof(r));
        r %= Jumblr_numsecretaddrs;
        safecopy(secretaddr,Jumblr_secretaddrs[r],64);
    }
    return r;
}

/****
 * @brief determine the address type
 * @param addr the address to examine
 * @returns 'z', 't', or -1 on error
 */
int32_t jumblr_addresstype(char *addr)
{
    // strip off double quotes
    if ( addr[0] == '"' && addr[strlen(addr)-1] == '"' )
    {
        addr[strlen(addr)-1] = 0;
        addr++;
    }

    if ( addr[0] == 'z' && addr[1] == 'c' && strlen(addr) >= 40 )
        return 'z';
    else if ( strlen(addr) < 40 )
        return 't';

    return -1;
}

/*****
 * @brief search for a jumblr item by opid
 * @param opid
 * @returns the item (or nullptr)
 */
jumblr_item *jumblr_opidfind(char *opid)
{
    jumblr_item *ptr;
    HASH_FIND(hh,Jumblrs,opid,(int32_t)strlen(opid),ptr);
    return ptr;
}

/****
 * @brief add (or find existing) opid to the hashtable
 * @param opid the opid to add
 * @returns the full entry
 */
jumblr_item *jumblr_opidadd(char *opid)
{
    jumblr_item *ptr = nullptr;
    if ( opid != nullptr && (ptr= jumblr_opidfind(opid)) == 0 )
    {
        // not found, so add it
        ptr = (jumblr_item *)calloc(1,sizeof(*ptr));
        safecopy(ptr->opid,opid,sizeof(ptr->opid));
        HASH_ADD_KEYPTR(hh,Jumblrs,ptr->opid,(int32_t)strlen(ptr->opid),ptr);
        if ( ptr != jumblr_opidfind(opid) )
            printf("jumblr_opidadd.(%s) ERROR, couldnt find after add\n",opid);
    }
    return ptr;
}

/*****
 * @brief get a new address
 * @note makes an RPC call
 * @returns the new address
 */
char *jumblr_zgetnewaddress()
{
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_getnewaddress",nullptr,BITCOIND_RPCPORT);
}

/*****
 * @brief get a list of operation ids
 * @note makes an RPC call
 * @returns a list of operation ids
 */
char *jumblr_zlistoperationids()
{
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_listoperationids",nullptr,BITCOIND_RPCPORT);
}

/*****
 * @brief Retrieve result and status of an operation which has finished, and then remove the operation from memory
 * @note makes an RPC call
 * @param opid the operation id
 * @returns an operation result
 */
char *jumblr_zgetoperationresult(char *opid)
{
    std::string params = std::string("[[\"") + opid + "\"]]";
    return jumblr_issuemethod( KMDUSERPASS, (char *)"z_getoperationresult",params,BITCOIND_RPCPORT);
}

/*****
 * @brief get a status of an op id
 * @note makes an RPC call
 * @returns the status
 */
char *jumblr_zgetoperationstatus(char *opid)
{
    std::string params = std::string("[[\"") + opid + "\"]]";
    return jumblr_issuemethod( KMDUSERPASS,(char *)"z_getoperationstatus",params,BITCOIND_RPCPORT);
}

/*****
 * @param amount the amount
 * @returns a float as a string in the format .8f
 */
std::string toFloatString(double amount)
{
    std::stringstream ss;
    ss << std::fixed << std::precision(8) << amount;
    return ss.str();
}

/****
 * @brief send t -> z
 * @param taddr the t address
 * @param zaddr the z address
 * @param amount the amount to send
 * @returns the response
 */
char *jumblr_sendt_to_z(char *taddr,char *zaddr,double amount)
{
    if ( jumblr_addresstype(zaddr) != 'z' || jumblr_addresstype(taddr) != 't' )
        return clonestr((char *)"{\"error\":\"illegal address in t to z\"}");

    double fee = ((amount-3*JUMBLR_TXFEE) * JUMBLR_FEE) * 1.5;
    std::string params = std::string("[\"")
            + taddr + "\", [{\"address\":\""
            + zaddr * "\",\"amount\":"
            + toFloatString(amount-fee-JUMBLR_TXFEE) + "}, {\"address\":\""
            + JUMBLR_ADDR + "\",\"amount\":"
            + toFloatString(fee) + "}], 1, "
            + toFloatString(JUMBLR_TXFEE) + "]";
    printf("t -> z: %s\n",params.c_str());
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_sendmany",params,BITCOIND_RPCPORT));
}

/******
 * @brief send z -> z
 * @param zaddrS the source address
 * @param zaddrD the destination address
 * @param amount the amount
 * @returns the response
 */
char *jumblr_sendz_to_z(char *zaddrS,char *zaddrD,double amount)
{
    if ( jumblr_addresstype(zaddrS) != 'z' || jumblr_addresstype(zaddrD) != 'z' )
        return clonestr((char *)"{\"error\":\"illegal address in z to z\"}");

    double fee = (amount-2*JUMBLR_TXFEE) * JUMBLR_FEE;
    std::string params = std::string("[\""
            + zaddrS + "\", [{\"address\":\""
            + zaddrD + "\",\"amount\":"
            + toFloatString(amount-fee-JUMBLR_TXFEE) + "}], 1, "
            + toFloatString(JUMBLR_TXFEE) + "]";
    printf("z -> z: %s\n",params.c_str());
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_sendmany",params,BITCOIND_RPCPORT);
}

/******
 * @brief send z -> t
 * @param zaddr the source address
 * @param taddr the destination address
 * @param amount the amount
 * @returns the response
 */
char *jumblr_sendz_to_t(char *zaddr,char *taddr,double amount)
{
    if ( jumblr_addresstype(zaddr) != 'z' || jumblr_addresstype(taddr) != 't' )
        return clonestr((char *)"{\"error\":\"illegal address in z to t\"}");

    double fee = ((amount-JUMBLR_TXFEE) * JUMBLR_FEE) * 1.5;
    std::string params = std::string("[\"")
            + zaddr + "\", [{\"address\":\""
            + taddr + "\",\"amount\":"
            + toFloatString(amount-fee-JUMBLR_TXFEE) + "}, {\"address\":\""
            + JUMBLR_ADDR + "\",\"amount\":"
            + toFloatString(fee) + "}], 1, "
            + toFloatString(JUMBLR_TXFEE) + "]";
    printf("z -> t: %s\n",params.c_str());
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_sendmany",params,BITCOIND_RPCPORT);
}

/*******
 * @brief list addresses
 * @returns list of addresses
 */
char *jumblr_zlistaddresses()
{
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_listaddresses",nullptr,BITCOIND_RPCPORT);
}

/*******
 * @brief list recceived by addresses
 * @param addr the address to look for
 * @returns list of addresses
 */
char *jumblr_zlistreceivedbyaddress(char *addr)
{
    std::string params = std::string("[\"") + addr + "\", 1]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_listreceivedbyaddress",params,BITCOIND_RPCPORT);
}

/*******
 * @brief get the amount recceived by an address
 * @param addr the address to look for
 * @returns the amount
 */
char *jumblr_getreceivedbyaddress(char *addr)
{
    std::string params = std::string("[\"") + addr + "\", 1]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"getreceivedbyaddress",params,BITCOIND_RPCPORT);
}

/*******
 * @brief Import a private key
 * @param wifstr the key in WIF format
 * @returns the response
 */
char *jumblr_importprivkey(char *wifstr)
{
    std::string params = std::string("[\"") + wifstr + "\", \"\", false]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"importprivkey",params,BITCOIND_RPCPORT);
}

/*******
 * @param addr the address to check
 * @returns the balance
 */
char *jumblr_zgetbalance(char *addr)
{
    std::string params = std::string("[\"") + addr + "\", 1]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"z_getbalance",params,BITCOIND_RPCPORT);
}

/*******
 * @param coinaddr the address to check
 * @returns the unspent UTXOs
 */
char *jumblr_listunspent(char *coinaddr)
{
    std::string params = std::string("[1, 99999999, [\"") + coinaddr + "\"]]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"listunspent",params,BITCOIND_RPCPORT);
}

/*******
 * @param txidstr the transaction id (as string)
 * @returns the transaction details
 */
char *jumblr_gettransaction(char *txidstr)
{
    std::string params = std::string("[\"") + txidstr + "\", 1]";
    return jumblr_issuemethod(KMDUSERPASS,(char *)"getrawtransaction",params,BITCOIND_RPCPORT);
}

/*******
 * @param txid the transaction id
 * @returns the number of vIns within that transaction
 */
int32_t jumblr_numvins(bits256 txid)
{
    char txidstr[65],params[1024],*retstr; cJSON *retjson,*vins; int32_t n,numvins = -1;
    bits256_str(txidstr,txid);
    if ( (retstr= jumblr_gettransaction(txidstr)) != 0 )
    {
        if ( (retjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( jobj(retjson,(char *)"vin") != 0 && ((vins= jarray(&n,retjson,(char *)"vin")) == 0 || n == 0) )
            {
                numvins = n;
            }
            free_json(retjson);
        }
        free(retstr);
    }
    return(numvins);
}

/*****
 * @brief get the amount received by an address
 * @param addr the address
 * @returns the amount in SATOSHIs
 */
int64_t jumblr_receivedby(char *addr)
{
    char *retstr; int64_t total = 0;
    if ( (retstr= jumblr_getreceivedbyaddress(addr)) != 0 )
    {
        total = atof(retstr) * SATOSHIDEN;
        free(retstr);
    }
    return(total);
}

/*****
 * @param the address
 * @returns the balance at the address
 */
int64_t jumblr_balance(char *addr)
{
    char *retstr = jumblr_zgetbalance(addr);
    if ( retstr != nullptr )
    {
        double val = atof(retstr);
        int64_t balance = 0;
        if ( val > SMALLVAL )
            balance = val * SATOSHIDEN;
        free(retstr);
        return balance;
    }
    return 0;
}

/*****
 * @brief parse JSON into the jumblr_item struct
 * @param[out] ptr the item
 * @param[in] item jumblr_item as JSON
 * @param status
 * @returns 1
 */
int32_t jumblr_itemset(jumblr_item *ptr, cJSON *item, char *status)
{
    /*
        "params" : {
        "fromaddress" : "RDhEGYScNQYetCyG75Kf8Fg61UWPdwc1C5",
        "amounts" : [
        {
        "address" : "zc9s3UdkDFTnnwHrMCr1vYy2WmkjhmTxXNiqC42s7BjeKBVUwk766TTSsrRPKfnX31Bbu8wbrTqnjDqskYGwx48FZMPHvft",
        "amount" : 3.00000000
        }
        ],
        "minconf" : 1,
        "fee" : 0.00010000
        }
    */
    cJSON *params = jobj(item, (char*)"params");
    if ( params != nullptr )
    {
        char *from = jstr(params, (char*)"fromaddress");
        if ( from != nullptr )
        {
            safecopy(ptr->src,from,sizeof(ptr->src));
        }
        int32_t n;
        cJSON *amounts = jarray(&n, params, (char*)"amounts");
        if ( amounts != nullptr )
        {
            for (int32_t i=0; i<n; i++)
            {
                cJSON *dest = jitem(amounts,i);
                char *addr = jstr(dest, (char*)"address");
                int64_t amount;
                if ( addr != nullptr && (amount= jdouble(dest,(char *)"amount")*SATOSHIDEN) > 0 )
                {
                    if ( strcmp(addr,JUMBLR_ADDR) == 0 )
                        ptr->fee = amount;
                    else
                    {
                        ptr->amount = amount;
                        safecopy(ptr->dest,addr,sizeof(ptr->dest));
                    }
                }
            }
        }
        ptr->txfee = jdouble(params,(char *)"fee") * SATOSHIDEN;
    }
    return 1;
}

/*****
 * @brief update the jumblr object
 * @note jumblr_item.opid used for lookup
 * @param ptr the object
 */
void jumblr_opidupdate(jumblr_item *ptr)
{
    if ( ptr->status == 0 )
    {
        char *retstr = jumblr_zgetoperationstatus(ptr->opid); 
        if ( retstr != nullptr )
        {
            cJSON *retjson = cJSON_Parse(retstr);
            if ( retjson != nullptr )
            {
                if ( cJSON_GetArraySize(retjson) == 1 && cJSON_IsArray(retjson) != 0 )
                {
                    cJSON *item = jitem(retjson,0);
                    char *status = jstr(item,(char*)"status");
                    if ( status != nullptr )
                    {
                        if ( strcmp(status,(char *)"success") == 0 )
                        {
                            ptr->status = jumblr_itemset(ptr,item,status);
                            if ( (jumblr_addresstype(ptr->src) == 't' 
                                     && jumblr_addresstype(ptr->src) == 'z'
                                     && strcmp(ptr->src,Jumblr_deposit) != 0) 
                                    || (jumblr_addresstype(ptr->src) == 'z' 
                                     && jumblr_addresstype(ptr->src) == 't' 
                                     && Jumblr_secretaddrfind(ptr->dest) < 0) )
                            {
                                printf("a non-jumblr t->z pruned\n");
                                free(jumblr_zgetoperationresult(ptr->opid));
                                ptr->status = -1;
                            }

                        }
                        else if ( strcmp(status,(char *)"failed") == 0 )
                        {
                            printf("jumblr_opidupdate %s failed\n",ptr->opid);
                            free(jumblr_zgetoperationresult(ptr->opid));
                            ptr->status = -1;
                        }
                    }
                }
                free_json(retjson);
            }
            free(retstr);
        }
    }
}

/*****
 */
void jumblr_prune(jumblr_item *ptr)
{
    if ( is_hexstr(ptr->opid,0) == 64 )
        return; // opid not formatted correctly
    
    std::string oldsrc(ptr->src); // save the old src

    free(jumblr_zgetoperationresult(ptr->opid)); // updates object
    jumblr_item *tmp;
    bool should_continue = true;
    while ( should_continue )
    {
        should_continue = false;
        HASH_ITER(hh,Jumblrs,ptr,tmp)
        {
            if ( oldsrc == ptr->dest )
            {
                if ( is_hexstr(ptr->opid,0) != 64 )
                {
                    free(jumblr_zgetoperationresult(ptr->opid));
                    oldsrc = ptr->src;
                    should_continue = true;
                    break;
                }
            }
        }
    }
}

bits256 jbits256(cJSON *json,char *field);

void jumblr_zaddrinit(char *zaddr)
{
    struct jumblr_item *ptr; char *retstr,*totalstr; cJSON *item,*array; double total; bits256 txid; char txidstr[65],t_z,z_z;
    if ( (totalstr= jumblr_zgetbalance(zaddr)) != 0 )
    {
        if ( (total= atof(totalstr)) > SMALLVAL )
        {
            if ( (retstr= jumblr_zlistreceivedbyaddress(zaddr)) != 0 )
            {
                if ( (array= cJSON_Parse(retstr)) != 0 )
                {
                    t_z = z_z = 0;
                    if ( cJSON_GetArraySize(array) == 1 && cJSON_IsArray(array) != 0 )
                    {
                        item = jitem(array,0);
                        if ( (uint64_t)((total+0.0000000049) * SATOSHIDEN) == (uint64_t)((jdouble(item,(char *)"amount")+0.0000000049) * SATOSHIDEN) )
                        {
                            txid = jbits256(item,(char *)"txid");
                            bits256_str(txidstr,txid);
                            if ( (ptr= jumblr_opidadd(txidstr)) != 0 )
                            {
                                ptr->amount = (total * SATOSHIDEN);
                                ptr->status = 1;
                                strcpy(ptr->dest,zaddr);
                                if ( jumblr_addresstype(ptr->dest) != 'z' )
                                    printf("error setting dest type to Z: %s\n",jprint(item,0));
                                if ( jumblr_numvins(txid) == 0 )
                                {
                                    z_z = 1;
                                    strcpy(ptr->src,zaddr);
                                    ptr->src[3] = '0';
                                    ptr->src[4] = '0';
                                    ptr->src[5] = '0';
                                    if ( jumblr_addresstype(ptr->src) != 'z' )
                                        printf("error setting address type to Z: %s\n",jprint(item,0));
                                }
                                else
                                {
                                    t_z = 1;
                                    strcpy(ptr->src,"taddr");
                                    if ( jumblr_addresstype(ptr->src) != 't' )
                                        printf("error setting address type to T: %s\n",jprint(item,0));
                                }
                                printf("%s %s %.8f t_z.%d z_z.%d\n",zaddr,txidstr,total,t_z,z_z); // cant be z->t from spend
                            }
                        } else printf("mismatched %s %s total %.8f vs %.8f -> %lld\n",zaddr,totalstr,dstr(SATOSHIDEN * total),dstr(SATOSHIDEN * jdouble(item,(char *)"amount")),(long long)((uint64_t)(total * SATOSHIDEN) - (uint64_t)(jdouble(item,(char *)"amount") * SATOSHIDEN)));
                    }
                    free_json(array);
                }
                free(retstr);
            }
        }
        free(totalstr);
    }
}

void jumblr_opidsupdate()
{
    char *retstr; cJSON *array; int32_t i,n; struct jumblr_item *ptr;
    if ( (retstr= jumblr_zlistoperationids()) != 0 )
    {
        if ( (array= cJSON_Parse(retstr)) != 0 )
        {
            if ( (n= cJSON_GetArraySize(array)) > 0 && cJSON_IsArray(array) != 0 )
            {
                for (i=0; i<n; i++)
                {
                    if ( (ptr= jumblr_opidadd(jstri(array,i))) != 0 )
                    {
                        if ( ptr->status == 0 )
                            jumblr_opidupdate(ptr);
                        if ( jumblr_addresstype(ptr->src) == 'z' && jumblr_addresstype(ptr->dest) == 't' )
                            jumblr_prune(ptr);
                    }
                }
            }
            free_json(array);
        }
        free(retstr);
    }
}

uint64_t jumblr_increment(uint8_t r,int32_t height,uint64_t total,uint64_t biggest,uint64_t medium, uint64_t smallest)
{
    int32_t i,n; uint64_t incrs[1000],remains = total;
    height /= JUMBLR_SYNCHRONIZED_BLOCKS;
    if ( (height % JUMBLR_SYNCHRONIZED_BLOCKS) == 0 || total >= 100*biggest )
    {
        if ( total >= biggest )
            return(biggest);
        else if ( total >= medium )
            return(medium);
        else if ( total >= smallest )
            return(smallest);
        else return(0);
    }
    else
    {
        n = 0;
        while ( remains > smallest && n < sizeof(incrs)/sizeof(*incrs) )
        {
            if ( remains >= biggest )
                incrs[n] = biggest;
            else if ( remains >= medium )
                incrs[n] = medium;
            else if ( remains >= smallest )
                incrs[n] = smallest;
            else break;
            remains -= incrs[n];
            n++;
        }
        if ( n > 0 )
        {
            r %= n;
            for (i=0; i<n; i++)
                printf("%.8f ",dstr(incrs[i]));
            printf("n.%d incrs r.%d -> %.8f\n",n,r,dstr(incrs[r]));
            return(incrs[r]);
        }
    }
    return(0);
}

void jumblr_iteration()
{
    static int32_t lastheight; static uint32_t lasttime;
    char *zaddr,*addr,*retstr=0,secretaddr[64]; cJSON *array; int32_t i,iter,height,acpublic,counter,chosen_one,n; uint64_t smallest,medium,biggest,amount=0,total=0; double fee; struct jumblr_item *ptr,*tmp; uint16_t r,s;
    acpublic = ASSETCHAINS_PUBLIC;
    if ( ASSETCHAINS_SYMBOL[0] == 0 && GetTime() >= KOMODO_SAPLING_DEADLINE )
        acpublic = 1;
    if ( JUMBLR_PAUSE != 0 || acpublic != 0 )
        return;
    if ( lasttime == 0 )
    {
        if ( (retstr= jumblr_zlistaddresses()) != 0 )
        {
            if ( (array= cJSON_Parse(retstr)) != 0 )
            {
                if ( (n= cJSON_GetArraySize(array)) > 0 && cJSON_IsArray(array) != 0 )
                {
                    for (i=0; i<n; i++)
                        jumblr_zaddrinit(jstri(array,i));
                }
                free_json(array);
            }
            free(retstr), retstr = 0;
        }
    }
    height = (int32_t)chainActive.LastTip()->GetHeight();
    if ( time(NULL) < lasttime+40 )
        return;
    lasttime = (uint32_t)time(NULL);
    if ( lastheight == height )
        return;
    lastheight = height;
    if ( (height % JUMBLR_SYNCHRONIZED_BLOCKS) != JUMBLR_SYNCHRONIZED_BLOCKS-3 )
        return;
    fee = JUMBLR_INCR * JUMBLR_FEE;
    smallest = SATOSHIDEN * ((JUMBLR_INCR + 3*fee) + 3*JUMBLR_TXFEE);
    medium = SATOSHIDEN * ((JUMBLR_INCR + 3*fee)*10 + 3*JUMBLR_TXFEE);
    biggest = SATOSHIDEN * ((JUMBLR_INCR + 3*fee)*777 + 3*JUMBLR_TXFEE);
    OS_randombytes((uint8_t *)&r,sizeof(r));
    s = (r % 3);
    switch ( s )
    {
        case 0: // t -> z
        default:
            if ( Jumblr_deposit[0] != 0 && (total= jumblr_balance(Jumblr_deposit)) >= smallest )
            {
                if ( (zaddr= jumblr_zgetnewaddress()) != 0 )
                {
                    if ( zaddr[0] == '"' && zaddr[strlen(zaddr)-1] == '"' )
                    {
                        zaddr[strlen(zaddr)-1] = 0;
                        addr = zaddr+1;
                    } else addr = zaddr;
                    amount = jumblr_increment(r/3,height,total,biggest,medium,smallest);
                    if ( amount > 0 && (retstr= jumblr_sendt_to_z(Jumblr_deposit,addr,dstr(amount))) != 0 )
                    {
                        printf("sendt_to_z.(%s)\n",retstr);
                        free(retstr), retstr = 0;
                    }
                    free(zaddr);
                } else printf("no zaddr from jumblr_zgetnewaddress\n");
            }
            else if ( Jumblr_deposit[0] != 0 )
                printf("%s total %.8f vs %.8f\n",Jumblr_deposit,dstr(total),(JUMBLR_INCR + 3*(fee+JUMBLR_TXFEE)));
            break;
        case 1: // z -> z
            jumblr_opidsupdate();
            chosen_one = -1;
            for (iter=counter=0; iter<2; iter++)
            {
                counter = n = 0;
                HASH_ITER(hh,Jumblrs,ptr,tmp)
                {
                    if ( ptr->spent == 0 && ptr->status > 0 && jumblr_addresstype(ptr->src) == 't' && jumblr_addresstype(ptr->dest) == 'z' )
                    {
                        if ( (total= jumblr_balance(ptr->dest)) >= (fee + JUMBLR_FEE)*SATOSHIDEN )
                        {
                            if ( iter == 1 && counter == chosen_one )
                            {
                                if ( (zaddr= jumblr_zgetnewaddress()) != 0 )
                                {
                                    if ( zaddr[0] == '"' && zaddr[strlen(zaddr)-1] == '"' )
                                    {
                                        zaddr[strlen(zaddr)-1] = 0;
                                        addr = zaddr+1;
                                    } else addr = zaddr;
                                    if ( (retstr= jumblr_sendz_to_z(ptr->dest,addr,dstr(total))) != 0 )
                                    {
                                        printf("n.%d counter.%d chosen_one.%d send z_to_z.(%s)\n",n,counter,chosen_one,retstr);
                                        free(retstr), retstr = 0;
                                    }
                                    ptr->spent = (uint32_t)time(NULL);
                                    free(zaddr);
                                    break;
                                }
                            }
                            counter++;
                        }
                    }
                    n++;
                }
                if ( counter == 0 )
                    break;
                if ( iter == 0 )
                {
                    OS_randombytes((uint8_t *)&chosen_one,sizeof(chosen_one));
                    if ( chosen_one < 0 )
                        chosen_one = -chosen_one;
                    chosen_one %= counter;
                    printf("jumblr z->z chosen_one.%d of %d, from %d\n",chosen_one,counter,n);
                }
            }
            break;
        case 2: // z -> t
            if ( Jumblr_numsecretaddrs > 0 )
            {
                jumblr_opidsupdate();
                chosen_one = -1;
                for (iter=0; iter<2; iter++)
                {
                    counter = n = 0;
                    HASH_ITER(hh,Jumblrs,ptr,tmp)
                    {
                        if ( ptr->spent == 0 && ptr->status > 0 && jumblr_addresstype(ptr->src) == 'z' && jumblr_addresstype(ptr->dest) == 'z' )
                        {
                            if ( (total= jumblr_balance(ptr->dest)) >= (fee + JUMBLR_FEE)*SATOSHIDEN )
                            {
                                if ( iter == 1 && counter == chosen_one )
                                {
                                    Jumblr_secretaddr(secretaddr);
                                    if ( (retstr= jumblr_sendz_to_t(ptr->dest,secretaddr,dstr(total))) != 0 )
                                    {
                                        printf("%s send z_to_t.(%s)\n",secretaddr,retstr);
                                        free(retstr), retstr = 0;
                                    } else printf("null return from jumblr_sendz_to_t\n");
                                    ptr->spent = (uint32_t)time(NULL);
                                    break;
                                }
                                counter++;
                            }
                        }
                        n++;
                    }
                    if ( counter == 0 )
                        break;
                    if ( iter == 0 )
                    {
                        OS_randombytes((uint8_t *)&chosen_one,sizeof(chosen_one));
                        if ( chosen_one < 0 )
                            chosen_one = -chosen_one;
                        chosen_one %= counter;
                        printf("jumblr z->t chosen_one.%d of %d, from %d\n",chosen_one,counter,n);
                    }
                }
            }
            break;
    }
}
