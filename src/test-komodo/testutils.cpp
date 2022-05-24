#include <cryptoconditions.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <set>

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
#include "utilmoneystr.h"
#include "coincontrol.h"
#include "cc/CCinclude.h"
#include "testutils.h"

void undo_init_notaries(); // test helper
bool CalcPoW(CBlock *pblock); // generate PoW on a block
void BitcoinMiner(CWallet *pwallet); // in miner.cpp
void komodo_init(int32_t height); // in komodo_bitcoind.cpp

std::string notaryPubkey = "0205a8ad0c1dbc515f149af377981aab58b836af008d4d7ab21bd76faf80550b47";
std::string notarySecret = "UxFWWxsf1d7w7K5TvAWSkeX4H95XQKwdwGv49DXwWUTzPTTjHBbU";
CKey notaryKey;

/*
 * We need to have control of clock,
 * otherwise block production can fail.
 */
int64_t nMockTime;

extern int32_t USE_EXTERNAL_PUBKEY;
extern std::string NOTARY_PUBKEY;

void adjust_hwmheight(int32_t in); // in komodo.cpp
CCriticalSection& get_cs_main(); // in main.cpp
std::shared_ptr<CBlock> generateBlock(CWallet* wallet, CValidationState* state = nullptr); // in mining.cpp

void displayTransaction(const CTransaction& tx)
{
    std::cout << "Transaction Hash: " << tx.GetHash().ToString();
    for(size_t i = 0; i < tx.vin.size(); ++i)
    {
        std::cout << "\nvIn " << i 
                << " prevout hash : " << tx.vin[i].prevout.hash.ToString()
                << " n: " << tx.vin[i].prevout.n;
    }
    for(size_t i = 0; i < tx.vout.size(); ++i)
    {
        std::cout << "\nvOut " << i
                << " nValue: " << tx.vout[i].nValue
                << " scriptPubKey: " << tx.vout[i].scriptPubKey.ToString()
                << " interest: " << tx.vout[i].interest;
    }
    std::cout << "\n";
}

void displayBlock(const CBlock& blk)
{
    std::cout << "Block Hash: " << blk.GetHash().ToString()
            << "\nPrev Hash: " << blk.hashPrevBlock.ToString()
            << "\n";
    for(size_t i = 0; i < blk.vtx.size(); ++i)
    {
        std::cout << i << " ";
        displayTransaction(blk.vtx[i]);
    }
    std::cout << "\n";
}

void setConsoleDebugging(bool enable)
{
    fPrintToConsole = enable;
}

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
 * @param block a place to store the block (can be nullptr)
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
    std::shared_ptr<CBlock> block;
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
TestChain::TestChain(CBaseChainParams::Network desiredNetwork)
{
    CleanGlobals();
    previousNetwork = Params().NetworkIDString();
    dataDir = GetTempPath() / strprintf("test_komodo_%li_%i", GetTime(), GetRand(100000));
    ASSETCHAINS_STAKED = 0;
    if (ASSETCHAINS_SYMBOL[0])
        dataDir = dataDir / strprintf("_%s", ASSETCHAINS_SYMBOL);
    boost::filesystem::create_directories(dataDir);
    mapArgs["-datadir"] = dataDir.string();

    setupChain(desiredNetwork);
    USE_EXTERNAL_PUBKEY = 0; // we want control of who mines the block
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
    CleanGlobals();
    // cruel and crude, but cleans up any wallet dbs so subsequent tests run.
    bitdb = std::shared_ptr<CDBEnv>(new CDBEnv{});
    boost::filesystem::remove_all(dataDir);
    if (previousNetwork == "main")
        SelectParams(CBaseChainParams::MAIN);
    if (previousNetwork == "regtest")
        SelectParams(CBaseChainParams::REGTEST);
    if (previousNetwork == "test")
        SelectParams(CBaseChainParams::TESTNET);

}

boost::filesystem::path TestChain::GetDataDir() { return dataDir; }

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

std::shared_ptr<CBlock> TestChain::generateBlock(std::shared_ptr<CWallet> wallet, CValidationState* state)
{
    std::shared_ptr<CBlock> block;
    if (wallet == nullptr)
    {
        ::generateBlock(block);
    }
    else
        block = ::generateBlock(wallet.get(), state);
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
    if (!ProcessNewBlock(1,chainActive.LastTip()->nHeight+1,state, NULL, retVal.get(), true, NULL))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "ProcessNewBlock, block not accepted");
    minedBlocks.push_back(retVal);
    return retVal;
}

/****
 * @brief get a block that is ready to be mined
 * @note The guts of this was taken from mining.cpp's generate() method
 * @returns a block with no PoW
 */
CBlock TestChain::BuildBlock(std::shared_ptr<TestWallet> who)
{
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

bool TestChain::ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex, 
        bool fJustCheck, bool fCheckPOW)
{
    LOCK( get_cs_main() );
    return ::ConnectBlock(block, state, pindex, *(this->GetCoinsViewCache()), fJustCheck, fCheckPOW);
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

/******
 * @brief A wallet for testing
 * @param chain the chain
 * @param name a name for the wallet
 */
TestWallet::TestWallet(const std::string& name) 
        : CWallet( name + ".dat")
{
    key.MakeNewKey(true);
    LOCK(cs_wallet);
    bool firstRunRet;
    DBErrors err = LoadWallet(firstRunRet);
    AddKey(key);
    RegisterValidationInterface(this);
}

/******
 * @brief A wallet for testing
 * @param chain the chain
 * @param in a key that already exists
 * @param name a name for the wallet
 */

TestWallet::TestWallet(const CKey& in, const std::string& name)
        : CWallet( name + ".dat"), key(in)
{
    LOCK( cs_wallet );
    bool firstRunRet;
    DBErrors err = LoadWallet(firstRunRet);
    AddKey(key);
    RegisterValidationInterface(this);
}

TestWallet::~TestWallet()
{
    UnregisterValidationInterface(this);
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
 * Transfer to another user
 * @param to who to transfer to
 * @param amount the amount
 * @returns the results
 */
CTransaction TestWallet::Transfer(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee)
{
    TransactionInProcess tip = CreateSpendTransaction(to, amount, fee);
    if (!CWallet::CommitTransaction( tip.transaction, tip.reserveKey))
        throw std::logic_error("Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
    return tip.transaction;
}

/*************
 * @brief Create a transaction, do not place in mempool
 * @note throws std::logic_error if there was a problem
 * @param to who to send to
 * @param amount the amount to send
 * @param fee the fee
 * @returns the transaction
*/
TransactionInProcess TestWallet::CreateSpendTransaction(std::shared_ptr<TestWallet> to, 
        CAmount amount, CAmount fee, bool commit)
{
    CAmount curBalance = this->GetBalance();
    CAmount curImatureBalance = this->GetImmatureBalance();
    CAmount curUnconfirmedBalance = this->GetUnconfirmedBalance();

    // Check amount
    if (amount <= 0)
        throw std::logic_error("Invalid amount");

    if (amount > curBalance)
        throw std::logic_error("Insufficient funds");

    // Build recipient vector
    std::vector<CRecipient> vecSend;
    bool fSubtractFeeFromAmount = false;
    CRecipient recipient = {GetScriptForDestination(to->GetPubKey()), amount, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);
    // other items needed for transaction creation call
    CAmount nFeeRequired;
    std::string strError;
    int nChangePosRet = -1;
    TransactionInProcess retVal(this);
    if (!CWallet::CreateTransaction(vecSend, retVal.transaction, retVal.reserveKey, nFeeRequired,
            nChangePosRet, strError))
    {
        if (!fSubtractFeeFromAmount && amount + nFeeRequired > GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!",
                    FormatMoney(nFeeRequired));
        throw std::logic_error(strError);
    }

    if (commit && !CommitTransaction(retVal.transaction, retVal.reserveKey))
    {
        std::logic_error("Unable to commit transaction");
    }

    return retVal;
}

/*************
 * @brief Create a transaction, do not place in mempool
 * @note throws std::logic_error if there was a problem
 * @param to who to send to
 * @param amount the amount to send
 * @param fee the fee
 * @param txToSpend the specific transaction to spend (ok if not transmitted yet)
 * @returns the transaction
*/
TransactionInProcess TestWallet::CreateSpendTransaction(std::shared_ptr<TestWallet> to, 
        CAmount amount, CAmount fee, CCoinControl& coinControl)
{
    // verify the passed-in transaction has enough funds
    std::vector<COutPoint> availableTxs;
    coinControl.ListSelected(availableTxs);
    CTransaction tx;
    uint256 hashBlock;
    if (!myGetTransaction(availableTxs[0].hash, tx, hashBlock))
        throw std::logic_error("Requested tx not found");

    CAmount curBalance = tx.vout[availableTxs[0].n].nValue;
    
    // Check amount
    if (amount <= 0)
        throw std::logic_error("Invalid amount");

    if (amount > curBalance)
        throw std::logic_error("Insufficient funds");

    // Build recipient vector
    std::vector<CRecipient> vecSend;
    bool fSubtractFeeFromAmount = false;
    CRecipient recipient = {GetScriptForDestination(to->GetPubKey()), amount, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);
    // other items needed for transaction creation call
    CAmount nFeeRequired;
    std::string strError;
    int nChangePosRet = -1;
    TransactionInProcess retVal(this);
    if (!CreateTransaction(vecSend, retVal.transaction, retVal.reserveKey, strError, &coinControl))
    {
        if (!fSubtractFeeFromAmount && amount + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!",
                    FormatMoney(nFeeRequired));
        throw std::logic_error(strError);
    }

    return retVal;
}

/****
 * @brief create a transaction spending a vout that is not yet in the wallet
 * @param vecSend the recipients
 * @param wtxNew the resultant tx
 * @param reserveKey the key used
 * @param strFailReason the reason for any failure
 * @param outputControl the tx to spend
 * @returns true on success
 */
bool TestWallet::CreateTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, 
        CReserveKey& reservekey, std::string& strFailReason, CCoinControl* coinControl)
{
    bool sign = true;
    int nChangePosRet = 0;
    uint64_t interest2 = 0; 
    CAmount nValue = 0; 
    unsigned int nSubtractFeeFromAmount = 0;
    BOOST_FOREACH (const CRecipient& recipient, vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must be positive");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty() || nValue < 0)
    {
        strFailReason = _("Transaction amounts must be positive");
        return false;
    }

    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.BindWallet(this);
    int nextBlockHeight = chainActive.Height() + 1;
    CMutableTransaction txNew = CreateNewContextualCMutableTransaction(Params().GetConsensus(), nextBlockHeight);

    if (IS_MODE_EXCHANGEWALLET && ASSETCHAINS_SYMBOL[0] == 0) 
        txNew.nLockTime = 0;
    else
    {
        if ( !komodo_hardfork_active((uint32_t)chainActive.LastTip()->nTime) )
            txNew.nLockTime = (uint32_t)chainActive.LastTip()->nTime + 1; // set to a time close to now
        else
            txNew.nLockTime = (uint32_t)chainActive.Tip()->GetMedianTimePast();
    }

    // Activates after Overwinter network upgrade
    if (NetworkUpgradeActive(nextBlockHeight, Params().GetConsensus(), Consensus::UPGRADE_OVERWINTER)) {
        if (txNew.nExpiryHeight >= TX_EXPIRY_HEIGHT_THRESHOLD){
            strFailReason = _("nExpiryHeight must be less than TX_EXPIRY_HEIGHT_THRESHOLD.");
            return false;
        }
    }

    unsigned int max_tx_size = MAX_TX_SIZE_AFTER_SAPLING;
    if (!NetworkUpgradeActive(nextBlockHeight, Params().GetConsensus(), Consensus::UPGRADE_SAPLING)) {
        max_tx_size = MAX_TX_SIZE_BEFORE_SAPLING;
    }
/*
    // Discourage fee sniping.
    //
    // However because of a off-by-one-error in previous versions we need to
    // neuter it by setting nLockTime to at least one less than nBestHeight.
    // Secondly currently propagation of transactions created for block heights
    // corresponding to blocks that were just mined may be iffy - transactions
    // aren't re-accepted into the mempool - we additionally neuter the code by
    // going ten blocks back. Doesn't yet do anything for sniping, but does act
    // to shake out wallet bugs like not showing nLockTime'd transactions at
    // all.
    txNew.nLockTime = std::max(0, chainActive.Height() - 10);

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);*/

    {
        LOCK2(cs_main, cs_wallet);
        {
            CAmount nFeeRet = 0;
            while (true)
            {
                txNew.vin.clear();
                txNew.vout.clear();
                wtxNew.fFromMe = true;
                nChangePosRet = -1;
                bool fFirst = true;

                CAmount nTotalValue = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nTotalValue += nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                BOOST_FOREACH (const CRecipient& recipient, vecSend)
                {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (txout.IsDust(::minRelayTxFee))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                strFailReason = _("The transaction amount is too small to pay the fee");
                            else
                                strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                        }
                        else
                            strFailReason = _("Transaction amount too small");
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                std::vector<std::pair<const CTransaction,unsigned int> > setCoins;
                CAmount nValueIn = 0;
                interest2 = 0;
                std::vector<COutPoint> ctrlVec;
                coinControl->ListSelected(ctrlVec);
                for(const auto& p : ctrlVec)
                {
                    CTransaction tx;
                    uint256 hashBlock;
                    if (myGetTransaction(p.hash, tx, hashBlock))
                    {
                        setCoins.push_back(std::pair<const CTransaction, unsigned int>(tx, p.n));
                    }
                }
                for(const auto& pcoin : setCoins)
                {
                    CAmount nCredit = pcoin.first.vout[pcoin.second].nValue;
                    nValueIn += nCredit;
                    //The coin age after the next block (depth+1) is used instead of the current,
                    //reflecting an assumption the user would accept a bit more delay for
                    //a chance at a free transaction.
                    //But mempool inputs might still be in the mempool, so their age stays 0
                    if ( !IS_MODE_EXCHANGEWALLET && ASSETCHAINS_SYMBOL[0] == 0 )
                    {
                        interest2 += pcoin.first.vout[pcoin.second].interest;
                    }
                    int age = 0;
                    if (age != 0)
                        age += 1;
                    dPriority += (double)nCredit * age;
                }
                if ( ASSETCHAINS_SYMBOL[0] == 0 && DONATION_PUBKEY.size() == 66 && interest2 > 5000 )
                {
                    CScript scriptDonation = CScript() << ParseHex(DONATION_PUBKEY) << OP_CHECKSIG;
                    CTxOut newTxOut(interest2,scriptDonation);
                    int32_t nDonationPosRet = txNew.vout.size() - 1; // dont change first or last
                    std::vector<CTxOut>::iterator position = txNew.vout.begin()+nDonationPosRet;
                    txNew.vout.insert(position, newTxOut);
                    interest2 = 0;
                }
                CAmount nChange = (nValueIn - nValue);
                if (nSubtractFeeFromAmount == 0)
                    nChange -= nFeeRet;

                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-bitcoin-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                        scriptChange = GetScriptForDestination(coinControl->destChange);

                    // no coin control: send change to newly generated address
                    else
                    {
                        // Note: We use a new key here to keep it from being obvious which side is the change.
                        //  The drawback is that by not reusing a previous key, the change may be lost if a
                        //  backup is restored, if the backup doesn't have the new private key for the change.
                        //  If we reused the old key, it would be possible to add code to look for and
                        //  rediscover unknown transactions that were written with keys of ours to recover
                        //  post-backup change.

                        // Reserve a new key pair from key pool
                        CPubKey vchPubKey;
                        bool ret;
                        ret = reservekey.GetReservedKey(vchPubKey);
                        assert(ret); // should never fail, as we just unlocked
                        scriptChange = GetScriptForDestination(vchPubKey.GetID());
                    }

                    CTxOut newTxOut(nChange, scriptChange);

                    // We do not move dust-change to fees, because the sender would end up paying more than requested.
                    // This would be against the purpose of the all-inclusive feature.
                    // So instead we raise the change and deduct from the recipient.
                    if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(::minRelayTxFee))
                    {
                        CAmount nDust = newTxOut.GetDustThreshold(::minRelayTxFee) - newTxOut.nValue;
                        newTxOut.nValue += nDust; // raise change until no more dust
                        for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
                        {
                            if (vecSend[i].fSubtractFeeFromAmount)
                            {
                                txNew.vout[i].nValue -= nDust;
                                if (txNew.vout[i].IsDust(::minRelayTxFee))
                                {
                                    strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                                    return false;
                                }
                                break;
                            }
                        }
                    }

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    if (newTxOut.IsDust(::minRelayTxFee))
                    {
                        nFeeRet += nChange;
                        reservekey.ReturnKey();
                    }
                    else
                    {
                        nChangePosRet = txNew.vout.size() - 1; // dont change first or last
                        std::vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosRet;
                        txNew.vout.insert(position, newTxOut);
                    }
                } else reservekey.ReturnKey();

                // Fill vin
                //
                // Note how the sequence number is set to max()-1 so that the
                // nLockTime set above actually works.
                for(const auto& coin : setCoins)
                    txNew.vin.push_back(CTxIn(coin.first.GetHash(),coin.second,CScript(),
                                              std::numeric_limits<unsigned int>::max()-1));

                // Check mempooltxinputlimit to avoid creating a transaction which the local mempool rejects
                size_t limit = (size_t)GetArg("-mempooltxinputlimit", 0);
                {
                    LOCK(cs_main);
                    if (NetworkUpgradeActive(chainActive.Height() + 1, Params().GetConsensus(), Consensus::UPGRADE_OVERWINTER)) {
                        limit = 0;
                    }
                }
                if (limit > 0) {
                    size_t n = txNew.vin.size();
                    if (n > limit) {
                        strFailReason = _(strprintf("Too many transparent inputs %zu > limit %zu", n, limit).c_str());
                        return false;
                    }
                }

                // Grab the current consensus branch ID
                auto consensusBranchId = CurrentEpochBranchId(chainActive.Height() + 1, Params().GetConsensus());

                // Sign
                int nIn = 0;
                CTransaction txNewConst(txNew);
                for(const auto& coin : setCoins)
                {
                    bool signSuccess;
                    const CScript& scriptPubKey = coin.first.vout[coin.second].scriptPubKey;
                    SignatureData sigdata;
                    if (sign)
                        signSuccess = ProduceSignature(TransactionSignatureCreator(
                                this, &txNewConst, nIn, coin.first.vout[coin.second].nValue, SIGHASH_ALL),
                                scriptPubKey, sigdata, consensusBranchId);
                    else
                        signSuccess = ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata, consensusBranchId);

                    if (!signSuccess)
                    {
                        strFailReason = _("Signing transaction failed");
                        return false;
                    } else {
                        UpdateTransaction(txNew, nIn, sigdata);
                    }

                    nIn++;
                }

                unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);

                // Remove scriptSigs if we used dummy signatures for fee calculation
                if (!sign) {
                    BOOST_FOREACH (CTxIn& vin, txNew.vin)
                        vin.scriptSig = CScript();
                }

                // Embed the constructed transaction data in wtxNew.
                *static_cast<CTransaction*>(&wtxNew) = CTransaction(txNew);

                // Limit size
                if (nBytes >= max_tx_size)
                {
                    strFailReason = _("Transaction too large");
                    return false;
                }

                dPriority = wtxNew.ComputePriority(dPriority, nBytes);

                // Can we complete this as a free transaction?
                if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                {
                    // Not enough fee: enough priority?
                    double dPriorityNeeded = mempool.estimatePriority(nTxConfirmTarget);
                    // Not enough mempool history to estimate: use hard-coded AllowFree.
                    if (dPriorityNeeded <= 0 && AllowFree(dPriority))
                        break;

                    // Small enough, and priority high enough, to send for free
                    if (dPriorityNeeded > 0 && dPriority >= dPriorityNeeded)
                        break;
                }

                CAmount nFeeNeeded = GetMinimumFee(nBytes, nTxConfirmTarget, mempool);
                if ( nFeeNeeded < 5000 )
                    nFeeNeeded = 5000;

                // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                // because we must be at the maximum allowed fee.
                if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                {
                    strFailReason = _("Transaction too large for fee policy");
                    return false;
                }

                if (nFeeRet >= nFeeNeeded)
                    break; // Done, enough fee included.

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                continue;
            }
        }
    }

    return true;
}

/**
 * Call after CreateTransaction unless you want to abort
 */
bool TestWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, CValidationState& state)
{
    {
        LOCK2(cs_main, cs_wallet);
        LogPrintf("CommitTransaction:\n%s", wtxNew.ToString());
        {
            // This is only to keep the database open to defeat the auto-flush for the
            // duration of this scope.  This is the only place where this optimization
            // maybe makes sense; please don't do it anywhere else.
            CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r+") : NULL;

            // Take key pair from key pool so it won't be used again
            reservekey.KeepKey();

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            AddToWallet(wtxNew, false, pwalletdb);

            // Notify that old coins are spent
            std::set<CWalletTx*> setCoins;
            BOOST_FOREACH(const CTxIn& txin, wtxNew.vin)
            {
                CWalletTx &coin = mapWallet[txin.prevout.hash];
                coin.BindWallet(this);
                NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
            }

            if (fFileBacked)
                delete pwalletdb;
        }

        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;

        if (fBroadcastTransactions)
        {
            // Broadcast
            if (!::AcceptToMemoryPool(mempool, state, wtxNew, false, nullptr))
            {
                fprintf(stderr,"commit failed\n");
                // This must not fail. The transaction has already been signed and recorded.
                LogPrintf("CommitTransaction(): Error: Transaction not valid\n");
                return false;
            }
            wtxNew.RelayWalletTransaction();
        }
    }
    return true;
}
