#pragma once

#include "main.h"
#include "wallet/wallet.h"

#define VCH(a,b) std::vector<unsigned char>(a, a + b)

static char ccjsonerr[1000] = "\0";
#define CCFromJson(o,s) \
    o = cc_conditionFromJSONString(s, ccjsonerr); \
    if (!o) FAIL() << "bad json: " << ccjsonerr;


extern std::string notaryPubkey;
extern std::string notarySecret;
extern CKey notaryKey;

class TestWallet;

/***
 * Clear any leftovers from tests
 */
void teardownChain();

void setupChain();
/***
 * Generate a block
 * @param block a place to store the block (read from disk)
 */
void generateBlock(CBlock *block=nullptr, bool includeMempool = true, std::shared_ptr<TestWallet> wallet = nullptr);
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

class TestChain
{
public:
    /***
     * ctor to create a chain
     */
    TestChain();
    /***
     * dtor to release resources
     */
    ~TestChain();
    /***
     * Get the block index at the specified height
     * @param height the height (0 indicates current height
     * @returns the block index
     */
    CBlockIndex *GetIndex(uint32_t height = 0);
    /***
     * Get this chains view of the state of the chain
     * @returns the view
     */
    CCoinsViewCache *GetCoinsViewCache();
    /**
     * Generate a block
     * @param prevHash the hash of the previous block (blank attaches to longest chain)
     * @param includeMempool true to include mempool transactions in block
     * @param miner who gets the credit
     * @returns the block generated
     */
    CBlock generateBlock(uint256 prevHash, bool includeMempool = true, std::shared_ptr<TestWallet> miner = nullptr);
    /****
     * @brief add a block to the longest chain
     * @param includeMempool true to include mempool transactions in block
     * @returns the block added
     */
    CBlock generateBlock(bool includeMempool = true);
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
    std::shared_ptr<TestWallet> AddWallet(const CKey &key);
    /****
     * Create a wallet
     * @returns the wallet
     */
    std::shared_ptr<TestWallet> AddWallet();
    /***
     * @return the number of transactions in the mempool
     */
    uint32_t MempoolSize();
    /***
     * @param height the height to look for
     * @returns the block hash that is at the height of the longest chain
     */
    uint256 BlockHash(uint32_t height = 0);
private:
    std::vector<std::shared_ptr<TestWallet>> toBeNotified;
    boost::filesystem::path dataDir;
    std::string previousNetwork;
    bool generateBlockAndNotify(CBlock& in, bool includeMempool, std::shared_ptr<TestWallet> miner = nullptr);
};

/***
 * A simplistic (dumb) wallet for helping with testing
 * - It does not keep track of spent transactions
 * - Blocks containing vOuts that apply are added to the front of a vector
 */
class TestWallet : public CWallet
{
public:
    TestWallet(TestChain* chain);
    TestWallet(TestChain* chain, const CKey& in);
    /***
     * @returns the public key
     */
    CPubKey GetPubKey() const;
    /***
     * @returns the private key
     */
    CKey GetPrivKey() const;
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
    void BlockNotification(const CBlock& block);
    /***
     * Get a transaction that has funds
     * NOTE: If no single transaction matches, throws
     * @param needed how much is needed
     * @returns a pair of CTransaction and the n value of the vout
     */
    std::pair<CTransaction, uint32_t> GetAvailable(CAmount needed);

    /***
     * Add a transaction to the list of available vouts
     * @param tx the transaction
     * @param n the n value of the vout
     */
    void AddOut(CTransaction tx, uint32_t n);
    /*****
     * @brief create a transaction with 1 recipient (unsigned)
     * @param to who to send funds to
     * @param amount
     * @param fee
     * @returns the transaction
     */
    CMutableTransaction CreateUnsignedSpendTransaction(std::shared_ptr<TestWallet> to, 
            std::pair<CTransaction, uint32_t> txToSpend, CAmount amount, CAmount fee);
    /*****
     * @brief create a transaction with 1 recipient (signed)
     * @param to who to send funds to
     * @param amount
     * @param fee
     * @returns the transaction
     */
    CTransaction CreateSpendTransaction(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee = 0);
    CTransaction SignTransaction(CMutableTransaction in, std::pair<CTransaction, uint32_t> txToSpend);
    /***
     * Transfer to another user (sends to mempool)
     * @param to who to transfer to
     * @param amount the amount
     * @returns the results
     */
    CValidationState Transfer(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee = 0);
private:
    TestChain *chain;
    CKey key;
    std::vector<std::pair<CTransaction, uint8_t>> availableTransactions;
    CScript destScript;
};
