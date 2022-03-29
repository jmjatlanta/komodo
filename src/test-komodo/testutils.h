#pragma once

#include "main.h"
#include "wallet/wallet.h" // CWallet, CReserveKey, etc.
#include <unordered_map>

#define VCH(a,b) std::vector<unsigned char>(a, a + b)

static char ccjsonerr[1000] = "\0";
#define CCFromJson(o,s) \
    o = cc_conditionFromJSONString(s, ccjsonerr); \
    if (!o) FAIL() << "bad json: " << ccjsonerr;


extern std::string notaryPubkey;
extern std::string notarySecret;
extern CKey notaryKey;


void setupChain(CBaseChainParams::Network network = CBaseChainParams::REGTEST);
/***
 * Generate a block
 * @param block a place to store the block (read from disk)
 */
void generateBlock(std::shared_ptr<CBlock> block = nullptr);
bool acceptTx(const CTransaction tx, CValidationState &state);
void acceptTxFail(const CTransaction tx);
/****
 * In order to do tests there needs to be inputs to spend.
 * This method creates a block and returns a transaction that spends the coinbase.
 * @param scriptPubKey
 * @returns the transaction
 */
CTransaction getInputTx(CScript scriptPubKey);
CMutableTransaction spendTx(const CTransaction &txIn, int nOut=0);
std::vector<uint8_t> getSig(const CMutableTransaction mtx, CScript inputPubKey, int nIn=0);

class TestWallet;
class CCoinsViewDB;

class TestChain
{
public:
    /***
     * ctor to create a chain
     * @param desiredNetwork mainnet, testnet, regtest
     * @param data_path where the data is (blank to create a temp dir)
     * @param inMemory keep indexes in memory instead of files
     */
    TestChain(CBaseChainParams::Network desiredNetwork = CBaseChainParams::REGTEST, 
            boost::filesystem::path data_path = "", bool inMemory = true, bool reindex = false);
    /***
     * dtor to release resources
     */
    ~TestChain();
    /***
     * Get the block index at the specified height
     * @param height the height (0 indicates current height
     * @returns the block index
     */
    CBlockIndex *GetIndex(uint32_t height = 0) const;
    /***
     * Get this chains view of the state of the chain
     * @returns the view
     */
    CCoinsViewCache *GetCoinsViewCache();

    /***
     * @brief Get this chain's block tree database
     * @note this is a low-level object, and only exposed for testing purposes. Higher
     * layers should probably not talk to this object directly.
     * @return the block tree db
     */
    CBlockTreeDB* GetBlockTreeDB();

    /**
     * Generate a block
     * @param who who will mine the block
     * @returns the block generated
     */
    std::shared_ptr<CBlock> generateBlock(std::shared_ptr<TestWallet> who = nullptr);

    /****
     * @brief generate PoW on block and submit to chain
     * @param in the block to push
     * @return the block
     */
    std::shared_ptr<CBlock> generateBlock(const CBlock& in);

    /***
     * @brief Build a block, but do not add PoW or submit to chain
     * @param who the miner
     */
    CBlock BuildBlock( std::shared_ptr<TestWallet> who );

    std::shared_ptr<CBlock> GetBlock(CBlockIndex* idx) const;
    
    /****
     * @brief set the chain time to something reasonable
     * @note must be called after generateBlock if you
     * want to produce another block
     */
    void IncrementChainTime();
    /***
     * @returns the notary's key
     */
    CKey getNotaryKey();
    /***
     * Add a transactoion to the mempool
     * @param tx the transaction
     * @returns the results
     */
    CValidationState acceptTx(const CTransaction &tx);
    /***
     * Creates a wallet with a specific key
     * @param key the key
     * @returns the wallet
     */
    std::shared_ptr<TestWallet> AddWallet(const CKey &key, const std::string& name = "");
    /****
     * Create a wallet
     * @returns the wallet
     */
    std::shared_ptr<TestWallet> AddWallet(const std::string& name = "");
protected:
    std::vector<std::shared_ptr<TestWallet>> toBeNotified;
    boost::filesystem::path dataDir;
    std::string previousNetwork;
    void CleanGlobals();
    void SetupMining(std::shared_ptr<TestWallet> who);
    std::vector<std::shared_ptr<CBlock>> minedBlocks;
    bool removeDataOnDestruction = true;
    bool inMemory = true;
    CCoinsViewDB* pcoinsdbview = nullptr;
    /***
     * @brief called by ctor to get things going
     * @param network the network to start
     * @param existing true to initialize with existing data on the drive
     */
    void StartChain(CBaseChainParams::Network network, bool existing);

};

class TransactionReference
{
public:
    uint256 hash;
    uint32_t index;
    TransactionReference(uint256 h, uint32_t i) : hash(h), index(i) {}
};

/***
 * A simplistic (dumb) wallet for helping with testing
 * - It does not keep track of spent transactions
 * - Blocks containing vOuts that apply are added to the front of a vector
 */
class TestWallet : public CWallet
{
public:
    TestWallet(TestChain* chain, const std::string& name = "");
    TestWallet(TestChain* chain, const CKey& in, const std::string& name = "");
    /***
     * @returns the public key
     */
    CPubKey GetPubKey() const;
    /***
     * @returns the private key
     */
    CKey GetPrivKey() const;
    /****
     * @brief shortcuts the real wallet to return the private key
     * @return true
     */
    virtual bool GetKey(const CKeyID &address, CKey &keyOut) const override 
            { keyOut = GetPrivKey(); return true; }
    /***
     * Sign a typical transaction
     * @param hash the hash to sign
     * @param hashType SIGHASH_ALL or something similar
     * @returns the bytes to add to ScriptSig
     */
    std::vector<unsigned char> Sign(uint256 hash, unsigned char hashType);
    /***
     * Sign a cryptocondition
     * @param cc the cryptocondition
     * @param hash the hash to sign
     * @returns the bytes to add to ScriptSig
     */
    std::vector<unsigned char> Sign(CC* cc, uint256 hash);
    /***
     * Notifies this wallet of a new block
     */
    void BlockNotification(std::shared_ptr<CBlock> block);

    /***
     * Add a transaction to the list of available vouts
     * @param tx the transaction
     * @param n the n value of the vout
     */
    CWalletTx& AddOut(const CTransaction& tx, uint32_t n);
    /*****
     * @brief create a transaction with 1 recipient (signed)
     * @param to who to send funds to
     * @param amount
     * @param fee
     * @returns the transaction
     */
    CTransaction CreateSpendTransaction(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee = 0);
    /***
     * Transfer to another user (sends to mempool)
     * @param to who to transfer to
     * @param amount the amount
     * @returns the results
     */
    CValidationState Transfer(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee = 0);
    virtual CReserveKey GetReserveKey() override;
    virtual void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool) override;


    /****
     * @brief get avalable outputs
     * @param[out] vCoins available outputs
     * @param fOnlyConfirmed only include confirmed txs
     * @param coinControl
     * @param fIncludeZeroValue
     * @param fIncludeCoinBase
     */
    virtual void AvailableCoins(std::vector<COutput>& vCoins, 
            bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL, 
            bool fIncludeZeroValue=false, bool fIncludeCoinBase=true) const override;

    bool IsSpent(const uint256& hash, unsigned int n) const override;
    bool IsMine(const uint256& hash, uint32_t voutNum) const;

    void DisplayContents();

private:
    TestChain *chain;
    CKey key;
    std::unordered_multimap<std::string, CWalletTx> availableTransactions;
    // the tx hash and the index within the tx
    std::unordered_multimap<std::string, uint32_t> spentTransactions;
    CScript destScript;
    const std::string name;
};
