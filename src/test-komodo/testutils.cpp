#include <cryptoconditions.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "core_io.h"
#include "key.h"
#include "main.h"
#include "miner.h"
#include "notarisationdb.h"
#include "random.h"
#include "rpc/server.h"
#include "rpc/protocol.h"
#include "txdb.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#include "consensus/validation.h"
#include "primitives/transaction.h"
#include "script/cc.h"
#include "script/interpreter.h"
#include "wallet/wallet.h"
#include "komodo_extern_globals.h"

#include "testutils.h"

void undo_init_notaries(); // test helper
bool CalcPoW(CBlock *pblock); // generate PoW on a block
void BitcoinMiner(CWallet *pwallet); // in miner.cpp
void komodo_init(int32_t height); // in komodo_bitcoind.cpp

std::string notaryPubkey = "0205a8ad0c1dbc515f149af377981aab58b836af008d4d7ab21bd76faf80550b47";
std::string notarySecret = "UxFWWxsf1d7w7K5TvAWSkeX4H95XQKwdwGv49DXwWUTzPTTjHBbU";
CKey notaryKey;
extern CWallet* pwalletMain;
extern CBlockTreeDB* pblocktree; // in main.cpp

/*
 * We need to have control of clock,
 * otherwise block production can fail.
 */
int64_t nMockTime;

void setupChain(CBaseChainParams::Network network)
{
    SelectParams(network);

    // Settings to get block reward
    NOTARY_PUBKEY = notaryPubkey;
    STAKED_NOTARY_ID = -1;
    USE_EXTERNAL_PUBKEY = 0;
    // dirty trick to release coinbase funds
    ASSETCHAINS_TIMEUNLOCKFROM = 100;
    ASSETCHAINS_TIMEUNLOCKTO = 100;
    mapArgs["-mineraddress"] = "bogus";
    // Global mock time
    nMockTime = GetTime();
    
    // Unload
    UnloadBlockIndex();

    // Init blockchain
    ClearDatadirCache();
    pblocktree = new CBlockTreeDB(1 << 20, true);
    CCoinsViewDB *pcoinsdbview = new CCoinsViewDB(1 << 23, true);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);
    pnotarisations = new NotarisationDB(1 << 20, true);
    InitBlockIndex();
}

void setupChain()
{
    setupChain(CBaseChainParams::REGTEST);
}

std::shared_ptr<CBlock> getBlock(const uint256& blockId)
{
    std::shared_ptr<CBlock> retVal = std::make_shared<CBlock>();
    if (!ReadBlockFromDisk( *(retVal), mapBlockIndex[blockId], false))
        throw;
    return retVal;
}

/***
 * Generate a block
 * @param block a place to store the block (nullptr skips the disk read)
 */
void generateBlock(std::shared_ptr<CBlock> block)
{
    SetMockTime(nMockTime+=100);  // CreateNewBlock can fail if not enough time passes

    UniValue params;
    params.setArray();
    params.push_back(1);

    try {
        UniValue out = generate(params, false, CPubKey());
        uint256 blockId;
        blockId.SetHex(out[0].getValStr());
        block = getBlock(blockId);
    } catch (const UniValue& e) {
        FAIL() << "failed to create block: " << e.write().data();
    }
}

/***
 * Accept a transaction, failing the gtest if the tx is not accepted
 * @param tx the transaction to be accepted
 */
void acceptTxFail(const CTransaction tx)
{
    CValidationState state;
    if (!acceptTx(tx, state)) 
        FAIL() << state.GetRejectReason();
}


bool acceptTx(const CTransaction tx, CValidationState &state)
{
    LOCK(cs_main);
    bool missingInputs = false;
    bool accepted = AcceptToMemoryPool(mempool, state, tx, false, &missingInputs, false, -1);
    if (state.IsValid() && (!accepted || missingInputs))
    {
        std::string message;
        if (!accepted)
            message = "Not accepted";
        if (missingInputs)
        {
            if (message.size() > 0)
                message += " due to ";
            message += "Missing Inputs";
        }
        state.DoS(100, false, 0U, message);
    }
    return accepted && !missingInputs;
}

/***
 * Create a transaction based on input
 * @param txIn the vin data (which becomes prevout)
 * @param nOut the index of txIn to use as prevout
 * @returns the transaction
 */
CMutableTransaction spendTx(const CTransaction &txIn, int nOut)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = txIn.GetHash();
    mtx.vin[0].prevout.n = nOut;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = txIn.vout[nOut].nValue - 1000;
    return mtx;
}


std::vector<uint8_t> getSig(const CMutableTransaction mtx, CScript inputPubKey, int nIn)
{
    uint256 hash = SignatureHash(inputPubKey, mtx, nIn, SIGHASH_ALL, 0, 0);
    std::vector<uint8_t> vchSig;
    notaryKey.Sign(hash, vchSig);
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    return vchSig;
}


/*
 * In order to do tests there needs to be inputs to spend.
 * This method creates a block and returns a transaction that spends the coinbase.
 */
CTransaction getInputTx(CScript scriptPubKey)
{
    // Get coinbase
    std::shared_ptr<CBlock> block = std::make_shared<CBlock>();
    generateBlock(block);
    CTransaction coinbase = block->vtx[0];

    // Create tx
    auto mtx = spendTx(coinbase);
    mtx.vout[0].scriptPubKey = scriptPubKey;
    uint256 hash = SignatureHash(coinbase.vout[0].scriptPubKey, mtx, 0, SIGHASH_ALL, 0, 0);
    std::vector<unsigned char> vchSig;
    notaryKey.Sign(hash, vchSig);
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mtx.vin[0].scriptSig << vchSig;

    // Accept
    acceptTxFail(mtx);
    return CTransaction(mtx);
}

/****
 * A class to provide a simple chain for tests
 */
TestChain::TestChain(CBaseChainParams::Network desiredNetwork, boost::filesystem::path data_path,
        bool inMemory) : inMemory(inMemory)
{
    bool existingDataDir = true;
    if (data_path.empty())
    {
        existingDataDir = false;
        dataDir = GetTempPath() / strprintf("test_komodo_%li_%i", GetTime(), GetRand(100000));
        if (ASSETCHAINS_SYMBOL[0])
            dataDir = dataDir / strprintf("_%s", ASSETCHAINS_SYMBOL);
        mapArgs["-datadir"] = dataDir.string();
        boost::filesystem::create_directories(dataDir);
        if (!inMemory)
        {
            auto otherPath = GetDataDir(true) / "blocks" / "index";
            boost::filesystem::create_directories(otherPath);
        }
    }
    else
    {
        dataDir = data_path;
        mapArgs["-datadir"] = dataDir.string();
    }

    CleanGlobals();
    previousNetwork = Params().NetworkIDString();
    ASSETCHAINS_STAKED = 0;

    StartChain(desiredNetwork, existingDataDir);
    CBitcoinSecret vchSecret;
    vchSecret.SetString(notarySecret); // this returns false due to network prefix mismatch but works anyway
    notaryKey = vchSecret.GetKey();
    // set up the Pubkeys array
    komodo_init(1);
    // now add the notary to the Pubkeys array for era 0
    knotary_entry *kp = (knotary_entry*)calloc(1, sizeof(knotary_entry));
    int nextId = Pubkeys[0].numnotaries;
    memcpy(kp->pubkey, &notaryKey.GetPubKey()[0],33);
    kp->notaryid = nextId;
    HASH_ADD_KEYPTR(hh,Pubkeys[0].Notaries,kp->pubkey,33,kp);
    // fix the numnotaries for all arrays
    for(int i = 0; i < 125; ++i)
        Pubkeys[i].numnotaries++;
}

TestChain::~TestChain()
{
    if (removeDataOnDestruction)
        boost::filesystem::remove_all(dataDir);
    else
        FlushStateToDisk();
    if (pnotarisations != nullptr)
    {
        delete pnotarisations;
        pnotarisations = nullptr;
    }
    if (pcoinsTip != nullptr)
    {
        delete pcoinsTip;
        pcoinsTip = nullptr;
    }
    if (pcoinsdbview != nullptr)
    {
        delete pcoinsdbview;
        pcoinsdbview = nullptr;
    }
    if (pblocktree != nullptr)
    {
        delete pblocktree;
        pblocktree = nullptr;
    }
    CleanGlobals();
    if (previousNetwork == "main")
        SelectParams(CBaseChainParams::MAIN);
    if (previousNetwork == "regtest")
        SelectParams(CBaseChainParams::REGTEST);
    if (previousNetwork == "test")
        SelectParams(CBaseChainParams::TESTNET);

}

void TestChain::StartChain(CBaseChainParams::Network network, bool existing)
{
    SelectParams(network);

    // Settings to get block reward
    NOTARY_PUBKEY = notaryPubkey;
    STAKED_NOTARY_ID = -1;
    USE_EXTERNAL_PUBKEY = 0;
    // dirty trick to release coinbase funds
    ASSETCHAINS_TIMEUNLOCKFROM = 100;
    ASSETCHAINS_TIMEUNLOCKTO = 100;
    mapArgs["-mineraddress"] = "bogus";
    // Global mock time
    nMockTime = GetTime();
    
    // Unload
    UnloadBlockIndex();

    // Init blockchain
    ClearDatadirCache();
    pblocktree = new CBlockTreeDB(1 << 20, inMemory);
    pcoinsdbview = new CCoinsViewDB(1 << 23, inMemory);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);
    pnotarisations = new NotarisationDB(1 << 20, inMemory);
    if (existing)
    {
        if (!LoadBlockIndex())
            throw std::logic_error("Unable to load block index");
    }
    else
        InitBlockIndex();
}

void TestChain::CleanGlobals()
{
    // hwmheight can get skewed if komodo_connectblock not called (which some tests do)
    undo_init_notaries();
    for(int i = 0; i < 33; ++i)
    {
        komodo_state s = KOMODO_STATES[i];
        s.events.clear();
        // TODO: clean notarization points
    }
}

/***
 * Get the block index at the specified height
 * @param height the height (0 indicates current height)
 * @returns the block index
 */
CBlockIndex *TestChain::GetIndex(uint32_t height) const
{
    if (height == 0)
        return chainActive.LastTip();
    return chainActive[height];

}

CBlockTreeDB* TestChain::GetBlockTreeDB()
{
    return pblocktree;
}

void TestChain::IncrementChainTime()
{
    SetMockTime(nMockTime += 100);
}

CCoinsViewCache *TestChain::GetCoinsViewCache()
{
    return pcoinsTip;
}

/****
 * @brief find a block by its index
 * @param idx the index
 * @return the block
 */
std::shared_ptr<CBlock> TestChain::GetBlock(CBlockIndex* idx) const
{
    uint256 hash = idx->GetBlockHash();
    for(auto itr = minedBlocks.rbegin(); itr != minedBlocks.rend(); ++itr)
    {
        if ( (*(itr))->GetHash() == hash)
            return *itr;
    }
    return nullptr;
}

std::shared_ptr<CBlock> TestChain::generateBlock(std::shared_ptr<TestWallet> who)
{
    std::shared_ptr<CBlock> block = std::make_shared<CBlock>();
    SetupMining(who);
    if (who == nullptr)
    {
        // use the RPC method
        ::generateBlock(block);
    }
    else
    {
        int32_t height = this->GetIndex()->GetHeight();
        // use the real miner (returns after 1 block for RegTest)
        try
        {
            BitcoinMiner(who.get());
        }
        catch (const boost::thread_interrupted& ex)
        {
            // this exception is normal, it is used to get out of the mining loop
            // when on the regtest chain
            if (height != this->GetIndex()->GetHeight())
            {
                block = getBlock(GetIndex()->GetBlockHash());
            }
        }
    }
    if (block != nullptr && !block->IsNull())
    {
        minedBlocks.push_back(block);
        for(auto wallet : toBeNotified)
        {
            wallet->BlockNotification(block);
        }
    }
    return block;
}

class MockReserveKey : public CReserveKey
{
public:
    MockReserveKey(TestWallet* wallet) : CReserveKey(wallet)
    {
        pubKey = wallet->GetPubKey();
    }
    ~MockReserveKey() {}
    virtual bool GetReservedKey(CPubKey& pubKey) override
    {
        pubKey = this->pubKey;
        return true;
    }
protected:
    CPubKey pubKey;
};

/***
 * @brief mine a block using a simple PoW algo (as RPC does)
 * @param in the block to mine
 * @returns the mined block
 */
std::shared_ptr<CBlock> TestChain::generateBlock(const CBlock& in)
{
    std::shared_ptr<CBlock> retVal = std::make_shared<CBlock>();
    *(retVal) = in;
    CalcPoW(retVal.get());
    CValidationState state;
    if (!ProcessNewBlock(1,chainActive.LastTip()->GetHeight()+1,state, NULL, retVal.get(), true, NULL))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "ProcessNewBlock, block not accepted");
    minedBlocks.push_back(retVal);
    return retVal;
}

/*****
 * @brief set global variables for mining
 */
void TestChain::SetupMining(std::shared_ptr<TestWallet> who)
{
    KOMODO_MININGTHREADS = 1;
    // adjust globals for notary
    if (who == nullptr || who->GetPrivKey() == getNotaryKey())
    {
        IS_KOMODO_NOTARY = true;
        CPubKey notary_pubkey = notaryKey.GetPubKey();
        if (who != nullptr)
            pwalletMain = who.get();
        else
            pwalletMain = toBeNotified[0].get();
        int keylen = notary_pubkey.size();
        for(int i = 0; i < keylen; ++i)
            NOTARY_PUBKEY33[i] = notary_pubkey[i];
    }
    else
    {
        IS_KOMODO_NOTARY = false;
        memset(NOTARY_PUBKEY33, 0, 33);
        pwalletMain = who.get();
    }
}

/****
 * @brief get a block that is ready to be mined
 * @note The guts of this was taken from mining.cpp's generate() method
 * @returns a block with no PoW
 */
CBlock TestChain::BuildBlock(std::shared_ptr<TestWallet> who)
{
    SetupMining(who);
    MockReserveKey reserveKey(who.get());
    int nHeight = chainActive.Height();
    unsigned int nExtraNonce = 0;
    UniValue blockHashes(UniValue::VARR);
    uint64_t lastTime = 0;
    // Validation may fail if block generation is too fast
    if (GetTime() == lastTime) MilliSleep(1001);
    lastTime = GetTime();

    std::unique_ptr<CBlockTemplate> pblocktemplate(
            CreateNewBlockWithKey(reserveKey,nHeight,KOMODO_MAXGPUCOUNT));

    if (!pblocktemplate.get())
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Wallet keypool empty");

    CBlock *pblock = &pblocktemplate->block;
    IncrementExtraNonce(pblock, chainActive.LastTip(), nExtraNonce);

    CBlock retVal = *pblock;
    return retVal;
}

CKey TestChain::getNotaryKey() { return notaryKey; }

CValidationState TestChain::acceptTx(const CTransaction& tx)
{
    CValidationState retVal;
    bool accepted = ::acceptTx(tx, retVal);
    if (!accepted && retVal.IsValid())
        retVal.DoS(100, false, 0U, "acceptTx returned false");
    return retVal;
}

std::shared_ptr<TestWallet> TestChain::AddWallet(const CKey& in, const std::string& name)
{
    std::shared_ptr<TestWallet> retVal = std::make_shared<TestWallet>(this, in, name);
    toBeNotified.push_back(retVal);
    return retVal;
}

std::shared_ptr<TestWallet> TestChain::AddWallet(const std::string& name)
{
    std::shared_ptr<TestWallet> retVal = std::make_shared<TestWallet>(this, name);
    toBeNotified.push_back(retVal);
    return retVal;
}


/***
 * A simplistic (dumb) wallet for helping with testing
 * - It does not keep track of spent transactions
 * - Blocks containing vOuts that apply are added to the front of a vector
 */

TestWallet::TestWallet(TestChain* chain, const std::string& name) : chain(chain), name(name)
{
    key.MakeNewKey(true);
    destScript = GetScriptForDestination(key.GetPubKey());
}

TestWallet::TestWallet(TestChain* chain, const CKey& in, const std::string& name)
        : chain(chain), key(in), name(name)
{
    destScript = GetScriptForDestination(key.GetPubKey());
}

/***
 * @returns the public key
 */
CPubKey TestWallet::GetPubKey() const { return key.GetPubKey(); }

/***
 * @returns the private key
 */
CKey TestWallet::GetPrivKey() const { return key; }

CReserveKey TestWallet::GetReserveKey()
{
    return MockReserveKey(this);
}

void TestWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
{
    nIndex = 0;
    keypool.vchPubKey = this->GetPubKey();
}


/***
 * Sign a typical transaction
 * @param hash the hash to sign
 * @param hashType SIGHASH_ALL or something similar
 * @returns the bytes to add to ScriptSig
 */
std::vector<unsigned char> TestWallet::Sign(uint256 hash, unsigned char hashType)
{
    std::vector<unsigned char> retVal;
    key.Sign(hash, retVal);
    retVal.push_back(hashType);
    return retVal;
}

/***
 * Sign a cryptocondition
 * @param cc the cryptocondition
 * @param hash the hash to sign
 * @returns the bytes to add to ScriptSig
 */
std::vector<unsigned char> TestWallet::Sign(CC* cc, uint256 hash)
{
    int out = cc_signTreeSecp256k1Msg32(cc, key.begin(), hash.begin());            
    return CCSigVec(cc);
}

/***
 * Notifies this wallet of a new block
 */
void TestWallet::BlockNotification(std::shared_ptr<CBlock> block)
{
    // Loop through all transactions in the block
    for(auto txIndex = 0; txIndex < block->vtx.size(); ++txIndex)
    {
        const auto& tx = block->vtx[txIndex];

        // loop through all vIns in the transaction
        for(uint32_t i = 0; i < tx.vin.size(); ++i)
        {
            const CTxIn& in = tx.vin[i];
            // find available transactions where the hash matches the 
            for( auto itr = availableTransactions.find(in.prevout.hash.ToString())
                    ; itr != availableTransactions.end(); ++itr)
            {
                if ((*itr).first != in.prevout.hash.ToString())
                    break;
                auto& mytx = (*itr).second;
                // loop through the vouts of the CWalletTx to see if we just spent it
                for(int i = 0; i < mytx.vout.size(); ++i)
                {
                    auto& out = mytx.vout[i];
                    // are we spending this one?
                    if (in.prevout.hash == mytx.GetHash() && in.prevout.n == i)
                        spentTransactions.insert( {in.prevout.hash.ToString(), i} );
                }
            }
        }
        // Did I get something?
        for(uint32_t i = 0; i < tx.vout.size(); ++i)
        {
            if (tx.vout[i].scriptPubKey == destScript)
            {
                CWalletTx& curr = AddOut(tx, i);
                curr.SetMerkleBranch( *block ); // also sets nIndex
                break; // skip to next tx
            }
        }
    }
}

/***
 * Add a transaction to the list of available vouts
 * @param tx the transaction
 * @param n the n value of the vout
 */
CWalletTx& TestWallet::AddOut(const CTransaction& tx, uint32_t n)
{
    auto itr = availableTransactions.insert( {tx.GetHash().ToString(), CWalletTx(this, tx) } );
    return (*itr).second;
}

/***
 * Transfer to another user
 * @param to who to transfer to
 * @param amount the amount
 * @returns the results
 */
CValidationState TestWallet::Transfer(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee)
{
    CTransaction fundTo(CreateSpendTransaction(to, amount, fee));
    /*
    std::cerr << "There are " << fundTo.vin.size() << " vin transactions.\n";
    for(int i = 0; i < fundTo.vin.size(); ++i)
        std::cerr << "Transaction " << std::to_string(i)
                << " uses transaction " << fundTo.vin[i].prevout.hash.ToString()
                << " vout " << std::to_string(fundTo.vin[i].prevout.n) << "\n";
    */
    return chain->acceptTx(fundTo);
}

/***
 * @brief create a transaction
 * @param to who to send to
 * @param amount the amount to transfer
 * @param fee the fee amount
 * @return the transaction
 */
CTransaction TestWallet::CreateSpendTransaction(
        std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee)
{
    std::vector<COutput> available;
    this->AvailableCoins(available);
    CMutableTransaction tx;
    // add txs until we do not need more
    CAmount collected = 0;
    CScript scriptToSign;
    uint32_t inIndex;
    for(auto output : available)
    {
        // go through and add up all my available outs
        if (output.fSpendable)
        {
            for(int idx = 0; idx < output.tx->vout.size(); ++idx)
            {
                const CTxOut& out = output.tx->vout[idx];
                if (out.scriptPubKey == destScript)
                {
                    tx.vin.push_back( CTxIn( output.tx->GetHash(), idx ) );
                    scriptToSign = output.tx->vout[idx].scriptPubKey;
                    inIndex = idx;
                    collected += out.nValue;
                }
            }
        }
        if (collected >= amount + fee)
            break;
    }
    if (collected < amount + fee)
        throw std::logic_error("No Funds");
    // give the rest back to wallet owner
    CTxOut out1;
    out1.scriptPubKey = GetScriptForDestination(key.GetPubKey());
    out1.nValue = collected - amount - fee;
    tx.vout.push_back(out1);
    // spend
    CTxOut out2;
    out2.scriptPubKey = GetScriptForDestination(to->GetPubKey());
    out2.nValue = amount;
    tx.vout.push_back(out2);

    uint256 hash = SignatureHash(scriptToSign, tx, inIndex, SIGHASH_ALL, amount, 
        CurrentEpochBranchId(chainActive.Height() + 1, Params().GetConsensus()));
    tx.vin[0].scriptSig << Sign(hash, SIGHASH_ALL);

    return CTransaction(tx);
}

/****
 * @brief get avalable outputs
 * @param[out] vCoins available outputs
 * @param fOnlyConfirmed only include confirmed txs
 * @param coinControl
 * @param fIncludeZeroValue
 * @param fIncludeCoinBase
 */
void TestWallet::AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, 
            bool fIncludeZeroValue, bool fIncludeCoinBase) const
{
    for( auto itr = availableTransactions.begin(); itr != availableTransactions.end(); ++itr)
    {
        auto& tx = (*itr).second;
        // loop through outs of tx
        for(int i = 0; i < tx.vout.size(); ++i)
            if (IsMine(tx.GetHash(), i) && !IsSpent(tx.GetHash(), i))
                vCoins.push_back(COutput(&tx, tx.nIndex, tx.GetDepthInMainChain(), true));
    }
}

bool TestWallet::IsMine(const uint256& hash, uint32_t n) const
{
    auto itr = availableTransactions.find(hash.ToString());
    while (itr != availableTransactions.end() && (*itr).first == hash.ToString())
    {
        auto& walletTx = (*itr).second;
        if (walletTx.vout.size() > n && walletTx.vout[n].scriptPubKey == destScript)
            return true;
        ++itr;
    }
    return false;
}

bool TestWallet::IsSpent(const uint256& hash, unsigned int n) const
{
    bool retVal = false;
    auto itr = spentTransactions.find(hash.ToString());
    while ( itr != spentTransactions.end() && (*itr).first == hash.ToString())
    {
        if (n == (*itr).second)
            return true;
        ++itr;
    }
    return false;
}

void TestWallet::DisplayContents()
{
    std::vector<COutput> available;
    AvailableCoins(available);
    std::cout << "Wallet of " << name << " contains " 
            << std::to_string(available.size()) << " transactions.\n";
    CAmount total = 0;
    for( const auto& out : available )
    {
        for (const auto& vout : out.tx->vout)
        {
            if (vout.scriptPubKey == destScript)
            {
                total += vout.nValue;
                std::cout << "  Transaction with a value of " 
                        << std::to_string(vout.nValue) << "\n";
            }
        }
    }
    std::cout << "Total: " << std::to_string(total) << "\n";
}