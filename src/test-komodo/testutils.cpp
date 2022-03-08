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
    mapArgs["-mineraddress"] = "bogus";
    Params().GetConsensus().SetCoinbaseMaturity(1);
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

CBlock getBlock(const uint256& blockId)
{
    CBlock retVal;
    if (!ReadBlockFromDisk(retVal, mapBlockIndex[blockId], false))
        throw;
    return retVal;
}

/***
 * Generate a block
 * @param block a place to store the block (nullptr skips the disk read)
 */
void generateBlock(CBlock *block)
{
    SetMockTime(nMockTime+=100);  // CreateNewBlock can fail if not enough time passes

    UniValue params;
    params.setArray();
    params.push_back(1);

    try {
        UniValue out = generate(params, false, CPubKey());
        uint256 blockId;
        blockId.SetHex(out[0].getValStr());
        *block = getBlock(blockId);
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
    CBlock block;
    generateBlock(&block);
    CTransaction coinbase = block.vtx[0];

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
TestChain::TestChain(CBaseChainParams::Network desiredNetwork)
{
    CleanGlobals();
    previousNetwork = Params().NetworkIDString();
    dataDir = GetTempPath() / strprintf("test_komodo_%li_%i", GetTime(), GetRand(100000));
    if (ASSETCHAINS_SYMBOL[0])
        dataDir = dataDir / strprintf("_%s", ASSETCHAINS_SYMBOL);
    boost::filesystem::create_directories(dataDir);
    mapArgs["-datadir"] = dataDir.string();

    setupChain(desiredNetwork);
    CBitcoinSecret vchSecret;
    vchSecret.SetString(notarySecret); // this returns false due to network prefix mismatch but works anyway
    notaryKey = vchSecret.GetKey();
    // set up the Pubkeys array
    komodo_init(1);
    // now add to the Pubkeys array
    knotary_entry *kp = (knotary_entry*)calloc(1, sizeof(knotary_entry));
    int nextId = Pubkeys->numnotaries;
    memcpy(kp->pubkey, &notaryKey.GetPubKey()[0],33);
    kp->notaryid = nextId;
    HASH_ADD_KEYPTR(hh,Pubkeys[0].Notaries,kp->pubkey,33,kp);
    Pubkeys[0].numnotaries++;

}

TestChain::~TestChain()
{
    CleanGlobals();
    boost::filesystem::remove_all(dataDir);
    if (previousNetwork == "main")
        SelectParams(CBaseChainParams::MAIN);
    if (previousNetwork == "regtest")
        SelectParams(CBaseChainParams::REGTEST);
    if (previousNetwork == "test")
        SelectParams(CBaseChainParams::TESTNET);

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
CBlockIndex *TestChain::GetIndex(uint32_t height)
{
    if (height == 0)
        return chainActive.LastTip();
    return chainActive[height];

}

void TestChain::IncrementChainTime()
{
    SetMockTime(nMockTime += 100);
}

CCoinsViewCache *TestChain::GetCoinsViewCache()
{
    return pcoinsTip;
}

CBlock TestChain::GetBlock(CBlockIndex* idx)
{
    return getBlock( idx->GetBlockHash() );
}

std::shared_ptr<CBlock> TestChain::generateBlock(std::shared_ptr<TestWallet> who)
{
    std::shared_ptr<CBlock> block = std::make_shared<CBlock>();
    SetupMining(who);
    if (who == nullptr)
    {
        // use the RPC method
        ::generateBlock(block.get());
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
            // this is normal
            if (height != this->GetIndex()->GetHeight())
            {
                CBlock currBlock = this->GetBlock(this->GetIndex());
                *(block) = currBlock;
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

std::shared_ptr<TestWallet> TestChain::AddWallet(const CKey& in)
{
    std::shared_ptr<TestWallet> retVal = std::make_shared<TestWallet>(this, in);
    toBeNotified.push_back(retVal);
    return retVal;
}

std::shared_ptr<TestWallet> TestChain::AddWallet()
{
    std::shared_ptr<TestWallet> retVal = std::make_shared<TestWallet>(this);
    toBeNotified.push_back(retVal);
    return retVal;
}


/***
 * A simplistic (dumb) wallet for helping with testing
 * - It does not keep track of spent transactions
 * - Blocks containing vOuts that apply are added to the front of a vector
 */

TestWallet::TestWallet(TestChain* chain) : chain(chain)
{
    key.MakeNewKey(true);
    destScript = GetScriptForDestination(key.GetPubKey());
}

TestWallet::TestWallet(TestChain* chain, const CKey& in) : chain(chain), key(in)
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
    // TODO: remove spent txs from availableTransactions
    // see if this block has any outs for me
    for( auto& tx : block->vtx )
    {
        for(uint32_t i = 0; i < tx.vout.size(); ++i)
        {
            if (tx.vout[i].scriptPubKey == destScript)
            {
                availableTransactions.push_back(CWalletTx(this, tx));
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
void TestWallet::AddOut(CTransaction tx, uint32_t n)
{
    availableTransactions.push_back(CWalletTx(this, tx));
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
    return chain->acceptTx(fundTo);
}

CTransaction TestWallet::CreateSpendTransaction(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee)
{
    std::vector<COutput> available;
    this->AvailableCoins(available);
    CMutableTransaction tx;
    // add txs until we do not need more
    CAmount collected = 0;
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
                    CTxIn incoming;
                    incoming.prevout.hash = out.GetHash();
                    incoming.prevout.n = idx;
                    tx.vin.push_back(incoming);
                    collected += out.nValue;
                }
            }
        }
        if (collected >= amount + fee)
            break;
    }
    if (collected < amount + fee)
        throw std::logic_error("No Funds");
    CTxOut out1;
    out1.scriptPubKey = GetScriptForDestination(to->GetPubKey());
    out1.nValue = amount;
    tx.vout.push_back(out1);
    // give the rest back to wallet owner
    CTxOut out2;
    out2.scriptPubKey = GetScriptForDestination(key.GetPubKey());
    out2.nValue = collected - amount - fee;
    tx.vout.push_back(out2);

    uint256 hash = SignatureHash(destScript, tx, 0, SIGHASH_ALL, 0, 0);
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
    for( auto p : this->availableTransactions)
    {
        vCoins.push_back(COutput(&p, p.nIndex, p.GetDepthInMainChain(), p.GetBlocksToMaturity()));
    }
}
