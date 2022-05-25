#pragma once

#include "main.h"
#include "wallet/wallet.h" // CWallet, CReserveKey, etc.
#include "consensus/validation.h"

#define VCH(a,b) std::vector<unsigned char>(a, a + b)

static char ccjsonerr[1000] = "\0";
#define CCFromJson(o,s) \
    o = cc_conditionFromJSONString(s, ccjsonerr); \
    if (!o) FAIL() << "bad json: " << ccjsonerr;


extern std::string notaryPubkey;
extern std::string notarySecret;
extern CKey notaryKey;

/***
 * @brief Look inside a transaction
 * @param tx the transaction to look at
 */
void displayTransaction(const CTransaction& tx);
/****
 * @brief Look inside a block
 * @param blk the block to look at
 */
void displayBlock(const CBlock& blk);

void setConsoleDebugging(bool enable);

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

class TransactionInProcess
{
public:
    TransactionInProcess(CWallet* wallet) : reserveKey(wallet) {}
    CWalletTx transaction;
    CReserveKey reserveKey;
};

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
    std::shared_ptr<CBlock> generateBlock(std::shared_ptr<CWallet> wallet, 
            CValidationState* validationState = nullptr);
    
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
    /****
     * @brief attempt to connect a block to the chain
     * @param block the block to connect
     * @param state where to hold the results
     * @param pindex the new chain index
     * @param justCheck whether or not to do all checks
     * @param checkPOW true to check PoW
     * @returns true on success
     */
    bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex,
            bool fJustCheck = false,bool fCheckPOW = false);
    
    boost::filesystem::path GetDataDir();

protected:
    boost::filesystem::path dataDir;
    bool removeDataOnDestruction = true;

private:
    std::string previousNetwork;
    void CleanGlobals();
    void SetupMining(std::shared_ptr<TestWallet> who);
    std::vector<std::shared_ptr<CBlock>> minedBlocks;
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
 * An easy-to-use wallet for testing Komodo
 */
class TestWallet : public CWallet
{
public:
    TestWallet(const std::string& name);
    TestWallet(const CKey& in, const std::string& name);
    ~TestWallet();
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
    /*****
     * @brief create a transaction with 1 recipient (signed)
     * @param to who to send funds to
     * @param amount
     * @param fee
     * @returns the transaction
     */
    TransactionInProcess CreateSpendTransaction(std::shared_ptr<TestWallet> to, CAmount amount,
            CAmount fee = 0, bool commit = true);
    /*************
     * @brief Create a transaction, do not place in mempool
     * @note throws std::logic_error if there was a problem
     * @param to who to send to
     * @param amount the amount to send
     * @param fee the fee
     * @param txToSpend the specific transaction to spend (ok if not transmitted yet)
     * @returns the transaction
    */
    TransactionInProcess CreateSpendTransaction(std::shared_ptr<TestWallet> to, 
            CAmount amount, CAmount fee, CCoinControl& coinControl);
    /****
     * @brief create a transaction spending a vout that is not yet in the wallet
     * @param vecSend the recipients
     * @param wtxNew the resultant tx
     * @param reserveKey the key used
     * @param strFailReason the reason for any failure
     * @param outputControl the tx to spend
     * @returns true on success
     */
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, 
            CReserveKey& reservekey, std::string& strFailReason, CCoinControl* coinControl);
    using CWallet::CommitTransaction;
    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, CValidationState& state);
    /***
     * Transfer to another user (sends to mempool)
     * @param to who to transfer to
     * @param amount the amount
     * @returns the results
     */
    CTransaction Transfer(std::shared_ptr<TestWallet> to, CAmount amount, CAmount fee = 0);
private:
    CKey key;
};
